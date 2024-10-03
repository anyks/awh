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
		auto j = this->_ctx->_brokers.find(this->_wid);
		// Если брокер найден
		if(j != this->_ctx->_brokers.end()){
			// Выполняем поиск завершившегося процесса
			for(auto & broker : j->second){
				// Если процесс найден
				if((broker->end = (broker->pid == pid))){
					// Выполняем остановку чтение сообщений
					broker->mess.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(broker->cfds[0]);
					::close(broker->mfds[1]);
					// Если статус сигнала, ручной остановкой процесса
					if(status == SIGINT){
						// Выполняем остановку работы
						const_cast <cluster_t *> (this->_ctx)->stop(this->_wid);
						// Выходим из приложения
						::exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					} else if((this->_ctx->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS) - broker->date) <= 180000) {
						// Выполняем остановку работы
						const_cast <cluster_t *> (this->_ctx)->stop(this->_wid);
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					// this->_log->print("Child process stopped, PID=%d, STATUS=%x", log_t::flag_t::CRITICAL, broker->pid, status);
					// Если функция обратного вызова установлена
					if(this->_ctx->_callbacks.is("process"))
						// Выполняем функцию обратного вызова
						this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", j->first, pid, event_t::STOP);
					// Выполняем поиск воркера
					auto i = this->_ctx->_workers.find(j->first);
					// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
					if((i != this->_ctx->_workers.end()) && i->second->restart){
						// Получаем индекс упавшего процесса
						const uint16_t index = this->_ctx->_pids.at(broker->pid);
						// Удаляем процесс из списка процессов
						const_cast <cluster_t *> (this->_ctx)->_pids.erase(broker->pid);
						// Выполняем создание нового процесса
						const_cast <cluster_t *> (this->_ctx)->fork(i->first, index, i->second->restart);
					// Просто удаляем процесс из списка процессов
					} else const_cast <cluster_t *> (this->_ctx)->_pids.erase(broker->pid);
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
					const_cast <cluster_t *> (worker->_ctx)->stop(worker->_wid);
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
		if(this->_ctx->_pid == static_cast <pid_t> (::getpid())){
			// Определяем тип события
			switch(static_cast <uint8_t> (event)){
				// Если выполняется событие закрытие подключения
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from child process [%u] is closed", log_t::flag_t::CRITICAL, this->_ctx->_pid, ::getpid());
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
					auto i = this->_ctx->_brokers.find(this->_wid);
					// Если текущий брокер найден
					if(i != this->_ctx->_brokers.end()){
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
					const ssize_t bytes = ::read(fd, this->_buffer, sizeof(this->_buffer));
					// Если сообщение получено полностью
					if(bytes != 0){
						// Если данные прочитанны правильно
						if(bytes > 0){
							// Если функция обратного вызова установлена
							if(this->_ctx->_callbacks.is("message")){
								// Объект работы с протоколом передачи данных
								cmp_t * cmp = nullptr;
								// Выполняем поиск объекта работы с протоколом передачи данных
								auto i = this->_cmp.find(pid);
								// Если объект работы с протоколом передачи данных найден
								if(i != this->_cmp.end())
									// Выполняем извлечение объекта работы с протоколом передачи данных
									cmp = i->second.get();
								// Создаём новый объект протокола передачи данных
								else cmp = this->_cmp.emplace(pid, std::unique_ptr <cmp_t> (new cmp_t(this->_log))).first->second.get();
								// Выполняем добавление бинарных данных в протокол
								cmp->append(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
								// Выполняем извлечение записей
								for(auto & index : cmp->items()){
									// Получаем буфер бинарных данных
									const auto & buffer = cmp->at(index);
									// Выполняем функцию обратного вызова
									this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, pid, buffer.data(), buffer.size());
									// Выполняем удаление указанной записи
									cmp->erase(index);
								}
							}
						}
					// Если данные не прочитаны
					} else {
						// Выводим сообщение об ошибке в лог
						this->_log->print("[%u] Data from child process [%u] could not be received", log_t::flag_t::CRITICAL, this->_ctx->_pid, pid);
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
		} else if(this->_ctx->_pid == static_cast <pid_t> (::getppid())) {
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
					auto i = this->_ctx->_brokers.find(this->_wid);
					// Если текущий брокер найден
					if(i != this->_ctx->_brokers.end()){
						// Получаем индекс текущего процесса
						const uint16_t index = this->_ctx->_pids.at(::getpid());
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
									// Закрываем файловый дескриптор на чтение из дочернего процесса
									::close(item->cfds[0]);
									// Закрываем файловый дескриптор на запись в основной процесс
									::close(item->mfds[1]);
								}
							}
							// Выполняем остановку чтение сообщений
							broker->mess.stop();
							// Выходим из функции
							return;
						}
						// Размер входящих сообщений
						const int32_t bytes = ::read(fd, this->_buffer, sizeof(this->_buffer));
						// Если сообщение получено полностью
						if(bytes != 0){
							// Если данные прочитанны правильно
							if(bytes > 0){
								// Если функция обратного вызова установлена
								if(this->_ctx->_callbacks.is("message")){
									// Объект работы с протоколом передачи данных
									cmp_t * cmp = nullptr;
									// Выполняем поиск объекта работы с протоколом передачи данных
									auto i = this->_cmp.find(::getppid());
									// Если объект работы с протоколом передачи данных найден
									if(i != this->_cmp.end())
										// Выполняем извлечение объекта работы с протоколом передачи данных
										cmp = i->second.get();
									// Создаём новый объект протокола передачи данных
									else cmp = this->_cmp.emplace(::getppid(), std::unique_ptr <cmp_t> (new cmp_t(this->_log))).first->second.get();
									// Выполняем добавление бинарных данных в протокол
									cmp->append(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
									// Выполняем извлечение записей
									for(auto & index : cmp->items()){
										// Получаем буфер бинарных данных
										const auto & buffer = cmp->at(index);
										// Выполняем функцию обратного вызова
										this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, this->_ctx->_pid, buffer.data(), buffer.size());
										// Выполняем удаление указанной записи
										cmp->erase(index);
									}
								}
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
			const_cast <cluster_t *> (this->_ctx)->stop(this->_wid);
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
/**
 * write Метод записи буфера данных в сокет
 * @param wid идентификатор воркера
 * @param fd  идентификатор файлового дескриптора
 */
void awh::Cluster::write(const uint16_t wid, const SOCKET fd) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск активных протоколов кластера
			auto i = this->_cmp.find(wid);
			// Если протокол кластера найден
			if(i != this->_cmp.end()){
				// Выполняем перебор всех чанков протокола
				while(!i->second->empty()){
					// Смещение в бинарном буфере
					size_t offset = 0;
					// Получаем бинарный буфер данных
					const auto & buffer = i->second->front();
					// Устанавливаем метку записи данных
					Send:
					// Выполняем запись в сокет
					const ssize_t bytes = ::write(fd, buffer.data() + offset, buffer.size() - offset);
					// Если данные записаны удачно
					if(bytes == static_cast <ssize_t> (buffer.size()))
						// Удаляем чанк из объекта протокола
						i->second->pop();
					// Если данные отправлены не полностью
					else if((bytes > 0) && (bytes < static_cast <ssize_t> (buffer.size()))) {
						// Увеличиваем смещение на количество отправленных данных
						offset += static_cast <size_t> (bytes);
						// Выполняем попытку отправить данные ещё раз
						goto Send;
					// Если мы поймали ошибку
					} else if(bytes < 0) {
						// Если защищённый режим работы запрещён
						if((AWH_ERROR() == EWOULDBLOCK) || (AWH_ERROR() == EINTR))
							// Выполняем попытку отправить данные ещё раз
							goto Send;
						// Выводим в лог сообщение
						else {
							// Выводим в лог сообщение
							this->_log->print("Cluster write: %s", log_t::flag_t::WARNING, this->_socket.message().c_str());
							// Выходим из цикла
							break;
						}
					// Если произошло отключение
					} else {
						// Выполняем остановку работы
						this->stop(wid);
						// Выводим в лог сообщение
						this->_log->print("Cluster write: %s", log_t::flag_t::WARNING, this->_socket.message().c_str());
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
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Выводим в лог сообщение
			this->_log->print("Write message to process", log_t::flag_t::CRITICAL);
		}
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
		/**
		 * Выполняем обработку ошибки
		 */
		try {
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
							this->_brokers.emplace(i->first, vector <std::unique_ptr <broker_t>> ());
							// Выполняем поиск брокера
							j = this->_brokers.find(i->first);
						}
						// Выполняем создание указанное количество брокеров
						for(size_t index = 0; index < i->second->count; index++){
							// Создаём объект брокера
							std::unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
							// Выполняем подписку на основной канал передачи данных
							if(::pipe(broker->mfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
								// Выходим принудительно из приложения
								::exit(EXIT_FAILURE);
							}
							// Выполняем подписку на дочерний канал передачи данных
							if(::pipe(broker->cfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
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
						std::unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
						// Выполняем подписку на основной канал передачи данных
						if(::pipe(broker->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("Cluster fork: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
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
							this->_log->print("Cluster fork: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
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
									// Выполняем активацию работы события чтения данных с сокета
									broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
									// выполняем активацию работы события закрытия подключения
									broker->mess.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
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
							// Если не нужно останавливаться на создании процессов
							if(!stop)
								// Выполняем создание новых процессов
								this->fork(i->first, index + 1, stop);
							// Если нужно остановиться после создания всех прцоессов
							else {
								// Выполняем активацию работы чтения данных с дочерних процессов
								broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
								// выполняем активацию работы события закрытия подключения
								broker->mess.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
							}
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
						for(auto & broker : j->second){
							// Выполняем активацию работы чтения данных с дочерних процессов
							broker->mess.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
							// выполняем активацию работы события закрытия подключения
							broker->mess.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
						}
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("process"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, this->_pid, event_t::START);
					}
				}
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
 * @param wid идентификатор воркера
 */
void awh::Cluster::send(const uint16_t wid) noexcept {
	// Флаг отправки сообщения
	const bool message = true;
	// Выполняем отправку сооббщения
	this->send(wid, reinterpret_cast <const char *> (&message), sizeof(message));
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
				// Выполняем поиск активных протоколов кластера
				auto j = this->_cmp.find(i->first);
				// Если протокол кластера найден
				if(j != this->_cmp.end()){
					// Выполняем добавление буфера данных в протокол
					j->second->push(buffer, size);
					// Выполняем отправку сообщения мастер-процессу
					this->write(i->first, i->second.at(this->_pids.at(pid))->mfds[1]);
				}
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
 * @param wid идентификатор воркера
 * @param pid идентификатор процесса для получения сообщения
 */
void awh::Cluster::send(const uint16_t wid, const pid_t pid) noexcept {
	// Флаг отправки сообщения
	const bool message = true;
	// Выполняем отправку сооббщения
	this->send(wid, pid, reinterpret_cast <const char *> (&message), sizeof(message));
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
				// Выполняем поиск активных протоколов кластера
				auto j = this->_cmp.find(i->first);
				// Если протокол кластера найден
				if(j != this->_cmp.end()){
					// Выполняем добавление буфера данных в протокол
					j->second->push(buffer, size);
					// Выполняем отправку сообщения дочернему-процессу
					this->write(i->first, i->second.at(this->_pids.at(pid))->cfds[1]);
				}
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
 * @param wid идентификатор воркера
 */
void awh::Cluster::broadcast(const uint16_t wid) noexcept {
	// Флаг отправки сообщения
	const bool message = true;
	// Выполняем отправку сооббщения
	this->broadcast(wid, reinterpret_cast <const char *> (&message), sizeof(message));
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
				// Выполняем поиск активных протоколов кластера
				auto j = this->_cmp.find(i->first);
				// Если протокол кластера найден
				if(j != this->_cmp.end()){
					// Переходим по всем дочерним процессам
					for(auto & broker : i->second){
						// Если идентификатор процесса не нулевой
						if(broker->pid > 0){
							// Выполняем добавление буфера данных в протокол
							j->second->push(buffer, size);
							// Выполняем отправку сообщения дочернему-процессу
							this->write(i->first, broker->cfds[1]);
						}
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
		map <uint16_t, vector <std::unique_ptr <broker_t>>> ().swap(this->_brokers);
	}
	// Выполняем очистку протокола передачи данных
	this->_cmp.clear();
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем освобождение выделенной памяти
	map <uint16_t, std::unique_ptr <worker_t>> ().swap(this->_workers);
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
					// Выполняем закрытие файловых дескрипторов
					::close(broker->cfds[0]);
					::close(broker->mfds[1]);
				}
			#endif
			// Очищаем список брокеров
			item.second.clear();
		}
	}
	// Если список активных протоколов кластера существует
	if(!this->_cmp.empty()){
		// Выполняем перебор всего списка протоколов кластера
		for(auto & item : this->_cmp)
			// Выполняем очистку протокола передачи данных
			item.second->clear();
	}
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
				// Выполняем закрытие файловых дескрипторов
				::close(broker->cfds[0]);
				::close(broker->mfds[1]);
			}
		#endif
		// Очищаем список брокеров
		i->second.clear();
	}
	// Выполняем поиск активных протоколов кластера
	auto j = this->_cmp.find(wid);
	// Если протокол кластера найден
	if(j != this->_cmp.end())
		// Выполняем удаление активного протокола кластера
		j->second->clear();
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
 * init Метод инициализации воркера
 * @param wid   идентификатор воркера
 * @param count максимальное количество процессов
 */
void awh::Cluster::init(const uint16_t wid, const uint16_t count) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем поиск идентификатора воркера
		auto i = this->_workers.find(wid);
		// Если воркер не найден
		if(i == this->_workers.end())
			// Добавляем воркер в список воркеров
			this->_workers.emplace(wid, std::unique_ptr <worker_t> (new worker_t(wid, this, this->_log)));
		// Выполняем поиск активных протоколов кластера
		auto j = this->_cmp.find(wid);
		// Если протокол кластера не найден
		if(j == this->_cmp.end())
			// Добавляем новый протокол кластера
			this->_cmp.emplace(wid, std::unique_ptr <cmp_t> (new cmp_t(this->_log)));
		// Выполняем установку максимально-возможного количества процессов
		this->count(wid, count);
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
