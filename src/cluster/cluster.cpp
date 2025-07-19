/**
 * @file: cluster.cpp
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
#include <cluster/cluster.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * message Метод обратного вызова получении сообщений
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
					// Определяем принцип передачи данных
					switch(static_cast <uint8_t> (this->_ctx->_transfer)){
						// Если мы передаём данные через unix-сокет
						case static_cast <uint8_t> (transfer_t::IPC): {
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
										// Выполняем удаление указанного брокера
										i->second.erase(j);
										// Выходим из цикла
										break;
									}
								}
							}
							// Ищем сокет в списке подключений
							auto j = this->_ctx->_clients.find(fd);
							// Если сокет есть в списке подключений
							if(j != this->_ctx->_clients.end()){
								// Выполняем остановку получения событий
								j->second->ev.stop();
								// Закрываем сокет подключения
								const_cast <cluster_t *> (this->_ctx)->close(j->second->wid, j->second->fd);
								// Выполняем удаление клиента
								const_cast <cluster_t *> (this->_ctx)->_clients.erase(j);
							}
						} break;
						// Если мы передаём данные через Shared memory
						case static_cast <uint8_t> (transfer_t::PIPE): {
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
										(* j)->read.stop();
										// Выполняем остановку отправки сообщений
										(* j)->write.stop();
										// Выполняем закрытие файловых дескрипторов
										const_cast <cluster_t *> (this->_ctx)->close(i->first, (* j)->mfds[0]);
										const_cast <cluster_t *> (this->_ctx)->close(i->first, (* j)->cfds[1]);
										// Выполняем удаление указанного брокера
										i->second.erase(j);
										// Выходим из цикла
										break;
									}
								}
							}
						} break;
					}
					// Закрываем сокет подключения
					const_cast <cluster_t *> (this->_ctx)->close(this->_wid, fd);
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from child process [%u] is closed", log_t::flag_t::CRITICAL, this->_ctx->_pid, pid);
					// Если установлен флаг аннигиляции
					if(!this->_autoRestart){
						// Выполняем остановку работы
						const_cast <cluster_t *> (this->_ctx)->clear();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					// Выходим из функции
					} else return;
				} break;
				// Если выполняется событие готовности сокета на запись
				case static_cast <uint8_t> (base_t::event_type_t::WRITE): {
					// Выполняем поиск активного процесса
					auto i = this->_ctx->_sockets.find(fd);
					// Если активный процесс привязанный к сокету найден
					if(i != this->_ctx->_sockets.end())
						// Выполняем запуск функции обратного вызова
						const_cast <cluster_t *> (this->_ctx)->sending(this->_wid, i->second, fd);
				} break;
				// Если выполняется событие чтения данных с сокета
				case static_cast <uint8_t> (base_t::event_type_t::READ): {
					// Если буфер данных её не инициализирован
					if(this->_buffer.size == 0){
						// Извлекаем размер сформированного буфера
						this->_buffer.size = this->_ctx->_server.socket.bufferSize(fd, socket_t::mode_t::READ);
						// Выполняем инициализацию буфера
						this->_buffer.data = std::unique_ptr <uint8_t []> (new uint8_t [this->_buffer.size]);
					}
					// Размер входящих сообщений
					const ssize_t bytes = ::recv(fd, this->_buffer.data.get(), this->_buffer.size, 0);
					// Если сообщение получено полностью
					if(bytes != 0){
						// Если мы получили данные
						if(bytes > 0){
							// Выполняем поиск нужный нам декодер
							auto i = this->_decoders.find(fd);
							// Если необходимый декодер найден
							if(i != this->_decoders.end()){
								// Выполняем добавление бинарных данных в протокол
								i->second->push(reinterpret_cast <char *> (this->_buffer.data.get()), static_cast <size_t> (bytes));
								// Выполняем извлечение записей
								while(!i->second->empty()){
									// Получаем буфер входящего сообщения
									const auto & message = i->second->get();
									// Определяем тип входящего сообщения
									switch(message.mid){
										// Если сообщение соответствует рукопожатию
										case static_cast <uint8_t> (message_t::HELLO): {
											// Добавляем полученный идентификатор процесса в список активных сокетов
											const_cast <cluster_t *> (this->_ctx)->_sockets.emplace(fd, i->second->pid());
											// Создаём новый объект энкодеров для передачи данных
											auto ret = const_cast <cluster_t *> (this->_ctx)->_encoders.emplace(i->second->pid(), make_unique <cmp::encoder_t> (this->_log));
											// Если метод компрессии сообщений установлен
											if(this->_ctx->_method != hash_t::method_t::NONE)
												// Выполняем установку метода компрессии
												ret.first->second->method(this->_ctx->_method);
											// Если размер шифрования и пароль установлены
											if((this->_ctx->_cipher != hash_t::cipher_t::NONE) && !this->_ctx->_pass.empty()){
												// Устанавливаем размер шифрования
												ret.first->second->cipher(this->_ctx->_cipher);
												// Устанавливаем пароль шифрования
												ret.first->second->password(this->_ctx->_pass);
												// Если соль шифрования установлена
												if(!this->_ctx->_salt.empty())
													// Устанавливаем соли шифрования
													ret.first->second->salt(this->_ctx->_salt);
											}
											// Определяем принцип передачи данных
											switch(static_cast <uint8_t> (this->_ctx->_transfer)){
												// Если мы передаём данные через unix-сокет
												case static_cast <uint8_t> (transfer_t::IPC): {
													// Выполняем поиск текущего брокера
													auto j = this->_ctx->_brokers.find(this->_wid);
													// Если текущий брокер найден
													if(j != this->_ctx->_brokers.end()){
														// Выполняем поиск идентификатор процесса
														auto k = this->_ctx->_pids.find(i->second->pid());
														// Если идентификатор процесса найден
														if(k != this->_ctx->_pids.end()){
															// Получаем найденного брокера
															broker_t * broker = j->second.at(k->second).get();
															// Устанавливаем файловый дескриптор на чтение данных
															broker->mfds[0] = fd;
															// Устанавливаем файловый дескриптор на запись данных
															broker->cfds[1] = fd;
														}
													}
													// Устанавливаем размер блока энкодера по размеру буфера данных сокета
													ret.first->second->chunkSize(this->_ctx->_server.socket.bufferSize(fd, socket_t::mode_t::WRITE));
												} break;
												// Если мы передаём данные через Shared memory
												case static_cast <uint8_t> (transfer_t::PIPE): {
													// Выполняем поиск текущего брокера
													auto j = this->_ctx->_brokers.find(this->_wid);
													// Если текущий брокер найден
													if(j != this->_ctx->_brokers.end()){
														// Выполняем поиск идентификатор процесса
														auto k = this->_ctx->_pids.find(i->second->pid());
														// Если идентификатор процесса найден
														if(k != this->_ctx->_pids.end())
															// Устанавливаем размер блока энкодера по размеру буфера данных сокета
															ret.first->second->chunkSize(this->_ctx->_server.socket.bufferSize(j->second.at(k->second)->cfds[1], socket_t::mode_t::WRITE));
													}
												} break;
											}
											// Если функция обратного вызова установлена
											if(this->_ctx->_callback.is("ready"))
												// Выполняем функцию обратного вызова
												this->_ctx->_callback.call <void (const uint16_t, const pid_t)> ("ready", this->_wid, i->second->pid());
										} break;
										// Если сообщение соответствует общему формату данных
										case static_cast <uint8_t> (message_t::GENERAL): {
											// Если функция обратного вызова установлена
											if(this->_ctx->_callback.is("message")){
												// Если буфер данных получен
												if((message.buffer != nullptr) && (message.size > 0))
													// Выполняем функцию обратного вызова
													this->_ctx->_callback.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, i->second->pid(), message.buffer, message.size);
												// Выводим значение по умолчанию
												else this->_ctx->_callback.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, i->second->pid(), nullptr, 0);
											}
										} break;
									}
									// Выполняем удаление указанной записи
									i->second->pop();
								}
							}
						}
					// Если данные не прочитаны
					} else {
						// Выводим сообщение об ошибке в лог
						this->_log->print("[%u] Data from child process could not be received", log_t::flag_t::CRITICAL, this->_ctx->_pid);
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
					// Определяем принцип передачи данных
					switch(static_cast <uint8_t> (this->_ctx->_transfer)){
						// Если мы передаём данные через unix-сокет
						case static_cast <uint8_t> (transfer_t::IPC): {
							// Выполняем поиск текущего брокера
							auto i = this->_ctx->_brokers.find(this->_wid);
							// Если текущий брокер найден
							if(i != this->_ctx->_brokers.end())
								// Выполняем удаление брокера подключения
								const_cast <cluster_t *> (this->_ctx)->_brokers.erase(i);
							// Ищем сокет в списке подключений
							auto j = this->_ctx->_clients.find(fd);
							// Если сокет есть в списке подключений
							if(j != this->_ctx->_clients.end()){
								// Выполняем остановку получения событий
								j->second->ev.stop();
								// Закрываем сокет подключения
								const_cast <cluster_t *> (this->_ctx)->close(j->second->wid, j->second->fd);
								// Выполняем удаление клиента
								const_cast <cluster_t *> (this->_ctx)->_clients.erase(j);
							}
						} break;
						// Если мы передаём данные через Shared memory
						case static_cast <uint8_t> (transfer_t::PIPE): {
							// Выполняем поиск текущего брокера
							auto i = this->_ctx->_brokers.find(this->_wid);
							// Если текущий брокер найден
							if(i != this->_ctx->_brokers.end()){
								// Получаем индекс текущего процесса
								const uint16_t index = this->_ctx->_pids.at(::getpid());
								// Получаем объект текущего брокера
								broker_t * broker = i->second.at(index).get();
								// Выполняем остановку чтение сообщений
								broker->read.stop();
								// Выполняем остановку отправки сообщений
								broker->write.stop();
								// Выполняем закрытие файловых дескрипторов
								const_cast <cluster_t *> (this->_ctx)->close(i->first, broker->cfds[0]);
								const_cast <cluster_t *> (this->_ctx)->close(i->first, broker->mfds[1]);
							}
						} break;
					}
					// Закрываем сокет подключения
					const_cast <cluster_t *> (this->_ctx)->close(this->_wid, fd);
					// Останавливаем чтение данных с родительского процесса
					const_cast <cluster_t *> (this->_ctx)->stop(this->_wid);
					// Выводим сообщение об ошибке в лог
					this->_log->print("[%u] Data from main process is closed", log_t::flag_t::CRITICAL, ::getpid());
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
									item->read.stop();
									// Выполняем остановку отправки сообщений
									item->write.stop();
									// Закрываем файловый дескриптор на чтение из дочернего процесса
									const_cast <cluster_t *> (this->_ctx)->close(i->first, item->cfds[0]);
									// Закрываем файловый дескриптор на запись в основной процесс
									const_cast <cluster_t *> (this->_ctx)->close(i->first, item->mfds[1]);
								}
							}
							// Выполняем остановку чтение сообщений
							broker->read.stop();
							// Выполняем остановку отправки сообщений
							broker->write.stop();
							// Выходим из функции
							return;
						}
						// Если буфер данных её не инициализирован
						if(this->_buffer.size == 0){
							// Извлекаем размер сформированного буфера
							this->_buffer.size = this->_ctx->_server.socket.bufferSize(fd, socket_t::mode_t::READ);
							// Выполняем инициализацию буфера
							this->_buffer.data = std::unique_ptr <uint8_t []> (new uint8_t [this->_buffer.size]);
						}
						// Размер входящих сообщений
						const int32_t bytes = ::recv(fd, this->_buffer.data.get(), this->_buffer.size, 0);
						// Если сообщение получено полностью
						if(bytes != 0){
							// Если данные прочитанны правильно
							if(bytes > 0){
								// Если функция обратного вызова установлена
								if(this->_ctx->_callback.is("message")){
									// Выполняем поиск нужный нам декодер
									auto i = this->_decoders.find(fd);
									// Если необходимый декодер найден
									if(i != this->_decoders.end()){
										// Выполняем добавление бинарных данных в протокол
										i->second->push(reinterpret_cast <char *> (this->_buffer.data.get()), static_cast <size_t> (bytes));
										// Выполняем извлечение записей
										while(!i->second->empty()){
											// Получаем буфер входящего сообщения
											const auto & message = i->second->get();
											// Определяем тип входящего сообщения
											switch(message.mid){
												// Если сообщение соответствует рукопожатию
												case static_cast <uint8_t> (message_t::HELLO):
													// Выводим сообщение об ошибке в лог
													this->_log->print("[%u] Master sends us his regards :=)", log_t::flag_t::WARNING, ::getpid());
												break;
												// Если сообщение соответствует общему формату данных
												case static_cast <uint8_t> (message_t::GENERAL): {
													// Если буфер данных получен
													if((message.buffer != nullptr) && (message.size > 0))
														// Выполняем функцию обратного вызова
														this->_ctx->_callback.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, this->_ctx->_pid, message.buffer, message.size);
													// Выводим значение по умолчанию
													else this->_ctx->_callback.call <void (const uint16_t, const pid_t, const char *, const size_t)> ("message", this->_wid, this->_ctx->_pid, nullptr, 0);
												} break;
											}
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
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Глобальный объект воркера
	 */
	static awh::cluster_t * cluster = nullptr;
	/**
	 * ipc Метод инициализации unix-сокета для обмены данными
	 * @param family семейстов кластера
	 */
	void awh::Cluster::ipc(const family_t family) noexcept {
		// Если приложение является сервером
		if(family == family_t::MASTER){
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_server.fs.isSock(this->_server.ipc))
				// Выходим из функции
				return;
		}
		// Если unix-сокет передан
		if(!this->_server.ipc.empty()){
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Создаем сокет подключения
				this->_server.fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
				// Если сокет не создан то выходим
				if((this->_server.fd == INVALID_SOCKET) || (this->_server.fd >= AWH_MAX_SOCKETS)){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->_server.ipc, static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
					#endif
					// Выходим
					return;
				}
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_server.socket.noSigILL();
				// Устанавливаем разрешение на повторное использование сокета
				this->_server.socket.reuseable(this->_server.fd);
				// Отключаем сигнал записи в оборванное подключение
				this->_server.socket.noSigPIPE(this->_server.fd);
				// Если приложение является сервером
				if(family == family_t::MASTER)
					// Переводим сокет в не блокирующий режим
					this->_server.socket.blocking(this->_server.fd, socket_t::mode_t::DISABLED);
				// Создаём объект подключения для клиента
				struct sockaddr_un client;
				// Создаём объект подключения для сервера
				struct sockaddr_un server;
				// Зануляем изначальную структуру данных клиента
				::memset(&client, 0, sizeof(client));
				// Зануляем изначальную структуру данных сервера
				::memset(&server, 0, sizeof(server));
				// Устанавливаем размер объекта подключения
				this->_server.peer.size = sizeof(struct sockaddr_un);
				// Устанавливаем протокол интернета
				server.sun_family = AF_UNIX;
				// Очищаем всю структуру для сервера
				::memset(&server.sun_path, 0, sizeof(server.sun_path));
				// Копируем адрес сокета сервера
				::strncpy(server.sun_path, this->_server.ipc.c_str(), sizeof(server.sun_path));
				// Выполняем копирование объект подключения сервера в сторейдж
				::memcpy(&this->_server.peer.addr, &server, sizeof(server));
				// Если приложение является сервером
				if(family == family_t::MASTER){
					// Получаем размер объекта сокета
					const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(server.sun_path));
					// Выполняем бинд на сокет
					if(::bind(this->_server.fd, reinterpret_cast <struct sockaddr *> (&this->_server.peer.addr), size) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(this->_server.ipc, static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выводим информацию о запущенном сервере на unix-сокете
					this->_log->print("Cluster [%s] has been started [%s] successfully", log_t::flag_t::INFO, this->_name.c_str(), this->_server.ipc.c_str());
				}
			#endif
		}
	}
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
				if((broker->stop = (broker->pid == pid))){
					// Выполняем остановку чтение сообщений
					broker->read.stop();
					// Выполняем остановку отправки сообщений
					broker->write.stop();
					// Выполняем закрытие файловых дескрипторов
					this->close(item.first, broker->mfds[0]);
					this->close(item.first, broker->cfds[1]);
					// Выводим сообщение об ошибке, о невозможности отправкить сообщение
					this->_log->print("Child process stopped, PID=%d, STATUS=%d", log_t::flag_t::WARNING, broker->pid, status);
					// Если статус сигнала, ручной остановкой процесса
					if(status == SIGINT){
						// Выполняем остановку работы
						this->clear();
						// Выходим из приложения
						::exit(SIGINT);
					// Если время жизни процесса составляет меньше 3-х минут
					} else if((this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) - broker->date) <= 180000) {
						// Выполняем остановку работы
						this->clear();
						// Выходим из приложения
						::exit(status);
					}
					// Если функция обратного вызова установлена
					if(this->_callback.is("exit"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint16_t, const pid_t, const int32_t)> ("exit", item.first, pid, status);
					// Если функция обратного вызова установлена
					if(this->_callback.is("process"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint16_t, const pid_t, const event_t)> ("process", item.first, pid, event_t::STOP);
					// Выполняем поиск воркера
					auto i = this->_workers.find(item.first);
					// Если запрашиваемый воркер найден и флаг автоматического перезапуска активен
					if((i != this->_workers.end()) && i->second->_autoRestart){
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
 * list Метод активации прослушивания сокета
 * @return результат выполнения операции
 */
bool awh::Cluster::list() noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем слушать порт сервера
	if(!(result = (::listen(this->_server.fd, SOMAXCONN) == 0))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, this->_server.socket.message(AWH_ERROR()).c_str());
		#endif
	}
	// Выходим из функции
	return result;
}
/**
 * connect Метод выполнения подключения
 * @return результат выполнения операции
 */
bool awh::Cluster::connect() noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Получаем размер объекта сокета
		this->_server.peer.size = (
			offsetof(struct sockaddr_un, sun_path) +
			strlen(reinterpret_cast <struct sockaddr_un *> (&this->_server.peer.addr)->sun_path)
		);
		// Если подключение к масте-процессу
		return ((this->_server.peer.size > 0) && (::connect(this->_server.fd, reinterpret_cast <struct sockaddr *> (&this->_server.peer.addr), this->_server.peer.size) == 0));
	#endif
	// Выводим статус подключения
	return false;
}
/**
 * accept Метод обратного вызова получении запроса на подключение
 * @param wid идентификатор воркера
 * @param fd  файловый дескриптор (сокет)
 */
