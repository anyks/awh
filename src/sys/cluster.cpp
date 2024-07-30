/**
 * @file: server.cpp
 * @date: 2024-07-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

// Подключаем заголовочный файл
#include <sys/cluster.hpp>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Глобальный объект воркера
	 */
	awh::cluster_t::worker_t * worker = nullptr;
	/**
	 * process Метод перезапуска упавшего процесса
	 * @param pid    идентификатор упавшего процесса
	 * @param status статус остановившегося процесса
	 */
	void awh::Cluster::Worker::process(const pid_t pid, const int32_t status) noexcept {
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
					// Выполняем остановку чтение сообщений
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
					} else if((this->cluster->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - broker->date) <= 180000) {
						// Выполняем остановку работы
						this->cluster->stop(this->wid);
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					// this->_log->print("Child process stopped, PID=%d, STATUS=%x", log_t::flag_t::CRITICAL, broker->pid, status);
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
	 * child Функция фильтр перехватчика сигналов
	 * @param signal номер сигнала полученного системой
	 * @param info   объект информации полученный системой
	 * @param ctx    передаваемый внутренний контекст
	 */
	void awh::Cluster::Worker::child(int32_t signal, siginfo_t * info, void * ctx) noexcept {
		// Зануляем неиспользуемые переменные
		(void) info;
		// Идентификатор упавшего процесса
		pid_t pid = 0;
		// Статус упавшего процесса
		int32_t status = 0;
		// Выполняем получение идентификатора упавшего процесса
		while((pid = ::waitpid(-1, &status, WNOHANG)) > 0){
			// Если работа процесса завершена
			if(status > 0){
				// Если объект воркера инициализирован
				if(worker != nullptr)
					// Выполняем создание дочернего потока
					std::thread(&worker_t::process, worker, pid, status).detach();
				// Выходим из цикла
				break;
			// Если нужно выполнить нормальное завершение работы
			} else {
				// Если объект воркера инициализирован
				if(worker != nullptr)
					// Выполняем остановку работы
					worker->cluster->stop(worker->wid);
				// Выполняем завершение работы
				::exit(status);
			}
		}
	}
	/**
	 * message Функция обратного вызова получении сообщений
	 * @param fd    файловый дескриптор (сокет)
	 * @param event произошедшее событие
	 */
	void awh::Cluster::Worker::message(SOCKET fd, const base_t::event_type_t event) noexcept {
		// Если процесс является родительским
		if(this->cluster->_pid == static_cast <pid_t> (::getpid())){
			// Определяем тип события
			switch(static_cast <uint8_t> (event)){
				// Если выполняется событие закрытие подключения
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from child process [%u] is closed", log_t::flag_t::CRITICAL, this->cluster->_pid, ::getpid());
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
				// Если выполняется событие чтения данных с сокета
				case static_cast <uint8_t> (base_t::event_type_t::READ): {
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
							found = (static_cast <SOCKET> (broker->mfds[0]) == fd);
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
							// Выполняем закрытие файлового дескриптора
							::close(fd);
							// Выходим из функции
							return;
						}
					}
					// Размер входящих сообщений
					const int32_t bytes = ::read(fd, this->_buffer, sizeof(this->_buffer));
					// Если сообщение получено полностью
					if(bytes != 0){
						// Если данные прочитанны правильно
						if(bytes > 0){
							// Выполняем поиск буфера полезной нагрузки
							auto i = this->_payload.find(pid);
							// Если буфер полезной нагрузки найден
							if(i != this->_payload.end())
								// Добавляем полученные данные в смарт-буфер
								i->second->buffer.insert(i->second->buffer.end(), reinterpret_cast <char *> (this->_buffer), reinterpret_cast <char *> (this->_buffer) + static_cast <size_t> (bytes));
							// Если буфер полезной нагрузки не найден
							else {
								// Выполняем создание буфера полезной нагрузки
								auto ret = this->_payload.emplace(pid, unique_ptr <payload_t> (new payload_t));
								// Добавляем полученные данные в смарт-буфер
								ret.first->second->buffer.insert(ret.first->second->buffer.end(), reinterpret_cast <char *> (this->_buffer), reinterpret_cast <char *> (this->_buffer) + static_cast <size_t> (bytes));
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
				} break;
			}
		// Если процесс является дочерним
		} else if(this->cluster->_pid == static_cast <pid_t> (::getppid())) {
			// Определяем тип события
			switch(static_cast <uint8_t> (event)){
				// Если выполняется событие закрытие подключения
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from main process is closed", log_t::flag_t::CRITICAL, ::getpid());
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
				// Если выполняется событие чтения данных с сокета
				case static_cast <uint8_t> (base_t::event_type_t::READ): {
					// Выполняем поиск текущего брокера
					auto i = this->cluster->_brokers.find(this->wid);
					// Если текущий брокер найден
					if(i != this->cluster->_brokers.end()){
						// Получаем индекс текущего процесса
						const uint16_t index = this->cluster->_pids.at(::getpid());
						// Получаем объект текущего брокера
						broker_t * broker = i->second.at(index).get();
						// Если файловый дескриптор не соответствует родительскому
						if(broker->cfds[0] != fd){
							// Переходим по всему списку брокеров
							for(auto & item : i->second){
								// Если брокер не является текущим брокером
								if((broker->cfds[0] != item->cfds[0]) && (broker->mfds[1] != item->mfds[1])){
									// Выполняем остановку чтение сообщений
									item->mess.stop();
									// Выполняем остановку подписки на отправку данных
									item->send.stop();
									// Закрываем файловый дескриптор на чтение из дочернего процесса
									::close(item->cfds[0]);
									// Закрываем файловый дескриптор на запись в основной процесс
									::close(item->mfds[1]);
								}
							}
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
							// Выполняем остановку подписки на отправку данных
							broker->send.stop();
							// Выходим из функции
							return;
						}
						// Размер входящих сообщений
						const int32_t bytes = ::read(fd, this->_buffer, sizeof(this->_buffer));
						// Если сообщение получено полностью
						if(bytes != 0){
							// Если данные прочитанны правильно
							if(bytes > 0){
								// Выполняем поиск буфера полезной нагрузки
								auto i = this->_payload.find(this->cluster->_pid);
								// Если буфер полезной нагрузки найден
								if(i != this->_payload.end())
									// Добавляем полученные данные в смарт-буфер
									i->second->buffer.insert(i->second->buffer.end(), reinterpret_cast <char *> (this->_buffer), reinterpret_cast <char *> (this->_buffer) + static_cast <size_t> (bytes));
								// Если буфер полезной нагрузки не найден
								else {
									// Выполняем создание буфера полезной нагрузки
									auto ret = this->_payload.emplace(this->cluster->_pid, unique_ptr <payload_t> (new payload_t));
									// Добавляем полученные данные в смарт-буфер
									ret.first->second->buffer.insert(ret.first->second->buffer.end(), reinterpret_cast <char *> (this->_buffer), reinterpret_cast <char *> (this->_buffer) + static_cast <size_t> (bytes));
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
				} break;
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
			// Объект передаваемых данных
			data_t data{};
			// Размер смещения в буфере
			const size_t offset = sizeof(mess_t);
			// Выполняем обработку входящего буфера данных
			while(!i->second->buffer.empty() && (i->second->buffer.size() >= offset)){
				// Выполняем извлечение входящих данных
				::memcpy(&i->second->message, i->second->buffer.data(), offset);
				// Определяем семейство кластера
				switch(static_cast <uint8_t> (family)){
					// Если идентификатор семейства кластера является мастер-процессом
					case static_cast <uint8_t> (family_t::MASTER): {
						// Если размер данных соответствует
						if((i->second->message.size > 0) && (i->second->message.size <= MAX_MESSAGE)){
							// Если данные прочитаны правильно
							if(i->second->buffer.size() >= (i->second->message.size + offset)){
								// Устанавливаем идентификатор процесса
								data.pid = i->second->message.pid;
								// Устанавливаем буфер передаваемых данных
								data.buffer.assign(i->second->buffer.data() + offset, i->second->buffer.data() + (offset + i->second->message.size));
								// Если размер буфера больше количества удаляемых байт
								if(i->second->buffer.size() >= (i->second->message.size + offset))
									// Удаляем количество обработанных байт
									i->second->buffer.erase(i->second->buffer.begin(), i->second->buffer.begin() + (i->second->message.size + offset));
								// Если байт в буфере меньше, просто очищаем буфер
								else i->second->buffer.clear();
								// Выполняем сброс общего размера сообщения
								i->second->message.size = 0;
								// Если асинхронный режим работы активирован
								if(this->async){
									// Выполняем запуск работы скрина
									this->_screen.start();
									// Выполняем отправку полученных данных
									this->_screen = std::move(data);
								// Если асинхронный режим работы деактивирован
								} else {
									// Выполняем остановку работы дочернего процесса
									this->_screen.stop();
									// Выполняем отправку полученных данных напрямую
									this->callback(std::move(data));
								}
							// Выходим из цикла
							} else break;
						// Если данные пришли битые
						} else {
							// Выполняем очистку буфера данных
							i->second->buffer.clear();
							// Выводим сообщение что данные пришли битые
							this->_log->print("[%u] Data from child process [%u] arrives corrupted", log_t::flag_t::CRITICAL, this->cluster->_pid, pid);
							// Выходим из цикла
							break;
						}
					} break;
					// Если идентификатор семейства кластера является дочерним-процессом
					case static_cast <uint8_t> (family_t::CHILDREN): {
						// Если размер данных соответствует
						if((i->second->message.size > 0) && (i->second->message.size <= MAX_MESSAGE)){
							// Если нужно завершить работу процесса
							if(i->second->message.quit){
								// Если данные прочитаны правильно
								if(i->second->buffer.size() >= (i->second->message.size + offset)){
									// Устанавливаем идентификатор процесса
									data.pid = i->second->message.pid;
									// Устанавливаем буфер передаваемых данных
									data.buffer.assign(i->second->buffer.data() + offset, i->second->buffer.data() + (offset + i->second->message.size));
									// Выполняем отправку полученных данных
									this->callback(std::move(data));
								}
								// Останавливаем чтение данных с родительского процесса
								this->cluster->stop(this->wid);
								// Выходим из приложения
								::exit(SIGCHLD);
							// Если данные прочитаны правильно
							} else if(i->second->buffer.size() >= (i->second->message.size + offset)) {
								// Устанавливаем идентификатор процесса
								data.pid = i->second->message.pid;
								// Устанавливаем буфер передаваемых данных
								data.buffer.assign(i->second->buffer.data() + offset, i->second->buffer.data() + (offset + i->second->message.size));
								// Если размер буфера больше количества удаляемых байт
								if(i->second->buffer.size() >= (i->second->message.size + offset))
									// Удаляем количество обработанных байт
									i->second->buffer.erase(i->second->buffer.begin(), i->second->buffer.begin() + (i->second->message.size + offset));
								// Если байт в буфере меньше, просто очищаем буфер
								else i->second->buffer.clear();
								// Выполняем сброс общего размера сообщения
								i->second->message.size = 0;
								// Если асинхронный режим работы активирован
								if(this->async){
									// Выполняем запуск работы скрина
									this->_screen.start();
									// Выполняем отправку полученных данных
									this->_screen = std::move(data);
								// Если асинхронный режим работы деактивирован
								} else {
									// Выполняем остановку работы дочернего процесса
									this->_screen.stop();
									// Выполняем отправку полученных данных напрямую
									this->callback(std::move(data));
								}
							// Выходим из цикла
							} else break;
						// Если данные пришли пустыми
						} else {
							// Если нужно завершить работу процесса
							if(i->second->message.quit){
								// Останавливаем чтение данных с родительского процесса
								this->cluster->stop(this->wid);
								// Выходим из приложения
								::exit(SIGCHLD);
							// Если данные пришли битые
							} else {
								// Выполняем очистку буфера данных
								i->second->buffer.clear();
								// Выводим сообщение что данные пришли битые
								this->_log->print("[%u] Data from main process arrives corrupted", log_t::flag_t::CRITICAL, ::getpid());
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			}
		}
	}
#endif
/**
 * Worker Конструктор
 * @param log объект для работы с логами
 */
awh::Cluster::Worker::Worker(const log_t * log) noexcept :
 wid(0), async(false), working(false), restart(false),
 count(1), _buffer{0}, cluster(nullptr),
 _screen(Screen <data_t>::health_t::DEAD), _log(log) {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку функции обратного вызова при получении сообщения
		this->_screen = static_cast <function <void (const data_t &)>> (std::bind(&worker_t::callback, this, _1));
	#endif
}
/**
 * ~Worker Деструктор
 */
awh::Cluster::Worker::~Worker() noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем остановку работы дочернего процесса
		this->_screen.stop();
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
						unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
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
					unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
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
								// Если база событий уже существует
								if(this->_base != nullptr)
									// Выполняем удаление базы событий
									delete this->_base;
								// Выполняем создание новой базы событий
								this->_base = new base_t(this->_fmk, this->_log);
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
								// Делаем сокет на чтение неблокирующим
								this->_socket.blocking(broker->cfds[0], socket_t::mode_t::DISABLE);
								// Делаем сокет на запись неблокирующим
								this->_socket.blocking(broker->mfds[1], socket_t::mode_t::DISABLE);
								// Устанавливаем идентификатор процесса
								broker->pid = pid;
								// Устанавливаем время начала жизни процесса
								broker->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
								// Устанавливаем базу событий для чтения
								broker->mess = this->_base;
								// Устанавливаем сокет для чтения
								broker->mess = broker->cfds[0];
								// Устанавливаем событие на чтение данных от основного процесса
								broker->mess = std::bind(&worker_t::message, i->second.get(), _1, _2);
								// Запускаем чтение данных с основного процесса
								broker->mess.start();
								// Устанавливаем базу событий для записи
								broker->send = this->_base;
								// Устанавливаем сокет для записи
								broker->send = broker->mfds[1];
								// Устанавливаем событие на запись данных в основной процесс
								broker->send = static_cast <base_t::callback_t> (std::bind(&cluster_t::write, this, wid, pid, _1, _2));
								// Запускаем запись данных в основной процесс
								broker->send.start();
								// Выполняем активацию работы события
								broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
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
						// Делаем сокет на чтение неблокирующим
						this->_socket.blocking(broker->mfds[0], socket_t::mode_t::DISABLE);
						// Делаем сокет на запись неблокирующим
						this->_socket.blocking(broker->cfds[1], socket_t::mode_t::DISABLE);
						// Устанавливаем PID-процесса
						broker->pid = pid;
						// Устанавливаем время начала жизни процесса
						broker->date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Устанавливаем базу событий для чтения
						broker->mess = this->_base;
						// Устанавливаем сокет для чтения
						broker->mess = broker->mfds[0];
						// Устанавливаем событие на чтение данных от дочернего процесса
						broker->mess = std::bind(&worker_t::message, i->second.get(), _1, _2);
						// Выполняем запуск работы чтения данных с дочерних процессов
						broker->mess.start();
						// Устанавливаем базу событий для записи
						broker->send = this->_base;
						// Устанавливаем сокет для записи
						broker->send = broker->cfds[1];
						// Устанавливаем событие на запись данных в дочерний процесс
						broker->send = static_cast <base_t::callback_t> (std::bind(&cluster_t::write, this, wid, pid, _1, _2));
						// Запускаем запись данных в дочерний процесс
						broker->send.start();
						// Если не нужно останавливаться на создании процессов
						if(!stop)
							// Выполняем создание новых процессов
							this->fork(i->first, index + 1, stop);
						// Выполняем активацию работы чтения данных с дочерних процессов
						else broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
					}
				}
			// Если все процессы удачно созданы
			} else if((i->second->working = !stop)) {
				// Выполняем поиск брокера
				auto j = this->_brokers.find(i->first);
				// Если идентификатор воркера получен
				if(j != this->_brokers.end()){
					// Если нужно отслеживать падение дочерних процессов
					if(this->_trackCrash){
						// Запоминаем текущий объект воркера
						worker = i->second.get();
						// Выполняем зануление структур перехватчиков событий
						::memset(&this->_sa, 0, sizeof(this->_sa));
						// Устанавливаем функцию перехвадчика событий
						this->_sa.sa_sigaction = worker_t::child;
						// Устанавливаем флаги перехвата сигналов
						this->_sa.sa_flags = SA_RESTART | SA_SIGINFO;
						// Устанавливаем маску перехвата
						sigemptyset(&this->_sa.sa_mask);
						// Активируем перехватчик событий
						::sigaction(SIGCHLD, &this->_sa, nullptr);
					}
					// Выполняем перебор всех доступных брокеров
					for(auto & broker : j->second)
						// Выполняем активацию работы чтения данных с дочерних процессов
						broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("process"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, this->_pid, event_t::START);
				}
			}
		}
	#endif
}
/**
 * send Метод асинхронной отправки буфера данных в сокет
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer буфер для записи данных
 * @param size   размер записываемых данных
 * @param fd     идентификатор файлового дескриптора
 */
void awh::Cluster::send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size, const SOCKET fd) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если идентификатор брокера подключений существует
		if((buffer != nullptr) && (size > 0) && (fd != INVALID_SOCKET)){
			// Выполняем запись в сокет
			const ssize_t bytes = ::write(fd, buffer, size);
			// Если данные отправлены не полностью
			if((bytes > 0) && (bytes < size))
				// Выполняем создание нового фрейма
				this->emplace(wid, pid, buffer + bytes, size - bytes, fd);
			// Если запись в сокет не выполнена
			else if(bytes < 0)
				// Выполняем создание нового фрейма
				this->emplace(wid, pid, buffer, size, fd);
			// Если подключение завершено
			else if(bytes == 0) {
				// Если подключение закрыто, выходим из приложения
				this->_log->print("Process [%u] is broken pipe", log_t::flag_t::CRITICAL, ::getpid());
				// Останавливаем чтение данных с родительского процесса
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
			}
		}
	#endif
}
/**
 * emplace Метод добавления нового буфера полезной нагрузки
 * @param wid    идентификатор воркера
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер полезной нагрузки
 * @param size   размер бинарного буфера полезной нагрузки
 * @param fd     идентификатор файлового дескриптора
 */
void awh::Cluster::emplace(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size, const SOCKET fd) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если идентификатор брокера подключений существует
		if((buffer != nullptr) && (size > 0) && (fd != INVALID_SOCKET)){
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
					// Получаем размер данных который возможно скопировать
					actual = ((MAX_PAYLOAD >= (size - offset)) ? (size - offset) : MAX_PAYLOAD);
					// Если количество копируемых байт больше нуля
					if(actual > 0){
						// Объект полезной нагрузки для отправки
						payload_t payload;
						// Устанавливаем файловый дескриптор
						payload.fd = fd;
						// Устанавливаем идентификатор процесса
						payload.pid = pid;
						// Устанавливаем размер буфера данных
						payload.size = actual;
						// Выполняем создание буфера данных
						payload.data = unique_ptr <char []> (new char [actual]);
						// Выполняем копирование буфера полезной нагрузки
						::memcpy(payload.data.get(), buffer + offset, actual);
						// Выполняем смещение в буфере
						offset += actual;
						// Увеличиваем смещение в буфере полезной нагрузки
						payload.offset += actual;
						// Ещем для указанного потока очередь полезной нагрузки
						auto i = this->_payloads.find(wid);
						// Если для потока очередь полезной нагрузки получена
						if(i != this->_payloads.end())
							// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
							i->second.emplace(pid, std::move(payload));
						// Если для потока почередь полезной нагрузки ещё не сформированна
						else {
							// Создаём новую очередь полезной нагрузки
							auto ret = this->_payloads.emplace(wid, multimap <pid_t, payload_t> ());
							// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
							ret.first->second.emplace(pid, std::move(payload));
						}
					// Выходим из цикла
					} else break;
				/**
				 * Если не все данные полезной нагрузки установлены, создаём новый буфер
				 */
				} while(offset < size);
				// Выполняем поиск брокеров
				auto i = this->_brokers.find(wid);
				// Если брокер найден
				if(i != this->_brokers.end()){
					// Выполняем поиск индекса брокера
					auto j = this->_pids.find(pid);
					// Если индекс брокера найден
					if(j != this->_pids.end())
						// Запускаем ожидание записи данных
						i->second.at(j->second)->send.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
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
 * write Метод записи буфера данных в сокет
 * @param wid   идентификатор воркера
 * @param pid   идентификатор процесса для получения сообщения
 * @param fd    идентификатор файлового дескриптора
 * @param event возникшее событие
 */
void awh::Cluster::write(const uint16_t wid, const pid_t pid, const SOCKET fd, const base_t::event_type_t event) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем поиск брокеров
		auto i = this->_brokers.find(wid);
		// Если брокер найден
		if(i != this->_brokers.end()){
			// Выполняем поиск индекса активного процесса
			auto j = this->_pids.find(pid);
			// Если индекс активного процесса найден
			if(j != this->_pids.end()){
				// Ещем для указанного потока очередь полезной нагрузки
				auto k = this->_payloads.find(wid);
				// Если для потока очередь полезной нагрузки получена
				if((k != this->_payloads.end()) && !k->second.empty()){
					// Выполняем поиск идентификатора процесса
					auto l = k->second.find(pid);
					// Если объект полезной нагрузки найден
					if((l != k->second.end()) && ((l->second.offset - l->second.pos) > 0)){
						// Останавливаем детектирования возможности записи в сокет
						i->second.at(j->second)->send.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
						// Выполняем запись в сокет
						const size_t bytes = ::write(fd, l->second.data.get() + l->second.pos, l->second.offset - l->second.pos);
						// Если данные записаны удачно
						if(bytes > 0){
							// Увеличиваем смещение в бинарном буфере
							l->second.pos += bytes;
							// Если все данные записаны успешно, тогда удаляем результат
							if(l->second.pos == l->second.offset){
								// Выполняем удаление буфера буфера полезной нагрузки
								k->second.erase(l);
								// Если очередь полностью пустая
								if(k->second.empty())
									// Выполняем удаление всей очереди
									this->_payloads.erase(k);
							}
						}
						// Если опередей полезной нагрузки нет, отключаем событие ожидания записи
						if(this->_payloads.find(wid) != this->_payloads.end()){
							// Если сокет подключения активен
							if((fd != INVALID_SOCKET) && (fd < MAX_SOCKETS))
								// Запускаем ожидание записи данных
								i->second.at(j->second)->send.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
						}
					// Если данных для отправки больше нет и сокет подключения активен
					} else if((fd != INVALID_SOCKET) && (fd < MAX_SOCKETS))
						// Останавливаем детектирования возможности записи в сокет
						i->second.at(j->second)->send.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
				// Останавливаем детектирования возможности записи в сокет
				} else i->second.at(j->second)->send.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
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
			if((i != this->_brokers.end()) && (this->_pids.find(pid) != this->_pids.end())){
				// Создаём объект сообщения
				mess_t message;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = pid;
				// Выполняем установку размера отправляемых данных
				message.size = size;
				// Получаем идентификатор брокера
				broker_t * broker = i->second.at(this->_pids.at(pid)).get();
				// Выполняем отправку сообщения дочернему процессу
				this->send(wid, ::getpid(), reinterpret_cast <const char *> (&message), sizeof(message), broker->mfds[1]);
				// Выполняем отправку буфера полезной нагрузки
				this->send(wid, ::getpid(), buffer, size, broker->mfds[1]);
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
			if((i != this->_brokers.end()) && (this->_pids.find(pid) != this->_pids.end())){
				// Создаём объект сообщения
				mess_t message;
				// Выполняем установку размера отправляемых данных
				message.size = size;
				// Устанавливаем пид процесса отправившего сообщение
				message.pid = this->_pid;
				// Получаем идентификатор брокера
				broker_t * broker = i->second.at(this->_pids.at(pid)).get();
				// Выполняем отправку сообщения дочернему процессу
				this->send(wid, pid, reinterpret_cast <const char *> (&message), sizeof(message), broker->cfds[1]);
				// Выполняем отправку буфера полезной нагрузки
				this->send(wid, pid, buffer, size, broker->cfds[1]);
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
						this->send(wid, broker->pid, reinterpret_cast <const char *> (&message), sizeof(message), broker->cfds[1]);
						// Выполняем отправку буфера полезной нагрузки
						this->send(wid, broker->pid, buffer, size, broker->cfds[1]);
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
		// Выполняем освобождение выделенной памяти брокеров подключения
		map <uint16_t, vector <unique_ptr <broker_t>>> ().swap(this->_brokers);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку блоков полезной нагрузки
	this->_payloads.clear();
	// Выполняем освобождение выделенной памяти
	map <uint16_t, unique_ptr <worker_t>> ().swap(this->_workers);
	// Выполняем освобождение выделенной памяти блоков полезной нагрузки
	map <uint16_t, multimap <pid_t, payload_t>> ().swap(this->_payloads);
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
					// Выполняем остановку чтение сообщений
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
				// Выполняем остановку чтение сообщений
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
void awh::Cluster::base(base_t * base) noexcept {
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
