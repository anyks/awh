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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <sys/cluster.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
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
					// Идентификатор процесса приславший сообщение
					pid_t pid = 0;
					// Выполняем поиск текущего брокера
					auto i = const_cast <cluster_t *> (this->_ctx)->_brokers.find(this->_wid);
					// Если текущий брокер найден
					if(i != const_cast <cluster_t *> (this->_ctx)->_brokers.end()){
						// Переходим по всему списку брокеров
						for(auto j = i->second.begin(); j != i->second.end(); ++j){
							// Если файловый дескриптор соответствует
							if(static_cast <SOCKET> ((* j)->mfds[0]) == fd){
								// Получаем идентификатор процесса приславшего сообщение
								pid = (* j)->pid;
								// Выполняем остановку чтение сообщений
								(* j)->ev.stop();
								// Выполняем закрытие файловых дескрипторов
								::close((* j)->mfds[0]);
								::close((* j)->cfds[1]);
								// Выполняем удаление указанного брокера
								i->second.erase(j);
								// Выходим из цикла
								break;
							}
						}
					}
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from child process [%u] is closed", log_t::flag_t::CRITICAL, this->_ctx->_pid, pid);
					// Если установлен флаг аннигиляции
					if(!this->_restart){
						// Выполняем остановку работы
						const_cast <cluster_t *> (this->_ctx)->clear();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					// Выходим из функции
					} else return;
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
								// Выполняем поиск объекта работы с протоколом передачи данных
								auto i = this->_cmp.find(pid);
								// Если объект работы с протоколом передачи данных найден
								if(i != this->_cmp.end()){
									// Выполняем добавление бинарных данных в протокол
									i->second->push(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
									// Выполняем извлечение записей
									while(!i->second->empty()){
										// Получаем буфер бинарных данных
										const auto & buffer = i->second->get();
										// Если буфер данных получен
										if((buffer.first != nullptr) && (buffer.second > 0))
											// Выполняем функцию обратного вызова
											this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, pid, reinterpret_cast <const char *> (buffer.first), buffer.second);
										// Выводим значение по умолчанию
										else this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, pid, nullptr, 0);
										// Выполняем удаление указанной записи
										i->second->pop();
									}
								}
							}
						}
					// Если данные не прочитаны
					} else {
						// Выводим сообщение об ошибке в лог
						this->_log->print("[%u] Data from child process [%u] could not be received", log_t::flag_t::CRITICAL, this->_ctx->_pid, pid);
						// Выходим из функции
						return;
					}
				} break;
			}
		// Если процесс является дочерним
		} else if(this->_ctx->_pid == static_cast <pid_t> (::getppid())) {
			// Определяем тип события
			switch(static_cast <uint8_t> (event)){
				// Если выполняется событие закрытие подключения
				case static_cast <uint8_t> (base_t::event_type_t::CLOSE): {
					// Выполняем поиск текущего брокера
					auto i = this->_ctx->_brokers.find(this->_wid);
					// Если текущий брокер найден
					if(i != this->_ctx->_brokers.end()){
						// Получаем индекс текущего процесса
						const uint16_t index = this->_ctx->_pids.at(::getpid());
						// Получаем объект текущего брокера
						broker_t * broker = i->second.at(index).get();
						// Выполняем остановку чтение сообщений
						broker->ev.stop();
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(broker->cfds[0]);
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(broker->mfds[1]);
					}
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from main process is closed", log_t::flag_t::CRITICAL, ::getpid());
					// Останавливаем чтение данных с родительского процесса
					const_cast <cluster_t *> (this->_ctx)->stop(this->_wid);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
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
									item->ev.stop();
									// Закрываем файловый дескриптор на чтение из дочернего процесса
									::close(item->cfds[0]);
									// Закрываем файловый дескриптор на запись в основной процесс
									::close(item->mfds[1]);
								}
							}
							// Выполняем остановку чтение сообщений
							broker->ev.stop();
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
									// Выполняем поиск объекта работы с протоколом передачи данных
									auto i = this->_cmp.find(::getppid());
									// Если объект работы с протоколом передачи данных найден
									if(i != this->_cmp.end()){
										// Выполняем добавление бинарных данных в протокол
										i->second->push(reinterpret_cast <char *> (this->_buffer), static_cast <size_t> (bytes));
										// Выполняем извлечение записей
										while(!i->second->empty()){
											// Получаем буфер бинарных данных
											const auto & buffer = i->second->get();
											// Если буфер данных получен
											if((buffer.first != nullptr) && (buffer.second > 0))
												// Выполняем функцию обратного вызова
												this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, this->_ctx->_pid, reinterpret_cast <const char *> (buffer.first), buffer.second);
											// Выводим значение по умолчанию
											else this->_ctx->_callbacks.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, this->_ctx->_pid, nullptr, 0);
											// Выполняем удаление указанной записи
											i->second->pop();
										}
									}
								}
							}
						// Если данные не прочитаны
						} else {
							// Выводим сообщение об ошибке в лог
							this->_log->print("[%u] Data from main process could not be received", log_t::flag_t::CRITICAL, ::getpid());
							// Выходим из функции
							return;
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
#endif
/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Глобальный объект воркера
	 */
	static awh::cluster_t * cluster = nullptr;
	/**
	 * process Метод перезапуска упавшего процесса
	 * @param pid    идентификатор упавшего процесса
	 * @param status статус остановившегося процесса
	 */
	void awh::Cluster::process(const pid_t pid, const int32_t status) noexcept {
		// Выполняем перебор всего списка процессов
		for(auto & item : this->_brokers){
			// Выполняем поиск завершившегося процесса
			for(auto & broker : item.second){
				// Если процесс найден
				if((broker->end = (broker->pid == pid))){
					// Выполняем остановку чтение сообщений
					broker->ev.stop();
					// Выполняем закрытие файловых дескрипторов
					::close(broker->mfds[0]);
					::close(broker->cfds[1]);
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					this->_log->print("Child process stopped, PID=%d, STATUS=%d", log_t::flag_t::WARNING, broker->pid, status);
					// Если статус сигнала, ручной остановкой процесса
					if(status == SIGINT){
						// Выполняем остановку работы
						this->clear();
						// Выходим из приложения
						::exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					} else if((this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS) - broker->date) <= 180000) {
						// Выполняем остановку работы
						this->clear();
						// Выходим из приложения
						::exit(status);
					}
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("exit"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint16_t, const pid_t, const int32_t)> ("exit", item.first, pid, status);
					// Если функция обратного вызова установлена
					if(this->_callbacks.is("process"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", item.first, pid, event_t::STOP);
					// Выполняем поиск воркера
					auto i = this->_workers.find(item.first);
					// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
					if((i != this->_workers.end()) && i->second->_restart){
						// Удаляем процесс из списка процессов
						this->_pids.erase(broker->pid);
						// Если процесс завершился не сам, перезапускаем его
						if(status > 0)
							// Выполняем создание нового процесса
							this->emplace(item.first, pid);
					// Просто удаляем процесс из списка процессов
					} else {
						// Выполняем остановку работы
						this->clear();
						// Выполняем завершение работы
						::exit(status);
					}
					// Выходим функции
					return;
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
	void awh::Cluster::child([[maybe_unused]] int32_t signal, [[maybe_unused]] siginfo_t * info, [[maybe_unused]] void * ctx) noexcept {
		// Идентификатор упавшего процесса
		pid_t pid = 0;
		// Статус упавшего процесса
		int32_t status = 0;
		// Выполняем получение идентификатора упавшего процесса
		while((pid = ::waitpid(-1, &status, WNOHANG)) > 0)
			// Выполняем создание дочернего потока
			const_cast <cluster_t *> (cluster)->process(pid, status);
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
					// Получаем бинарный буфер данных
					const auto & buffer = i->second->get();
					// Если данных в буфере нету, выходим из цикла
					if((buffer.first == nullptr) || (buffer.second == 0)){
						// Процесс превратился в зомби, самоликвидируем его
						this->_log->print("Cluster message sending stack was broken for process [%u]", log_t::flag_t::WARNING, ::getpid());
						// Выходим из цикла
						break;
					}
					// Устанавливаем метку отправки данных
					Send:
					// Выполняем запись в сокет
					const ssize_t bytes = ::write(fd, reinterpret_cast <const char *> (buffer.first), buffer.second);
					// Если данные доставлены успешно
					if(bytes > 0)
						// Удаляем чанк из объекта протокола
						i->second->pop();
					// Если мы поймали ошибку
					else if(bytes < 0) {
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
						// Выводим в лог сообщение
						this->_log->print("Cluster write: %s", log_t::flag_t::WARNING, this->_socket.message().c_str());
						// Выполняем остановку работы
						this->stop(wid);
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(wid, fd), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	#endif
}
/**
 * emplace Метод размещения нового дочернего процесса
 * @param wid идентификатор воркера
 * @param pid идентификатор предыдущего процесса
 */
void awh::Cluster::emplace(const uint16_t wid, const pid_t pid) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Запоминаем идентификатор предыдущего процесса
			const pid_t opid = pid;
			// Выполняем поиск воркера
			auto i = this->_workers.find(wid);
			// Если воркер найден
			if(i != this->_workers.end()){
				// Выполняем поиск брокера
				auto j = this->_brokers.find(i->first);
				// Если список брокеров ещё пустой
				if(j != this->_brokers.end()){
					// Создаём объект брокера
					unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
					// Выполняем подписку на основной канал передачи данных
					if(::pipe(broker->mfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						// Выполняем остановку работы
						this->clear();
						// Выходим принудительно из приложения
						::exit(EXIT_FAILURE);
					}
					// Выполняем подписку на дочерний канал передачи данных
					if(::pipe(broker->cfds) != 0){
						// Выводим в лог сообщение
						this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						// Выполняем остановку работы
						this->clear();
						// Выходим принудительно из приложения
						::exit(EXIT_FAILURE);
					}
					// Выполняем добавление брокера в список брокеров
					j->second.push_back(::move(broker));
				}
				// Устанавливаем идентификатор процесса
				pid_t pid = -1;
				// Определяем тип потока
				switch((pid = ::fork())){
					// Если поток не создан
					case -1: {
						// Выводим в лог сообщение
						this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
						// Выполняем остановку работы
						this->clear();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					} break;
					// Если процесс является дочерним
					case 0: {
						// Если идентификатор процесса соответствует
						if((i->second->_working = (this->_pid == static_cast <pid_t> (::getppid())))){
							// Получаем идентификатор текущего процесса
							const pid_t pid = ::getpid();
							// Добавляем в список дочерних процессов, идентификатор процесса
							this->_pids.emplace(pid, j->second.size() - 1);
							{
								// Выполняем переинициализацию базы событий
								this->_core->reinit();
								// Получаем объект текущего брокера
								broker_t * broker = j->second.back().get();
								// Закрываем файловый дескриптор на запись в дочерний процесс
								::close(broker->cfds[1]);
								// Закрываем файловый дескриптор на чтение из основного процесса
								::close(broker->mfds[0]);
								// Делаем сокет на чтение неблокирующим
								this->_socket.blocking(broker->cfds[0], socket_t::mode_t::DISABLED);
								// Делаем сокет на запись неблокирующим
								this->_socket.blocking(broker->mfds[1], socket_t::mode_t::ENABLED);
								// Устанавливаем идентификатор процесса
								broker->pid = pid;
								// Устанавливаем время начала жизни процесса
								broker->date = this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS);
								// Устанавливаем базу событий для чтения
								broker->ev = this->_core->eventBase();
								// Устанавливаем сокет для чтения
								broker->ev = broker->cfds[0];
								// Устанавливаем событие на чтение данных от основного процесса
								broker->ev = std::bind(&worker_t::message, i->second.get(), _1, _2);
								// Запускаем чтение данных с основного процесса
								broker->ev.start();
								// Выполняем активацию работы события чтения данных с сокета
								broker->ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
								// выполняем активацию работы события закрытия подключения
								broker->ev.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
								// Создаём новый объект протокола передачи данных
								this->_cmp.emplace(wid, unique_ptr <cmp::encoder_t> (new cmp::encoder_t(this->_log)));
								// Создаём новый объект протокола получения данных
								i->second->_cmp.emplace(this->_pid, unique_ptr <cmp::decoder_t> (new cmp::decoder_t(this->_log)));
								// Если функция обратного вызова установлена
								if(this->_callbacks.is("process"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
							}
						// Если процесс превратился в зомби
						} else {
							// Процесс превратился в зомби, самоликвидируем его
							this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
							// Выполняем остановку работы
							this->stop(wid);
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
					} break;
					// Если процесс является родительским
					default: {
						// Добавляем в список дочерних процессов, идентификатор процесса
						this->_pids.emplace(pid, j->second.size() - 1);
						// Получаем объект текущего брокера
						broker_t * broker = j->second.back().get();
						// Закрываем файловый дескриптор на запись в основной процесс
						::close(broker->mfds[1]);
						// Закрываем файловый дескриптор на чтение из дочернего процесса
						::close(broker->cfds[0]);
						// Делаем сокет на чтение неблокирующим
						this->_socket.blocking(broker->mfds[0], socket_t::mode_t::DISABLED);
						// Делаем сокет на запись неблокирующим
						this->_socket.blocking(broker->cfds[1], socket_t::mode_t::ENABLED);
						// Устанавливаем PID-процесса
						broker->pid = pid;
						// Устанавливаем время начала жизни процесса
						broker->date = this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS);
						// Устанавливаем базу событий для чтения
						broker->ev = this->_core->eventBase();
						// Устанавливаем сокет для чтения
						broker->ev = broker->mfds[0];
						// Устанавливаем событие на чтение данных от дочернего процесса
						broker->ev = std::bind(&worker_t::message, i->second.get(), _1, _2);
						// Выполняем запуск работы чтения данных с дочерних процессов
						broker->ev.start();
						// Выполняем активацию работы чтения данных с дочерних процессов
						broker->ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
						// выполняем активацию работы события закрытия подключения
						broker->ev.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
						// Создаём новый объект протокола получения данных
						i->second->_cmp.emplace(pid, unique_ptr <cmp::decoder_t> (new cmp::decoder_t(this->_log)));
						// Если функция обратного вызова установлена
						if(this->_callbacks.is("rebase") && (opid > 0))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const uint16_t, const pid_t, const pid_t)> ("rebase", i->first, pid, opid);
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(wid, pid), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	#endif
}
/**
 * create Метод создания дочерних процессов при запуске кластера
 * @param wid   идентификатор воркера
 * @param index индекс инициализированного процесса
 */
void awh::Cluster::create(const uint16_t wid, const uint16_t index) noexcept {
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
				if(index < i->second->_count){
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
						for(size_t index = 0; index < i->second->_count; index++){
							// Создаём объект брокера
							unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
							// Выполняем подписку на основной канал передачи данных
							if(::pipe(broker->mfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
								// Выполняем остановку работы
								this->clear();
								// Выходим принудительно из приложения
								::exit(EXIT_FAILURE);
							}
							// Выполняем подписку на дочерний канал передачи данных
							if(::pipe(broker->cfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster fork child: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
								// Выполняем остановку работы
								this->clear();
								// Выходим принудительно из приложения
								::exit(EXIT_FAILURE);
							}
							// Выполняем добавление брокера в список брокеров
							j->second.push_back(::move(broker));
						}
					}
					// Если процесс завершил свою работу
					if(j->second.at(index)->end){
						// Создаём объект брокера
						unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
						// Выполняем подписку на основной канал передачи данных
						if(::pipe(broker->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("Cluster fork: %s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
							// Выполняем поиск завершившегося процесса
							for(auto & broker : j->second)
								// Выполняем остановку чтение сообщений
								broker->ev.stop();
							// Выполняем остановку работы
							this->clear();
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
								broker->ev.stop();
							// Выполняем остановку работы
							this->clear();
							// Выходим принудительно из приложения
							::exit(EXIT_FAILURE);
						}
						// Устанавливаем нового брокера
						j->second.at(index) = ::move(broker);
					}
					// Устанавливаем идентификатор процесса
					pid_t pid = -1;
					// Определяем тип потока
					switch((pid = ::fork())){
						// Если поток не создан
						case -1: {
							// Выводим в лог сообщение
							this->_log->print("Child process could not be created", log_t::flag_t::CRITICAL);
							// Выполняем остановку работы
							this->clear();
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						} break;
						// Если процесс является дочерним
						case 0: {
							// Если идентификатор процесса соответствует
							if((i->second->_working = (this->_pid == static_cast <pid_t> (::getppid())))){
								// Получаем идентификатор текущего процесса
								const pid_t pid = ::getpid();
								// Добавляем в список дочерних процессов, идентификатор процесса
								this->_pids.emplace(pid, index);
								{
									// Выполняем переинициализацию базы событий
									this->_core->reinit();
									// Получаем объект текущего брокера
									broker_t * broker = j->second.at(index).get();
									// Закрываем файловый дескриптор на запись в дочерний процесс
									::close(broker->cfds[1]);
									// Закрываем файловый дескриптор на чтение из основного процесса
									::close(broker->mfds[0]);
									// Выполняем перебор всего списка брокеров
									for(auto & item : j->second){
										// Если найдены остальные брокеры
										if(item->cfds[1] != broker->cfds[1]){
											// Закрываем файловый дескриптор на запись в дочерний процесс
											::close(item->cfds[0]);
											::close(item->cfds[1]);
											// Закрываем файловый дескриптор на чтение из основного процесса
											::close(item->mfds[0]);
											::close(item->mfds[1]);
										}
									}
									// Делаем сокет на чтение неблокирующим
									this->_socket.blocking(broker->cfds[0], socket_t::mode_t::DISABLED);
									// Делаем сокет на запись неблокирующим
									this->_socket.blocking(broker->mfds[1], socket_t::mode_t::ENABLED);
									// Устанавливаем идентификатор процесса
									broker->pid = pid;
									// Устанавливаем время начала жизни процесса
									broker->date = this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS);
									// Устанавливаем базу событий для чтения
									broker->ev = this->_core->eventBase();
									// Устанавливаем сокет для чтения
									broker->ev = broker->cfds[0];
									// Устанавливаем событие на чтение данных от основного процесса
									broker->ev = std::bind(&worker_t::message, i->second.get(), _1, _2);
									// Запускаем чтение данных с основного процесса
									broker->ev.start();
									// Выполняем активацию работы события чтения данных с сокета
									broker->ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
									// выполняем активацию работы события закрытия подключения
									broker->ev.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
									// Создаём новый объект протокола передачи данных
									this->_cmp.emplace(wid, unique_ptr <cmp::encoder_t> (new cmp::encoder_t(this->_log)));
									// Создаём новый объект протокола получения данных
									i->second->_cmp.emplace(this->_pid, unique_ptr <cmp::decoder_t> (new cmp::decoder_t(this->_log)));
									// Если функция обратного вызова установлена
									if(this->_callbacks.is("process"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
								}
							// Если процесс превратился в зомби
							} else {
								// Процесс превратился в зомби, самоликвидируем его
								this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
								// Выполняем остановку работы
								this->stop(wid);
								// Выходим из приложения
								::exit(EXIT_FAILURE);
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
							this->_socket.blocking(broker->mfds[0], socket_t::mode_t::DISABLED);
							// Делаем сокет на запись неблокирующим
							this->_socket.blocking(broker->cfds[1], socket_t::mode_t::ENABLED);
							// Устанавливаем PID-процесса
							broker->pid = pid;
							// Устанавливаем время начала жизни процесса
							broker->date = this->_fmk->timestamp(fmk_t::chrono_t::MILLISECONDS);
							// Устанавливаем базу событий для чтения
							broker->ev = this->_core->eventBase();
							// Устанавливаем сокет для чтения
							broker->ev = broker->mfds[0];
							// Устанавливаем событие на чтение данных от дочернего процесса
							broker->ev = std::bind(&worker_t::message, i->second.get(), _1, _2);
							// Выполняем запуск работы чтения данных с дочерних процессов
							broker->ev.start();
							// Выполняем создание новых процессов
							this->create(i->first, index + 1);
							// Создаём новый объект протокола получения данных
							i->second->_cmp.emplace(pid, unique_ptr <cmp::decoder_t> (new cmp::decoder_t(this->_log)));
						}
					}
				// Если все процессы удачно созданы
				} else {
					// Выполняем поиск брокера
					auto j = this->_brokers.find(i->first);
					// Если идентификатор воркера получен
					if((i->second->_working = (j != this->_brokers.end()))){
						// Выполняем перебор всех доступных брокеров
						for(auto & broker : j->second){
							// Выполняем активацию работы чтения данных с дочерних процессов
							broker->ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
							// выполняем активацию работы события закрытия подключения
							broker->ev.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
						}
						// Создаём новый объект протокола передачи данных
						this->_cmp.emplace(wid, unique_ptr <cmp::encoder_t> (new cmp::encoder_t(this->_log)));
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
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(wid, index), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
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
		return i->second->_working;
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем остановку работы
			this->stop(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
	// Выполняем очистку протокола передачи данных
	this->_cmp.clear();
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
					broker->ev.stop();
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
				broker->ev.stop();
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
				restart = i->second->_restart;
				// Снимаем флаг перезапуска процесса
				i->second->_restart = false;
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
				i->second->_restart = restart;
		// Если процесс является дочерним
		} else if(this->_pid == static_cast <pid_t> (::getppid()))
			// Выполняем закрытие подключения передачи сообщений
			this->close(wid);
		// Если процесс превратился в зомби
		else {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем закрытие подключения передачи сообщений
			this->close(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
		// Если воркер найден, снимаем флаг запуска кластера
		if(i != this->_workers.end())
			// Снимаем флаг запуска кластера
			i->second->_working = false;
		// Удаляем список дочерних процессов
		this->_pids.clear();
	}
}
/**
 * start Метод запуска кластера
 * @param wid идентификатор воркера
 */
void awh::Cluster::start(const uint16_t wid) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Выполняем поиск идентификатора воркера
			auto i = this->_workers.find(wid);
			// Если вокер найден
			if((i != this->_workers.end()) && !i->second->_working)
				// Выполняем запуск процесса
				this->create(i->first);
		// Если процесс превратился в зомби
		} else if((this->_pid != ::getpid()) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем остановку работы
			this->stop(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
 * restart Метод установки флага перезапуска процессов
 * @param wid  идентификатор воркера
 * @param mode флаг перезапуска процессов
 */
void awh::Cluster::restart(const uint16_t wid, const bool mode) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Выполняем поиск идентификатора воркера
			auto i = this->_workers.find(wid);
			// Если вокер найден
			if(i != this->_workers.end())
				// Устанавливаем флаг автоматического перезапуска процесса
				i->second->_restart = mode;
		// Если процесс превратился в зомби
		} else if((this->_pid != ::getpid()) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем остановку работы
			this->stop(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
 * base Метод установки сетевого ядра
 * @param core сетевое ядро для установки
 */
void awh::Cluster::core(core_t * core) noexcept {
	// Если все воркеры уже инициализированы
	if(!this->_workers.empty()){
		// Переходим по всем активным воркерам
		for(auto & worker : this->_workers)
			// Выполняем остановку процессов
			this->stop(worker.first);
	}
	// Выполняем установку базы событий
	this->_core = core;
}
/**
 * emplace Метод размещения нового дочернего процесса
 * @param wid идентификатор воркера
 */
void awh::Cluster::emplace(const uint16_t wid) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Выполняем поиск идентификатора воркера
			auto i = this->_workers.find(wid);
			// Если вокер найден и работа кластера запущена
			if((i != this->_workers.end()) && i->second->_working){
				// Увеличиваем количество активных воркеров
				i->second->_count++;
				// Выполняем запуск нового процесса
				this->emplace(i->first, 0);
			}
		// Если процесс превратился в зомби
		} else if((this->_pid != ::getpid()) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем остановку работы
			this->stop(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
 * erase Метод удаления активного процесса
 * @param wid идентификатор воркера
 * @param pid идентификатор процесса
 */
void awh::Cluster::erase(const uint16_t wid, const pid_t pid) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Выполняем поиск брокеров
			auto i = this->_brokers.find(wid);
			// Если брокер найден и работа кластера запущена
			if(i != this->_brokers.end()){
				// Выполняем поиск индекса брокера
				auto j = this->_pids.find(pid);
				// Если индекс найден
				if((j != this->_pids.end()) && (static_cast <size_t> (j->second) < i->second.size())){
					// Получаем объект текущего брокера
					broker_t * broker = i->second.at(j->second).get();
					// Выполняем остановку чтение сообщений
					broker->ev.stop();
					// Выполняем закрытие открытых файловых дескрипторов
					::close(broker->mfds[0]);
					::close(broker->cfds[1]);
					// Выполняем удаление указанного брокера
					i->second.erase(next(i->second.begin(), j->second));
					// Выполняем удаление процесса из списка
					this->_pids.erase(j);
					// Выполняем убийство процесса
					::kill(pid, SIGTERM);
				}
			}
		// Если процесс превратился в зомби
		} else if((this->_pid != static_cast <pid_t> (::getpid())) && (this->_pid != static_cast <pid_t> (::getppid()))) {
			// Процесс превратился в зомби, самоликвидируем его
			this->_log->print("Process [%u] has turned into a zombie, we perform self-destruction", log_t::flag_t::CRITICAL, ::getpid());
			// Выполняем остановку работы
			this->stop(wid);
			// Выходим из приложения
			::exit(EXIT_FAILURE);
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
		return i->second->_count;
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
			i->second->_count = (thread::hardware_concurrency() / 2);
		// Устанавливаем максимальное количество процессов
		else i->second->_count = count;
		// Если количество процессов не установлено
		if(i->second->_count == 0)
			// Устанавливаем один рабочий процесс
			i->second->_count = 1;
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
			this->_workers.emplace(wid, unique_ptr <worker_t> (new worker_t(wid, this, this->_log)));
		// Выполняем установку максимально-возможного количества процессов
		this->count(wid, count);
	/**
	 * Если возникает ошибка
	 */
	} catch(const bad_alloc &) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(wid, count), log_t::flag_t::CRITICAL, "Memory allocation error");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::Cluster::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова при завершении работы процесса
	this->_callbacks.set("exit", callbacks);
	// Выполняем установку функции обратного вызова при пересоздании процесса
	this->_callbacks.set("rebase", callbacks);
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
	this->_callbacks.set("process", callbacks);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callbacks.set("message", callbacks);
}
/**
 * Cluster Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Cluster::Cluster(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _callbacks(log), _socket(fmk, log), _core(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку объекта кластера
		cluster = this;
		// Выполняем зануление структур перехватчиков событий
		::memset(&this->_sa, 0, sizeof(this->_sa));
		// Устанавливаем функцию перехвадчика событий
		this->_sa.sa_sigaction = cluster_t::child;
		// Устанавливаем флаги перехвата сигналов
		this->_sa.sa_flags = SA_RESTART | SA_SIGINFO;
		// Устанавливаем маску перехвата
		sigemptyset(&this->_sa.sa_mask);
		// Активируем перехватчик событий
		::sigaction(SIGCHLD, &this->_sa, nullptr);
	#endif
}
/**
 * Cluster Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Cluster::Cluster(core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _callbacks(log), _socket(fmk, log), _core(core), _fmk(fmk), _log(log) {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку объекта кластера
		cluster = this;
		// Выполняем зануление структур перехватчиков событий
		::memset(&this->_sa, 0, sizeof(this->_sa));
		// Устанавливаем функцию перехвадчика событий
		this->_sa.sa_sigaction = cluster_t::child;
		// Устанавливаем флаги перехвата сигналов
		this->_sa.sa_flags = SA_RESTART | SA_SIGINFO;
		// Устанавливаем маску перехвата
		sigemptyset(&this->_sa.sa_mask);
		// Активируем перехватчик событий
		::sigaction(SIGCHLD, &this->_sa, nullptr);
	#endif
}
/**
 * ~Cluster Деструктор
 */
awh::Cluster::~Cluster() noexcept {
	// Если активные брокеры присутствуют в кластере
	if(!this->_brokers.empty()){
		// Список активных брокеров
		vector <uint16_t> brokers;
		// Выполняем перебор всех активных брокеров
		for(auto & item : this->_brokers)
			// Выполняем добавление идентификатора брокера в список
			brokers.push_back(item.first);
		// Выполняем перебор всего списка брокеров
		for(auto & broker : brokers)
			// Останавливаем работу кластера
			this->stop(broker);
	}
}