void awh::Cluster::accept(const uint16_t wid, const SOCKET fd, const base_t::event_type_t) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем поиск воркера
		auto i = this->_workers.find(wid);
		// Если воркер найден
		if(i != this->_workers.end()){
			// Выполняем инициализацию нового клиента
			std::unique_ptr <client_t> client = make_unique <client_t> (this->_fmk, this->_log);
			// Заполняем структуру клиента нулями
			::memset(&client->peer.addr, 0, sizeof(client->peer.addr));
			// Создаём объект подключения для клиента
			struct sockaddr_un addr;
			// Очищаем всю структуру для клиента
			::memset(&addr, 0, sizeof(addr));
			// Устанавливаем протокол интернета
			addr.sun_family = AF_UNIX;
			// Запоминаем размер структуры
			client->peer.size = sizeof(addr);
			// Выполняем копирование объект подключения клиента в сторейдж
			::memcpy(&client->peer.addr, &addr, sizeof(addr));
			// Определяем разрешено ли подключение к прокси серверу
			client->fd = ::accept(fd, reinterpret_cast <struct sockaddr *> (&client->peer.addr), &client->peer.size);
			// Если сокет не создан тогда выходим
			if((client->fd == INVALID_SOCKET) || (client->fd >= AWH_MAX_SOCKETS))
				// Выходим из функции
				return;
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->_server.socket.noSigILL();
			// Отключаем сигнал записи в оборванное подключение
			this->_server.socket.noSigPIPE(client->fd);
			// Устанавливаем разрешение на повторное использование сокета
			this->_server.socket.reuseable(client->fd);
			// Переводим сокет в не блокирующий режим
			this->_server.socket.blocking(client->fd, socket_t::mode_t::DISABLED);
			// Устанавливаем размер буфера на чтение
			this->_server.socket.bufferSize(client->fd, this->_bandwidth.read, socket_t::mode_t::READ);
			// Устанавливаем размер буфера на запись
			this->_server.socket.bufferSize(client->fd, this->_bandwidth.write, socket_t::mode_t::WRITE);
			{
				// Добавляем новый декодер для созданного сокета чтения входящих сообщений
				auto ret = i->second->_decoders.emplace(client->fd, make_unique <cmp::decoder_t> (this->_log));
				// Устанавливаем размер блока декодерва по размеру буфера данных сокета
				ret.first->second->chunkSize(this->_server.socket.bufferSize(client->fd, socket_t::mode_t::READ));
				// Если размер шифрования и пароль установлены
				if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
					// Устанавливаем пароль шифрования
					ret.first->second->password(this->_pass);
					// Если соль шифрования установлена
					if(!this->_salt.empty())
						// Устанавливаем соли шифрования
						ret.first->second->salt(this->_salt);
				}
			}{
				// Выполняем добавление клиента в список новых клиентов
				auto ret = this->_clients.emplace(client->fd, ::move(client));
				// Устанавливаем идентификатор воркера
				ret.first->second->wid = wid;
				// Устанавливаем базу событий для чтения
				ret.first->second->ev = this->_core->eventBase();
				// Устанавливаем сокет для чтения
				ret.first->second->ev = ret.first->second->fd;
				// Устанавливаем событие на чтение данных от дочернего процесса
				ret.first->second->ev = std::bind(&worker_t::message, i->second.get(), _1, _2);
				// Выполняем запуск работы чтения данных с дочерних процессов
				ret.first->second->ev.start();
				// Выполняем активацию работы чтения данных с дочерних процессов
				ret.first->second->ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
				// Выполняем активацию работы события закрытия подключения
				ret.first->second->ev.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
				// Выполняем деактивацию работы события готовности на запись сокета
				ret.first->second->ev.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
			}
		}
	#endif
}
/**
 * write Метод записи буфера данных в сокет
 * @param wid идентификатор воркера
 * @param pid идентификатор процесса для получения сообщения
 * @param fd  идентификатор файлового дескриптора
 */
void awh::Cluster::write(const uint16_t wid, const pid_t pid, const SOCKET fd) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск активных энкодеров
			auto i = this->_encoders.find(pid);
			// Если объект энкодера найден
			if(i != this->_encoders.end()){
				// Получаем размер размер буфера данных
				const size_t max = i->second->chunkSize();
				// Выполняем перебор всех чанков протокола
				while(!i->second->empty()){
					// Получаем бинарный буфер данных
					const void * buffer = i->second->data();
					// Если данных в буфере нету, выходим из цикла
					if(buffer == nullptr){
						// Процесс превратился в зомби, самоликвидируем его
						this->_log->print("Cluster [%s] message sending stack was broken for process [%u]", log_t::flag_t::WARNING, this->_name.c_str(), ::getpid());
						// Выходим из цикла
						break;
					}
					// Если процесс является родительским
					if(this->_pid == static_cast <pid_t> (::getpid())){
						// Выполняем запись в сокет
						const ssize_t bytes = ::send(fd, reinterpret_cast <const char *> (buffer), (i->second->size() > max ? max : i->second->size()), 0);
						// Если данные доставлены успешно
						if(bytes > 0)
							// Удаляем чанк из объекта протокола
							i->second->erase(bytes);
						// Если мы поймали ошибку
						else if(bytes < 0) {
							// Определяем принцип передачи данных
							switch(static_cast <uint8_t> (this->_transfer)){
								// Если мы передаём данные через unix-сокет
								case static_cast <uint8_t> (transfer_t::IPC): {
									// Выполняем поиск нужного нам клиента
									auto i = this->_clients.find(fd);
									// Если нужный нам клиент найден
									if(i != this->_clients.end())
										// Выполняем активацию ожидания готовности сокета на запись
										i->second->ev.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
								} break;
								// Если мы передаём данные через Shared memory
								case static_cast <uint8_t> (transfer_t::PIPE): {
									// Выполняем поиск брокеров
									auto i = this->_brokers.find(wid);
									// Если брокер найден
									if(i != this->_brokers.end()){
										// Выполняем поиск нужного нам идентификатора процесса
										auto j = this->_pids.find(pid);
										// Если идентификатор процесса найден
										if(j != this->_pids.end())
											// Выполняем активацию ожидания готовности сокета на запись
											i->second.at(j->second)->write.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::ENABLED);
									}
								} break;
							}
							// Выходим из цикла
							break;
						// Если произошло отключение
						} else {
							// Выводим в лог сообщение
							this->_log->print("Cluster [%s] write: %s", log_t::flag_t::WARNING, this->_name.c_str(), this->_server.socket.message().c_str());
							// Выполняем остановку работы
							this->stop(wid);
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
					// Если процесс является дочерним
					} else {
						// Устанавливаем метку отправки данных
						Send:
						// Выполняем запись в сокет
						const ssize_t bytes = ::send(fd, reinterpret_cast <const char *> (buffer), (i->second->size() > max ? max : i->second->size()), 0);
						// Если данные доставлены успешно
						if(bytes > 0)
							// Удаляем чанк из объекта протокола
							i->second->erase(bytes);
						// Если мы поймали ошибку
						else if(bytes < 0) {
							// Если защищённый режим работы запрещён
							if((AWH_ERROR() == EWOULDBLOCK) || (AWH_ERROR() == EINTR))
								// Выполняем попытку отправить данные ещё раз
								goto Send;
							// Выводим в лог сообщение
							else {
								// Выводим в лог сообщение
								this->_log->print("Cluster [%s] write: %s", log_t::flag_t::WARNING, this->_name.c_str(), this->_server.socket.message().c_str());
								// Выходим из цикла
								break;
							}
						// Если произошло отключение
						} else {
							// Выводим в лог сообщение
							this->_log->print("Cluster [%s] write: %s", log_t::flag_t::WARNING, this->_name.c_str(), this->_server.socket.message().c_str());
							// Выполняем остановку работы
							this->stop(wid);
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						}
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
 * sending Метод обратного вызова получении сообщений готовности сокета на запись
 * @param wid идентификатор воркера
 * @param pid идентификатор процесса для отправки сообщения
 * @param fd  файловый дескриптор (сокет)
 */
void awh::Cluster::sending(const uint16_t wid, const pid_t pid, const SOCKET fd) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем принцип передачи данных
			switch(static_cast <uint8_t> (this->_transfer)){
				// Если мы передаём данные через unix-сокет
				case static_cast <uint8_t> (transfer_t::IPC): {
					// Выполняем поиск активного клиента
					auto i = this->_clients.find(fd);
					// Если активный клиент найден
					if(i != this->_clients.end()){
						// Выполняем деаактивацию работы события готовности сокета на запись
						i->second->ev.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
						// Выполняем отправку сообщения дочернему-процессу
						this->write(wid, pid, fd);
					}
				} break;
				// Если мы передаём данные через Shared memory
				case static_cast <uint8_t> (transfer_t::PIPE): {
					// Выполняем поиск брокеров
					auto i = this->_brokers.find(wid);
					// Если брокер найден
					if(i != this->_brokers.end()){
						// Выполняем поиск нужного нам идентификатора процесса
						auto j = this->_pids.find(pid);
						// Если идентификатор процесса найден
						if(j != this->_pids.end()){
							// Выполняем деаактивацию работы события готовности сокета на запись
							i->second.at(j->second)->write.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
							// Выполняем отправку сообщения дочернему-процессу
							this->write(i->first, j->first, fd);
						}
					}
				} break;
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(wid, pid, fd), log_t::flag_t::CRITICAL, error.what());
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
	 * Для операционной системы не являющейся OS Windows
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
					std::unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
					// Если мы передаём данные через Shared memory
					if(this->_transfer == transfer_t::PIPE){
						// Выполняем подписку на основной канал передачи данных
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->mfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("Cluster [%s] fork child: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
							// Выполняем остановку работы
							this->clear();
							// Выходим принудительно из приложения
							::exit(EXIT_FAILURE);
						}
						// Выполняем подписку на дочерний канал передачи данных
						if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->cfds) != 0){
							// Выводим в лог сообщение
							this->_log->print("Cluster [%s] fork child: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
							// Выполняем остановку работы
							this->clear();
							// Выходим принудительно из приложения
							::exit(EXIT_FAILURE);
						}
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
							this->_pids.emplace(pid, static_cast <uint16_t> (j->second.size() - 1));
							{
								// Выполняем переинициализацию базы событий
								this->_core->reinit();
								// Получаем объект текущего брокера
								broker_t * broker = j->second.back().get();
								// Устанавливаем идентификатор процесса
								broker->pid = pid;
								// Устанавливаем время начала жизни процесса
								broker->date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
								// Определяем принцип передачи данных
								switch(static_cast <uint8_t> (this->_transfer)){
									// Если мы передаём данные через unix-сокет
									case static_cast <uint8_t> (transfer_t::IPC): {
										// Выполняем закрытие сокета сервера
										this->close(i->first, this->_server.fd);
										// Выполняем инициализацию unix-сокета
										this->ipc(family_t::CHILDREN);
										// Если подключение выполнено
										while(!this->connect())
											// Погружаем поток в сон на 100мс
											this_thread::sleep_for(100ms);
										// Делаем сокет на чтение блокирующим
										this->_server.socket.blocking(this->_server.fd, socket_t::mode_t::ENABLED);
										// Выполняем установку нового сокета для чтения
										broker->cfds[0] = this->_server.fd;
										// Выполняем установку нового сокета для записи
										broker->mfds[1] = this->_server.fd;
										// Устанавливаем базу событий для чтения
										broker->read = this->_core->eventBase();
										// Устанавливаем сокет для чтения
										broker->read = this->_server.fd;
										// Устанавливаем событие на чтение данных от основного процесса
										broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
										// Запускаем чтение данных с основного процесса
										broker->read.start();
										// Выполняем активацию работы события чтения данных с сокета
										broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
										// Выполняем активацию работы события закрытия подключения
										broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
										// Устанавливаем размер буфера на чтение
										this->_server.socket.bufferSize(this->_server.fd, this->_bandwidth.read, socket_t::mode_t::READ);
										// Устанавливаем размер буфера на запись
										this->_server.socket.bufferSize(this->_server.fd, this->_bandwidth.write, socket_t::mode_t::WRITE);
										{
											// Добавляем новый декодер для созданного сокета чтения входящих сообщений
											auto ret = i->second->_decoders.emplace(this->_server.fd, make_unique <cmp::decoder_t> (this->_log));
											// Устанавливаем размер блока декодерва по размеру буфера данных сокета
											ret.first->second->chunkSize(this->_server.socket.bufferSize(this->_server.fd, socket_t::mode_t::READ));
											// Если размер шифрования и пароль установлены
											if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
												// Устанавливаем пароль шифрования
												ret.first->second->password(this->_pass);
												// Если соль шифрования установлена
												if(!this->_salt.empty())
													// Устанавливаем соли шифрования
													ret.first->second->salt(this->_salt);
											}
										}{
											// Создаём новый объект энкодеров для передачи данных
											auto ret = this->_encoders.emplace(this->_pid, make_unique <cmp::encoder_t> (this->_log));
											// Устанавливаем размер блока энкодера по размеру буфера данных сокета
											ret.first->second->chunkSize(this->_server.socket.bufferSize(this->_server.fd, socket_t::mode_t::WRITE));
											// Выполняем добавление буфера данных в протокол
											ret.first->second->push(static_cast <uint8_t> (message_t::HELLO), reinterpret_cast <const uint8_t *> (&pid), sizeof(pid));
											// Если метод компрессии сообщений установлен
											if(this->_method != hash_t::method_t::NONE)
												// Выполняем установку метода компрессии
												ret.first->second->method(this->_method);
											// Если размер шифрования и пароль установлены
											if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
												// Устанавливаем размер шифрования
												ret.first->second->cipher(this->_cipher);
												// Устанавливаем пароль шифрования
												ret.first->second->password(this->_pass);
												// Если соль шифрования установлена
												if(!this->_salt.empty())
													// Устанавливаем соли шифрования
													ret.first->second->salt(this->_salt);
											}
										}
										// Выполняем отправку сообщения мастер-процессу
										this->write(i->first, this->_pid, this->_server.fd);
									} break;
									// Если мы передаём данные через Shared memory
									case static_cast <uint8_t> (transfer_t::PIPE): {
										// Закрываем файловый дескриптор на запись в дочерний процесс
										this->close(i->first, broker->cfds[1]);
										// Закрываем файловый дескриптор на чтение из основного процесса
										this->close(i->first, broker->mfds[0]);
										// Выполняем перебор всего списка брокеров
										for(auto & item : j->second){
											// Если мы нашли брокера который не совпадает с нашими файловыми дескрипторами
											if((item->cfds[0] != broker->cfds[0]) && (item->cfds[0] != broker->mfds[1]))
												// Закрываем ненужный нам сокет
												this->close(i->first, item->cfds[0]);
											// Если мы нашли брокера который не совпадает с нашими файловыми дескрипторами
											if((item->cfds[1] != broker->cfds[0]) && (item->cfds[1] != broker->mfds[1]))
												// Закрываем ненужный нам сокет
												this->close(i->first, item->cfds[1]);
											// Если мы нашли брокера который не совпадает с нашими файловыми дескрипторами
											if(item->mfds[0] != broker->cfds[0] && item->mfds[0] != broker->mfds[1])
												// Закрываем ненужный нам сокет
												this->close(i->first, item->mfds[0]);
											// Если мы нашли брокера который не совпадает с нашими файловыми дескрипторами
											if(item->mfds[1] != broker->cfds[0] && item->mfds[1] != broker->mfds[1])
												// Закрываем ненужный нам сокет
												this->close(i->first, item->mfds[1]);
										}
										// Делаем сокет на чтение блокирующим
										this->_server.socket.blocking(broker->cfds[0], socket_t::mode_t::ENABLED);
										// Делаем сокет на запись блокирующим
										this->_server.socket.blocking(broker->mfds[1], socket_t::mode_t::ENABLED);
										// Устанавливаем базу событий для чтения
										broker->read = this->_core->eventBase();
										// Устанавливаем сокет для чтения
										broker->read = broker->cfds[0];
										// Устанавливаем событие на чтение данных от основного процесса
										broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
										// Запускаем чтение данных с основного процесса
										broker->read.start();
										// Выполняем активацию работы события чтения данных с сокета
										broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
										// Выполняем активацию работы события закрытия подключения
										broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
										// Устанавливаем размер буфера на чтение
										this->_server.socket.bufferSize(broker->cfds[0], this->_bandwidth.read, socket_t::mode_t::READ);
										// Устанавливаем размер буфера на запись
										this->_server.socket.bufferSize(broker->mfds[1], this->_bandwidth.write, socket_t::mode_t::WRITE);
										{
											// Добавляем новый декодер для созданного сокета чтения входящих сообщений
											auto ret = i->second->_decoders.emplace(broker->cfds[0], make_unique <cmp::decoder_t> (this->_log));
											// Устанавливаем размер блока декодерва по размеру буфера данных сокета
											ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->cfds[0], socket_t::mode_t::READ));
											// Если размер шифрования и пароль установлены
											if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
												// Устанавливаем пароль шифрования
												ret.first->second->password(this->_pass);
												// Если соль шифрования установлена
												if(!this->_salt.empty())
													// Устанавливаем соли шифрования
													ret.first->second->salt(this->_salt);
											}
										}{
											// Создаём новый объект энкодеров для передачи данных
											auto ret = this->_encoders.emplace(this->_pid, make_unique <cmp::encoder_t> (this->_log));
											// Устанавливаем размер блока энкодера по размеру буфера данных сокета
											ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->mfds[1], socket_t::mode_t::WRITE));
											// Выполняем добавление буфера данных в протокол
											ret.first->second->push(static_cast <uint8_t> (message_t::HELLO), reinterpret_cast <const uint8_t *> (&pid), sizeof(pid));
											// Если метод компрессии сообщений установлен
											if(this->_method != hash_t::method_t::NONE)
												// Выполняем установку метода компрессии
												ret.first->second->method(this->_method);
											// Если размер шифрования и пароль установлены
											if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
												// Устанавливаем размер шифрования
												ret.first->second->cipher(this->_cipher);
												// Устанавливаем пароль шифрования
												ret.first->second->password(this->_pass);
												// Если соль шифрования установлена
												if(!this->_salt.empty())
													// Устанавливаем соли шифрования
													ret.first->second->salt(this->_salt);
											}
										}
										// Выполняем отправку сообщения мастер-процессу
										this->write(i->first, this->_pid, broker->mfds[1]);
									} break;
								}
								// Если функция обратного вызова установлена
								if(this->_callback.is("process"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
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
						this->_pids.emplace(pid, static_cast <uint16_t> (j->second.size() - 1));
						// Получаем объект текущего брокера
						broker_t * broker = j->second.back().get();
						// Устанавливаем PID-процесса
						broker->pid = pid;
						// Устанавливаем время начала жизни процесса
						broker->date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
						// Определяем принцип передачи данных
						switch(static_cast <uint8_t> (this->_transfer)){
							// Если мы передаём данные через Shared memory
							case static_cast <uint8_t> (transfer_t::PIPE): {
								// Закрываем файловый дескриптор на запись в основной процесс
								this->close(i->first, broker->mfds[1]);
								// Закрываем файловый дескриптор на чтение из дочернего процесса
								this->close(i->first, broker->cfds[0]);
								// Делаем сокет на чтение неблокирующим
								this->_server.socket.blocking(broker->mfds[0], socket_t::mode_t::DISABLED);
								// Делаем сокет на запись неблокирующим
								this->_server.socket.blocking(broker->cfds[1], socket_t::mode_t::DISABLED);
								// Устанавливаем базу событий для чтения
								broker->read = this->_core->eventBase();
								// Устанавливаем сокет для чтения
								broker->read = broker->mfds[0];
								// Устанавливаем событие на чтение данных от дочернего процесса
								broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
								// Выполняем запуск работы чтения данных с дочерних процессов
								broker->read.start();
								// Устанавливаем базу событий для записи
								broker->write = this->_core->eventBase();
								// Устанавливаем сокет для записи
								broker->write = broker->cfds[1];
								// Устанавливаем событие на запись данных дочернему процессу
								broker->write = std::bind(&cluster_t::sending, this, i->first, pid, _1);
								// Выполняем запуск работы записи данных дочернему процессу
								broker->write.start();
								// Устанавливаем размер буфера на чтение
								this->_server.socket.bufferSize(broker->mfds[0], this->_bandwidth.read, socket_t::mode_t::READ);
								// Устанавливаем размер буфера на запись
								this->_server.socket.bufferSize(broker->cfds[1], this->_bandwidth.write, socket_t::mode_t::WRITE);
								// Добавляем новый декодер для созданного сокета чтения входящих сообщений
								auto ret = i->second->_decoders.emplace(broker->mfds[0], make_unique <cmp::decoder_t> (this->_log));
								// Устанавливаем размер блока декодерва по размеру буфера данных сокета
								ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->mfds[0], socket_t::mode_t::READ));
								// Если размер шифрования и пароль установлены
								if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
									// Устанавливаем пароль шифрования
									ret.first->second->password(this->_pass);
									// Если соль шифрования установлена
									if(!this->_salt.empty())
										// Устанавливаем соли шифрования
										ret.first->second->salt(this->_salt);
								}
								// Выполняем активацию работы чтения данных с дочерних процессов
								broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
								// Выполняем активацию работы события закрытия подключения
								broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
								// Выполняем деактивацию работы записи данных в дочерний процесс
								broker->write.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
								// Выполняем активацию работы события закрытия подключения
								broker->write.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
							} break;
						}
						// Если функция обратного вызова установлена
						if(this->_callback.is("rebase") && (opid > 0))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint16_t, const pid_t, const pid_t)> ("rebase", i->first, pid, opid);
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
	 * Для операционной системы не являющейся OS Windows
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
							this->_brokers.emplace(i->first, vector <std::unique_ptr <broker_t>> ());
							// Выполняем поиск брокера
							j = this->_brokers.find(i->first);
						}
						// Выполняем создание указанное количество брокеров
						for(size_t index = 0; index < i->second->_count; index++){
							// Создаём объект брокера
							std::unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
							// Если мы передаём данные через Shared memory
							if(this->_transfer == transfer_t::PIPE){
								// Выполняем подписку на основной канал передачи данных
								if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->mfds) != 0){
									// Выводим в лог сообщение
									this->_log->print("Cluster [%s] fork child: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
									// Выполняем остановку работы
									this->clear();
									// Выходим принудительно из приложения
									::exit(EXIT_FAILURE);
								}
								// Выполняем подписку на дочерний канал передачи данных
								if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->cfds) != 0){
									// Выводим в лог сообщение
									this->_log->print("Cluster [%s] fork child: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
									// Выполняем остановку работы
									this->clear();
									// Выходим принудительно из приложения
									::exit(EXIT_FAILURE);
								}
							}
							// Выполняем добавление брокера в список брокеров
							j->second.push_back(::move(broker));
						}
					}
					// Если процесс завершил свою работу
					if(j->second.at(index)->stop){
						// Создаём объект брокера
						std::unique_ptr <broker_t> broker(new broker_t(this->_fmk, this->_log));
						// Если мы передаём данные через Shared memory
						if(this->_transfer == transfer_t::PIPE){
							// Выполняем подписку на основной канал передачи данных
							if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->mfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster [%s] fork: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
								// Выполняем поиск завершившегося процесса
								for(auto & broker : j->second){
									// Выполняем остановку чтение сообщений
									broker->read.stop();
									// Выполняем остановку записи сообщений
									broker->write.stop();
								}
								// Выполняем остановку работы
								this->clear();
								// Выходим принудительно из приложения
								::exit(EXIT_FAILURE);
							}
							// Выполняем подписку на дочерний канал передачи данных
							if(::socketpair(AF_UNIX, SOCK_STREAM, 0, broker->cfds) != 0){
								// Выводим в лог сообщение
								this->_log->print("Cluster [%s] fork: %s", log_t::flag_t::CRITICAL, this->_name.c_str(), this->_server.socket.message(AWH_ERROR()).c_str());
								// Выполняем поиск завершившегося процесса
								for(auto & broker : j->second){
									// Выполняем остановку чтение сообщений
									broker->read.stop();
									// Выполняем остановку записи сообщений
									broker->write.stop();
								}
								// Выполняем остановку работы
								this->clear();
								// Выходим принудительно из приложения
								::exit(EXIT_FAILURE);
							}
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
									// Устанавливаем идентификатор процесса
									broker->pid = pid;
									// Устанавливаем время начала жизни процесса
									broker->date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
									// Определяем принцип передачи данных
									switch(static_cast <uint8_t> (this->_transfer)){
										// Если мы передаём данные через unix-сокет
										case static_cast <uint8_t> (transfer_t::IPC): {
											// Выполняем закрытие сокета сервера
											this->close(i->first, this->_server.fd);
											// Выполняем инициализацию unix-сокета
											this->ipc(family_t::CHILDREN);
											// Если подключение выполнено
											while(!this->connect())
												// Погружаем поток в сон на 100мс
												this_thread::sleep_for(100ms);
											// Делаем сокет на чтение блокирующим
											this->_server.socket.blocking(this->_server.fd, socket_t::mode_t::ENABLED);
											// Выполняем установку нового сокета для чтения
											broker->cfds[0] = this->_server.fd;
											// Выполняем установку нового сокета для записи
											broker->mfds[1] = this->_server.fd;
											// Устанавливаем базу событий для чтения
											broker->read = this->_core->eventBase();
											// Устанавливаем сокет для чтения
											broker->read = this->_server.fd;
											// Устанавливаем событие на чтение данных от основного процесса
											broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
											// Запускаем чтение данных с основного процесса
											broker->read.start();
											// Выполняем активацию работы события чтения данных с сокета
											broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
											// Выполняем активацию работы события закрытия подключения
											broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
											// Устанавливаем размер буфера на чтение
											this->_server.socket.bufferSize(this->_server.fd, this->_bandwidth.read, socket_t::mode_t::READ);
											// Устанавливаем размер буфера на запись
											this->_server.socket.bufferSize(this->_server.fd, this->_bandwidth.write, socket_t::mode_t::WRITE);
											{
												// Добавляем новый декодер для созданного сокета чтения входящих сообщений
												auto ret = i->second->_decoders.emplace(this->_server.fd, make_unique <cmp::decoder_t> (this->_log));
												// Устанавливаем размер блока декодерва по размеру буфера данных сокета
												ret.first->second->chunkSize(this->_server.socket.bufferSize(this->_server.fd, socket_t::mode_t::READ));
												// Если размер шифрования и пароль установлены
												if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
													// Устанавливаем пароль шифрования
													ret.first->second->password(this->_pass);
													// Если соль шифрования установлена
													if(!this->_salt.empty())
														// Устанавливаем соли шифрования
														ret.first->second->salt(this->_salt);
												}
											}{
												// Создаём новый объект энкодеров для передачи данных
												auto ret = this->_encoders.emplace(this->_pid, make_unique <cmp::encoder_t> (this->_log));
												// Устанавливаем размер блока энкодера по размеру буфера данных сокета
												ret.first->second->chunkSize(this->_server.socket.bufferSize(this->_server.fd, socket_t::mode_t::WRITE));
												// Выполняем добавление буфера данных в протокол
												ret.first->second->push(static_cast <uint8_t> (message_t::HELLO), reinterpret_cast <const uint8_t *> (&pid), sizeof(pid));
												// Если метод компрессии сообщений установлен
												if(this->_method != hash_t::method_t::NONE)
													// Выполняем установку метода компрессии
													ret.first->second->method(this->_method);
												// Если размер шифрования и пароль установлены
												if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
													// Устанавливаем размер шифрования
													ret.first->second->cipher(this->_cipher);
													// Устанавливаем пароль шифрования
													ret.first->second->password(this->_pass);
													// Если соль шифрования установлена
													if(!this->_salt.empty())
														// Устанавливаем соли шифрования
														ret.first->second->salt(this->_salt);
												}
											}
											// Выполняем отправку сообщения мастер-процессу
											this->write(i->first, this->_pid, this->_server.fd);
										} break;
										// Если мы передаём данные через Shared memory
										case static_cast <uint8_t> (transfer_t::PIPE): {
											// Закрываем файловый дескриптор на запись в дочерний процесс
											this->close(i->first, broker->cfds[1]);
											// Закрываем файловый дескриптор на чтение из основного процесса
											this->close(i->first, broker->mfds[0]);
											// Выполняем перебор всего списка брокеров
											for(auto & item : j->second){
												// Если найдены остальные брокеры
												if(item->cfds[1] != broker->cfds[1]){
													// Закрываем файловый дескриптор на запись в дочерний процесс
													this->close(i->first, item->cfds[0]);
													this->close(i->first, item->cfds[1]);
													// Закрываем файловый дескриптор на чтение из основного процесса
													this->close(i->first, item->mfds[0]);
													this->close(i->first, item->mfds[1]);
												}
											}
											// Делаем сокет на чтение блокирующим
											this->_server.socket.blocking(broker->cfds[0], socket_t::mode_t::ENABLED);
											// Делаем сокет на запись блокирующим
											this->_server.socket.blocking(broker->mfds[1], socket_t::mode_t::ENABLED);
											// Устанавливаем базу событий для чтения
											broker->read = this->_core->eventBase();
											// Устанавливаем сокет для чтения
											broker->read = broker->cfds[0];
											// Устанавливаем событие на чтение данных от основного процесса
											broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
											// Запускаем чтение данных с основного процесса
											broker->read.start();
											// Выполняем активацию работы события чтения данных с сокета
											broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
											// Выполняем активацию работы события закрытия подключения
											broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
											// Устанавливаем размер буфера на чтение
											this->_server.socket.bufferSize(broker->cfds[0], this->_bandwidth.read, socket_t::mode_t::READ);
											// Устанавливаем размер буфера на запись
											this->_server.socket.bufferSize(broker->mfds[1], this->_bandwidth.write, socket_t::mode_t::WRITE);
											{
												// Добавляем новый декодер для созданного сокета чтения входящих сообщений
												auto ret = i->second->_decoders.emplace(broker->cfds[0], make_unique <cmp::decoder_t> (this->_log));
												// Устанавливаем размер блока декодерва по размеру буфера данных сокета
												ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->cfds[0], socket_t::mode_t::READ));
												// Если размер шифрования и пароль установлены
												if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
													// Устанавливаем пароль шифрования
													ret.first->second->password(this->_pass);
													// Если соль шифрования установлена
													if(!this->_salt.empty())
														// Устанавливаем соли шифрования
														ret.first->second->salt(this->_salt);
												}
											}{
												// Создаём новый объект энкодеров для передачи данных
												auto ret = this->_encoders.emplace(this->_pid, make_unique <cmp::encoder_t> (this->_log));
												// Устанавливаем размер блока энкодера по размеру буфера данных сокета
												ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->mfds[1], socket_t::mode_t::WRITE));
												// Выполняем добавление буфера данных в протокол
												ret.first->second->push(static_cast <uint8_t> (message_t::HELLO), reinterpret_cast <const uint8_t *> (&pid), sizeof(pid));
												// Если метод компрессии сообщений установлен
												if(this->_method != hash_t::method_t::NONE)
													// Выполняем установку метода компрессии
													ret.first->second->method(this->_method);
												// Если размер шифрования и пароль установлены
												if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
													// Устанавливаем размер шифрования
													ret.first->second->cipher(this->_cipher);
													// Устанавливаем пароль шифрования
													ret.first->second->password(this->_pass);
													// Если соль шифрования установлена
													if(!this->_salt.empty())
														// Устанавливаем соли шифрования
														ret.first->second->salt(this->_salt);
												}
											}
											// Выполняем отправку сообщения мастер-процессу
											this->write(i->first, this->_pid, broker->mfds[1]);
										} break;
									}
									// Если функция обратного вызова установлена
									if(this->_callback.is("process"))
										// Выполняем функцию обратного вызова
										this->_callback.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, pid, event_t::START);
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
							// Устанавливаем PID-процесса
							broker->pid = pid;
							// Устанавливаем время начала жизни процесса
							broker->date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							// Определяем принцип передачи данных
							switch(static_cast <uint8_t> (this->_transfer)){
								// Если мы передаём данные через Shared memory
								case static_cast <uint8_t> (transfer_t::PIPE): {
									// Закрываем файловый дескриптор на запись в основной процесс
									this->close(i->first, broker->mfds[1]);
									// Закрываем файловый дескриптор на чтение из дочернего процесса
									this->close(i->first, broker->cfds[0]);
									// Делаем сокет на чтение неблокирующим
									this->_server.socket.blocking(broker->mfds[0], socket_t::mode_t::DISABLED);
									// Делаем сокет на запись неблокирующим
									this->_server.socket.blocking(broker->cfds[1], socket_t::mode_t::DISABLED);
									// Устанавливаем базу событий для чтения
									broker->read = this->_core->eventBase();
									// Устанавливаем сокет для чтения
									broker->read = broker->mfds[0];
									// Устанавливаем событие на чтение данных от дочернего процесса
									broker->read = std::bind(&worker_t::message, i->second.get(), _1, _2);
									// Выполняем запуск работы чтения данных с дочерних процессов
									broker->read.start();
									// Устанавливаем базу событий для записи
									broker->write = this->_core->eventBase();
									// Устанавливаем сокет для записи
									broker->write = broker->cfds[1];
									// Устанавливаем событие на запись данных дочернему процессу
									broker->write = std::bind(&cluster_t::sending, this, i->first, pid, _1);
									// Выполняем запуск работы записи данных дочернему процессу
									broker->write.start();
								} break;
							}
							// Выполняем создание новых процессов
							this->create(i->first, index + 1);
						}
					}
				// Если все процессы удачно созданы
				} else {
					// Выполняем поиск брокера
					auto j = this->_brokers.find(i->first);
					// Если идентификатор воркера получен
					if((i->second->_working = (j != this->_brokers.end()))){
						// Определяем принцип передачи данных
						switch(static_cast <uint8_t> (this->_transfer)){
							// Если мы передаём данные через unix-сокет
							case static_cast <uint8_t> (transfer_t::IPC): {
								// Выполняем инициализацию unix-сокета
								this->ipc(family_t::MASTER);
								// Выполняем прослушивание порта
								if(this->list()){
									// Устанавливаем базу событий для чтения
									this->_server.ev = this->_core->eventBase();
									// Устанавливаем сокет для чтения
									this->_server.ev = this->_server.fd;
									// Устанавливаем событие на чтение данных от дочернего процесса
									this->_server.ev = std::bind(&cluster_t::accept, this, i->first, _1, _2);
									// Выполняем запуск работы чтения данных с дочерних процессов
									this->_server.ev.start();
									// Активируем получение данных с клиента
									this->_server.ev.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
								}
							} break;
							// Если мы передаём данные через Shared memory
							case static_cast <uint8_t> (transfer_t::PIPE): {
								// Выполняем перебор всех доступных брокеров
								for(auto & broker : j->second){
									// Устанавливаем размер буфера на чтение
									this->_server.socket.bufferSize(broker->mfds[0], this->_bandwidth.read, socket_t::mode_t::READ);
									// Устанавливаем размер буфера на запись
									this->_server.socket.bufferSize(broker->cfds[1], this->_bandwidth.write, socket_t::mode_t::WRITE);
									// Добавляем новый декодер для созданного сокета чтения входящих сообщений
									auto ret = i->second->_decoders.emplace(broker->mfds[0], make_unique <cmp::decoder_t> (this->_log));
									// Устанавливаем размер блока декодерва по размеру буфера данных сокета
									ret.first->second->chunkSize(this->_server.socket.bufferSize(broker->mfds[0], socket_t::mode_t::READ));
									// Если размер шифрования и пароль установлены
									if((this->_cipher != hash_t::cipher_t::NONE) && !this->_pass.empty()){
										// Устанавливаем пароль шифрования
										ret.first->second->password(this->_pass);
										// Если соль шифрования установлена
										if(!this->_salt.empty())
											// Устанавливаем соли шифрования
											ret.first->second->salt(this->_salt);
									}
									// Выполняем активацию работы чтения данных с дочерних процессов
									broker->read.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
									// Выполняем активацию работы события закрытия подключения
									broker->read.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
									// Выполняем деактивацию работы записи данных в дочерний процесс
									broker->write.mode(base_t::event_type_t::WRITE, base_t::event_mode_t::DISABLED);
									// Выполняем активацию работы события закрытия подключения
									broker->write.mode(base_t::event_type_t::CLOSE, base_t::event_mode_t::ENABLED);
								}
							} break;
						}
						// Если мы передаём данные через PIPE
						if(this->_transfer == transfer_t::PIPE)
							// Выводим информацию о запущенном сервере на PIPE
							this->_log->print("Cluster [%s] has been started successfully", log_t::flag_t::INFO, this->_name.c_str());
						// Если функция обратного вызова установлена
						if(this->_callback.is("process"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const uint16_t, const pid_t, const event_t)> ("process", i->first, this->_pid, event_t::START);
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
		 * Для операционной системы не являющейся OS Windows
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
	 * Для операционной системы не являющейся OS Windows
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
			if(i != this->_brokers.end()){
				// Выполняем поиск идентификатор процесса
				auto j = this->_pids.find(pid);
				// Если идентификатор процесса найден
				if(j != this->_pids.end()){
					// Выполняем поиск объектов энкодеров для отправки сообщения
					auto k = this->_encoders.find(this->_pid);
					// Если протокол кластера найден
					if(k != this->_encoders.end()){
						// Выполняем добавление буфера данных в протокол
						k->second->push(static_cast <uint8_t> (message_t::GENERAL), buffer, size);
						// Выполняем отправку сообщения мастер-процессу
						this->write(i->first, this->_pid, i->second.at(j->second)->mfds[1]);
					}
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
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (::getpid())) && (size > 0)){
			// Выполняем поиск брокеров
			auto i = this->_brokers.find(wid);
			// Если брокер найден
			if(i != this->_brokers.end()){
				// Выполняем поиск идентификатор процесса
				auto j = this->_pids.find(pid);
				// Если идентификатор процесса найден
				if(j != this->_pids.end()){
					// Выполняем поиск объектов энкодеров для отправки сообщения
					auto k = this->_encoders.find(j->first);
					// Если протокол кластера найден
					if(k != this->_encoders.end()){
						// Определяем есть ли в буфере данные
						const bool empty = k->second->empty();
						// Выполняем добавление буфера данных в протокол
						k->second->push(static_cast <uint8_t> (message_t::GENERAL), buffer, size);
						// Если в буфере данных нету
						if(empty)
							// Выполняем отправку сообщения дочернему-процессу
							this->write(i->first, j->first, i->second.at(j->second)->cfds[1]);
					}
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
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if((this->_pid == static_cast <pid_t> (::getpid())) && (size > 0)){
			// Выполняем поиск брокеров
			auto i = this->_brokers.find(wid);
			// Если брокер найден
			if((i != this->_brokers.end()) && !i->second.empty()){
				// Переходим по всем дочерним процессам
				for(auto & broker : i->second){
					// Если идентификатор процесса не нулевой
					if(broker->pid > 0){
						// Выполняем поиск объектов энкодеров для отправки сообщения
						auto j = this->_encoders.find(broker->pid);
						// Если протокол кластера найден
						if(j != this->_encoders.end()){
							// Определяем есть ли в буфере данные
							const bool empty = j->second->empty();
							// Выполняем добавление буфера данных в протокол
							j->second->push(static_cast <uint8_t> (message_t::GENERAL), buffer, size);
							// Если в буфере данных нету
							if(empty)
								// Выполняем отправку сообщения дочернему-процессу
								this->write(i->first, broker->pid, broker->cfds[1]);
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
		// Определяем принцип передачи данных
		switch(static_cast <uint8_t> (this->_transfer)){
			// Если мы передаём данные через unix-сокет
			case static_cast <uint8_t> (transfer_t::IPC): {
				// останавливаем работу сервера
				this->_server.ev.stop();
				// Переходим по всему списку клиентов
				for(auto i = this->_clients.begin(); i != this->_clients.end();){
					// Выполняем остановку получения событий
					i->second->ev.stop();
					// Закрываем сокет подключения
					this->close(i->second->wid, i->second->fd);
					// Закрываем сокет подключения
					this->close(i->second->wid, this->_server.fd);
					// Выполняем удаление клиента
					i = this->_clients.erase(i);
				}
			} break;
			// Если мы передаём данные через Shared memory
			case static_cast <uint8_t> (transfer_t::PIPE): {
				// Переходим по всем брокерам
				for(auto & item : this->_brokers)
					// Выполняем остановку процессов
					this->stop(item.first);
			} break;
		}
		// Выполняем очистку списка брокеров
		this->_brokers.clear();
		// Выполняем освобождение выделенной памяти брокеров подключения
		std::map <uint16_t, vector <std::unique_ptr <broker_t>>> ().swap(this->_brokers);
	}
	// Выполняем очистку списка воркеров
	this->_workers.clear();
	// Выполняем очистку клиентов
	this->_clients.clear();
	// Выполняем очистку списка объектов энкодеров для отправки сообщений
	this->_encoders.clear();
	// Выполняем освобождение памяти активных клиентов
	std::map <SOCKET, std::unique_ptr <client_t>> ().swap(this->_clients);
	// Выполняем освобождение выделенной памяти
	std::map <uint16_t, std::unique_ptr <worker_t>> ().swap(this->_workers);
	// Выполняем освобождение памяти энкодера
	std::map <pid_t, std::unique_ptr <cmp::encoder_t>> ().swap(this->_encoders);
}
/**
 * close Метод закрытия всех подключений
 */
void awh::Cluster::close() noexcept {
	// Если список брокеров не пустой
	if(!this->_brokers.empty()){
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Определяем принцип передачи данных
			switch(static_cast <uint8_t> (this->_transfer)){
				// Если мы передаём данные через unix-сокет
				case static_cast <uint8_t> (transfer_t::IPC): {
					// Переходим по всему списку клиентов
					for(auto i = this->_clients.begin(); i != this->_clients.end();){
						// Выполняем остановку получения событий
						i->second->ev.stop();
						// Закрываем сокет подключения
						this->close(i->second->wid, i->second->fd);
						// Выполняем удаление клиента
						i = this->_clients.erase(i);
					}
					// Переходим по всем брокерам
					for(auto & item : this->_brokers){
						// Переходим по всему списку брокеров
						for(auto & broker : item.second){
							// Выполняем поиск объекта энкодера для отправки сообщений
							auto j = this->_encoders.find(broker->pid);
							// Если объект энкодера для отправки сообщений найден
							if(j != this->_encoders.end())
								// Выполняем очистку объекта энкодера
								j->second->clear();
						}
						// Очищаем список брокеров
						item.second.clear();
					}
				} break;
				// Если мы передаём данные через Shared memory
				case static_cast <uint8_t> (transfer_t::PIPE): {
					// Переходим по всем брокерам
					for(auto & item : this->_brokers){
						// Переходим по всему списку брокеров
						for(auto & broker : item.second){
							// Выполняем остановку чтение сообщений
							broker->read.stop();
							// Выполняем остановку записи сообщений
							broker->write.stop();
							// Выполняем закрытие файловых дескрипторов
							this->close(item.first, broker->cfds[0]);
							this->close(item.first, broker->cfds[1]);
							this->close(item.first, broker->mfds[0]);
							this->close(item.first, broker->mfds[1]);
							// Выполняем поиск объекта энкодера для отправки сообщений
							auto j = this->_encoders.find(broker->pid);
							// Если объект энкодера для отправки сообщений найден
							if(j != this->_encoders.end())
								// Выполняем очистку объекта энкодера
								j->second->clear();
						}
						// Очищаем список брокеров
						item.second.clear();
					}
				} break;
			}
		#endif
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
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Определяем принцип передачи данных
			switch(static_cast <uint8_t> (this->_transfer)){
				// Если мы передаём данные через unix-сокет
				case static_cast <uint8_t> (transfer_t::IPC): {
					// Переходим по всему списку брокеров
					for(auto & broker : i->second){
						// Выполняем поиск активного клиента
						auto i = this->_clients.find(broker->mfds[0]);
						// Если активный клиент найден
						if(i != this->_clients.end()){
							// Выполняем остановку получения событий
							i->second->ev.stop();
							// Закрываем сокет подключения
							this->close(i->second->wid, i->second->fd);
							// Выполняем удаление клиента
							this->_clients.erase(i);
						}
					}
					// Переходим по всему списку брокеров
					for(auto & broker : i->second){
						// Выполняем поиск объекта энкодера для отправки сообщений
						auto j = this->_encoders.find(broker->pid);
						// Если объект энкодера для отправки сообщений найден
						if(j != this->_encoders.end())
							// Выполняем очистку объекта энкодера
							j->second->clear();
					}
				} break;
				// Если мы передаём данные через Shared memory
				case static_cast <uint8_t> (transfer_t::PIPE): {
					// Переходим по всему списку брокеров
					for(auto & broker : i->second){
						// Выполняем остановку чтение сообщений
						broker->read.stop();
						// Выполняем остановку записи сообщений
						broker->write.stop();
						// Выполняем закрытие файловых дескрипторов
						this->close(i->first, broker->cfds[0]);
						this->close(i->first, broker->cfds[1]);
						this->close(i->first, broker->mfds[0]);
						this->close(i->first, broker->mfds[1]);
						// Выполняем поиск объекта энкодера для отправки сообщений
						auto j = this->_encoders.find(broker->pid);
						// Если объект энкодера для отправки сообщений найден
						if(j != this->_encoders.end())
							// Выполняем удаление объекта энкодера
							this->_encoders.erase(j);
					}
				}
			}
		#endif
		// Очищаем список брокеров
		i->second.clear();
	}
}
/**
 * close Метод закрытия файлового дескриптора
 * @param wid идентификатор воркера
 * @param fd  файловый дескриптор для закрытия
 */
void awh::Cluster::close(const uint16_t wid, const SOCKET fd) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Закрываем сокет подключения
		::close(fd);
		// Выполняем поиск воркера
		auto i = this->_workers.find(wid);
		// Если брокер найден
		if(i != this->_workers.end()){
			// Выполняем поиск нужного нам декодера
			auto j = i->second->_decoders.find(fd);
			// Если декодер найден
			if(j != i->second->_decoders.end())
				// Выполняем удаление декодера
				i->second->_decoders.erase(j);
		}
		// Выполняем поиск активного сокета
		auto j = this->_sockets.find(fd);
		// Если активный сокет найден
		if(j != this->_sockets.end())
			// Выполняем удаление сокета
			this->_sockets.erase(j);
	#endif
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
			bool autoRestart = false;
			// Если воркер найден, получаем флаг перезапуска
			if(i != this->_workers.end()){
				// Получаем флаг перезапуска
				autoRestart = i->second->_autoRestart;
				// Снимаем флаг перезапуска процесса
				i->second->_autoRestart = false;
			}
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выполняем закрытие подключения передачи сообщений
				this->close(wid);
			#endif
			// Если воркер найден, возвращаем флаг перезапуска
			if(i != this->_workers.end()){
				// Очищаем декодер воркера
				i->second->_decoders.clear();
				// Очищаем буфер получения данных
				i->second->_buffer = buffer_t();
				// Возвращаем значение флага автоматического перезапуска процесса
				i->second->_autoRestart = autoRestart;
			}
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
	 * Для операционной системы не являющейся OS Windows
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
 * name Метод установки названия кластера
 * @param name название кластера для установки
 */
void awh::Cluster::name(const string & name) noexcept {
	// Если процесс является родительским
	if(!name.empty() && (this->_pid == static_cast <pid_t> (::getpid()))){
		// Если названия не совпадают
		if(this->_name.compare(name) != 0){
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_server.fs.isSock(this->_server.ipc))
				// Удаляем файл сокета
				::unlink(this->_server.ipc.c_str());
			// Выполняем установку названия кластера
			this->_name = name;
			// Выполняем инициализацию unix-сокета
			this->_server.ipc = this->_fmk->format(
				"/tmp/%s_cluster_%u.sock",
				this->_fmk->transform(this->_name, fmk_t::transform_t::LOWER).c_str(),
				::getpid()
			);
		}
	}
}
/**
 * transfer Метод установки режима передачи данных
 * @param transfer режим передачи данных
 */
void awh::Cluster::transfer(const transfer_t transfer) noexcept {
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid()))
		// Выполняем установку режима передачи данных
		this->_transfer = transfer;
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::Cluster::salt(const string & salt) noexcept {
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid()))
		// Устанавливаем соль шифрования
		this->_salt = salt;
}
/**
 * password Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::Cluster::password(const string & password) noexcept {
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid()))
		// Устанавливаем пароль шифрования
		this->_pass = password;
}
/**
 * cipher Метод установки размера шифрования
 * @param cipher размер шифрования
 */
void awh::Cluster::cipher(const hash_t::cipher_t cipher) noexcept {
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid()))
		// Устанавливаем размер шифрования
		this->_cipher = cipher;
}
/**
 * compressor Метод установки метода компрессии
 * @param compressor метод компрессии для установки
 */
void awh::Cluster::compressor(const hash_t::method_t compressor) noexcept {
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid()))
		// Устанавливаем метод компрессии
		this->_method = compressor;
}
/**
 * emplace Метод размещения нового дочернего процесса
 * @param wid идентификатор воркера
 */
void awh::Cluster::emplace(const uint16_t wid) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
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
	 * Для операционной системы не являющейся OS Windows
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
					// Определяем принцип передачи данных
					switch(static_cast <uint8_t> (this->_transfer)){
						// Если мы передаём данные через unix-сокет
						case static_cast <uint8_t> (transfer_t::IPC): {
							// Выполняем поиск активного клиента
							auto i = this->_clients.find(broker->mfds[0]);
							// Если активный клиент найден
							if(i != this->_clients.end()){
								// Выполняем остановку получения событий
								i->second->ev.stop();
								// Закрываем сокет подключения
								this->close(i->second->wid, i->second->fd);
							}
						} break;
						// Если мы передаём данные через Shared memory
						case static_cast <uint8_t> (transfer_t::PIPE): {
							// Выполняем остановку чтение сообщений
							broker->read.stop();
							// Выполняем остановку записи сообщений
							broker->write.stop();
							// Выполняем закрытие открытых файловых дескрипторов
							this->close(i->first, broker->mfds[0]);
							this->close(i->first, broker->cfds[1]);
						} break;
					}
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
 * autoRestart Метод установки флага перезапуска процессов
 * @param wid  идентификатор воркера
 * @param mode флаг перезапуска процессов
 */
void awh::Cluster::autoRestart(const uint16_t wid, const bool mode) noexcept {
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если процесс является родительским
		if(this->_pid == static_cast <pid_t> (::getpid())){
			// Выполняем поиск идентификатора воркера
			auto i = this->_workers.find(wid);
			// Если вокер найден
			if(i != this->_workers.end())
				// Устанавливаем флаг автоматического перезапуска процесса
				i->second->_autoRestart = mode;
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
			this->_workers.emplace(wid, make_unique <worker_t> (wid, this, this->_log));
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
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::Cluster::bandwidth(const string & read, const string & write) noexcept {
	// Если размер пропускной способности на чтение передано
	if(!read.empty())
		// Устанавливаем размер пропускной способности на чтение
		this->_bandwidth.read = this->_fmk->sizeBuffer(read);
	// Если размер пропускной способности на запись передано
	if(!write.empty())
		// Устанавливаем размер пропускной способности на запись
		this->_bandwidth.write = this->_fmk->sizeBuffer(write);
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::Cluster::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова при завершении работы процесса
	this->_callback.set("exit", callback);
	// Выполняем установку функции обратного вызова при готовности процесса к обмену сообщениями
	this->_callback.set("ready", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса
	this->_callback.set("rebase", callback);
	// Выполняем установку функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
	this->_callback.set("process", callback);
	// Выполняем установку функции обратного вызова при получении сообщения
	this->_callback.set("message", callback);
}
/**
 * Cluster Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Cluster::Cluster(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _name{""}, _salt{""}, _pass{""}, _server(fmk, log), _callback(log),
 _transfer(transfer_t::PIPE), _cipher(hash_t::cipher_t::NONE), _method(hash_t::method_t::NONE),
 _core(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Для операционной системы не являющейся OS Windows
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
		// Выполняем инициализацию unix-сокета
		this->name(AWH_SHORT_NAME);
	#endif
}
/**
 * Cluster Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Cluster::Cluster(core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _name{""}, _salt{""}, _pass{""}, _server(fmk, log), _callback(log),
 _transfer(transfer_t::PIPE), _cipher(hash_t::cipher_t::NONE), _method(hash_t::method_t::NONE),
 _core(core), _fmk(fmk), _log(log) {
	/**
	 * Для операционной системы не являющейся OS Windows
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
		// Выполняем инициализацию unix-сокета
		this->name(AWH_SHORT_NAME);
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
	// Если процесс является родительским
	if(this->_pid == static_cast <pid_t> (::getpid())){
		// Если сокет в файловой системе уже существует, удаляем его
		if(this->_server.fs.isSock(this->_server.ipc))
			// Удаляем файл сокета
			::unlink(this->_server.ipc.c_str());
	}
}
