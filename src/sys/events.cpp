/**
 * @file: events.cpp
 * @date: 2024-06-26
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

/**
 * Подключаем заголовочный файл
 */
#include <sys/events.hpp>

/**
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winsockInitialized Метод проверки на инициализацию WinSocksAPI
	 * @return результат проверки
	 */
	static bool winsockInitialized() noexcept {
		// Выполняем создание сокета
		SOCKET fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		// Если сокет не создан
		if(fd == INVALID_SOCKET)
			// Сообщаем, что сокет не создан а значит WinSocksAPI не инициализирован
			return false;
		// Выполняем закрытие открытого сокета
		::closesocket(fd);
		// Сообщаем, что WinSocksAPI уже инициализирован
		return true;
	}
#endif

/**
 * init Метод инициализации базы событий
 * @param mode флаг инициализации
 */
void awh::Base::init(const bool mode) noexcept {
	// Если база событий необходимо инициализировать
	if(mode){
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если WinSocksAPI ещё не инициализирована
			if(!(this->_winSockInit = winsockInitialized())){
				// Идентификатор ошибки
				int error = 0;
				// Выполняем инициализацию сетевого контекста
				if((error = WSAStartup(MAKEWORD(2, 2), &this->_wsaData)) != 0){
					// Выводим сообщение об ошибке
					this->_log->print("Events base is not init: %s", log_t::flag_t::CRITICAL, this->_socket.message(error).c_str());
					// Очищаем сетевой контекст
					WSACleanup();
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем проверку версии WinSocket
				if((2 != LOBYTE(this->_wsaData.wVersion)) || (2 != HIBYTE(this->_wsaData.wVersion))){
					// Выводим сообщение об ошибке
					this->_log->print("Events base is not init", log_t::flag_t::CRITICAL);
					// Очищаем сетевой контекст
					WSACleanup();
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем инициализацию EPoll
			if((this->_efd = epoll_create(this->_maxCount)) == -1){
				// Выводим сообщение об ошибке
				this->_log->print("Events base is not init: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
				// Выходим принудительно из приложения
				::exit(EXIT_FAILURE);
			}
			// Выполняем открытие файлового дескриптора
			fcntl(this->_efd, F_SETFD, FD_CLOEXEC);
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем инициализацию Kqueue
			if((this->_kq = kqueue()) == -1){
				// Выводим сообщение об ошибке
				this->_log->print("Events base is not init: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
				// Выходим принудительно из приложения
				::exit(EXIT_FAILURE);
			}
			// Выполняем открытие файлового дескриптора
			fcntl(this->_kq, F_SETFD, FD_CLOEXEC);
		#endif
	// Если базу событий необходимо деинициализировать
	} else {
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если WinSocksAPI была инициализированна в этой базе событий
			if(!this->_winSockInit)
				// Очищаем сетевой контекст
				WSACleanup();
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем закрытие подключения
			::close(this->_efd);
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем закрытие подключения
			::close(this->_kq);
		#endif
	}
}
/**
 * del Метод удаления файлового дескриптора из базы событий для всех событий
 * @param fd файловый дескриптор для удаления
 * @return   результат работы функции
 */
bool awh::Base::del(const SOCKET fd) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if(fd > -1){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем закрытие подключения
				::closesocket(fd);
				// Получаем список уже добавленных индексов
				auto ret = this->_indexes.equal_range(fd);
				// Выполняем перебор всех индексов
				for(auto j = ret.first; j != ret.second; ++j){
					// Очищаем полученное событие
					this->_fds.at(j->second).revents = 0;
					// Выполняем сброс файлового дескриптора
					this->_fds.at(j->second).fd = -1;
					// Выполняем удаление события из списка отслеживания
					this->_fds.erase(std::next(this->_fds.begin(), j->second));
				}
				// Выполняем удаление индекса из списка
				this->_indexes.erase(fd);
				// Выполняем удаление всего события
				this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем закрытие подключения
				::close(fd);
				// Получаем список уже добавленных индексов
				auto ret = this->_indexes.equal_range(fd);
				// Выполняем перебор всех индексов
				for(auto j = ret.first; j != ret.second; ++j){
					// Выполняем изменение параметров события
					if(!(result = (epoll_ctl(this->_efd, EPOLL_CTL_DEL, fd, &this->_change.at(j->second)) == 0)))
						// Выводим сообщение об ошибке
						this->_log->print("Remove event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
					// Выполняем удаление события из списка изменений
					this->_change.erase(std::next(this->_change.begin(), j->second));
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(std::next(this->_events.begin(), j->second));
				}
				// Выполняем удаление индекса из списка
				this->_indexes.erase(fd);
				// Выполняем удаление всего события
				this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем закрытие подключения
				::close(fd);
				// Получаем список уже добавленных индексов
				auto ret = this->_indexes.equal_range(fd);
				// Выполняем перебор всех индексов
				for(auto j = ret.first; j != ret.second; ++j){
					// Выполняем отключение работы события
					EV_SET(&this->_change.at(j->second), fd, EVFILT_READ | EVFILT_WRITE, EV_DISABLE, 0, 0, 0);
					// Выполняем очистку объекта события
					EV_SET(&this->_change.at(j->second), fd, EVFILT_READ | EVFILT_WRITE, EV_CLEAR, 0, 0, 0);
					// Выполняем удаление объекта события
					EV_SET(&this->_change.at(j->second), fd, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
					// Выполняем удаление события из списка изменений
					this->_change.erase(std::next(this->_change.begin(), j->second));
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(std::next(this->_events.begin(), j->second));
				}
				// Выполняем удаление индекса из списка
				this->_indexes.erase(fd);
				// Выполняем удаление всего события
				this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * del Метод удаления файлового дескриптора из базы событий для указанного события
 * @param fd   файловый дескриптор для удаления
 * @param type тип отслеживаемого события
 * @return     результат работы функции
 */
bool awh::Base::del(const SOCKET fd, const event_type_t type) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if(fd > -1){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Определяем тип переданного события
				switch(static_cast <uint8_t> (type)){
					// Если событие установлено как отслеживание закрытия подключения
					case static_cast <uint8_t> (event_type_t::CLOSE): {
						// Выполняем поиск типа события и его режим работы
						auto k = i->second.mode.find(type);
						// Если режим работы события получен
						if((result = (k != i->second.mode.end()))){
							// Выполняем отключение работы события
							k->second = event_mode_t::DISABLED;
							// Выполняем удаление типа события
							i->second.mode.erase(k);
						}
					} break;
					// Если событие установлено как отслеживание события чтения из сокета
					case static_cast <uint8_t> (event_type_t::READ):
					// Если событие установлено как отслеживание события записи в сокет
					case static_cast <uint8_t> (event_type_t::WRITE): {
						// Получаем индекс искомого нами типа события
						auto j = i->second.indexes.find(type);
						// Если индекс события найден
						if((result = (j != i->second.indexes.end()))){
							// Выполняем поиск типа события и его режим работы
							auto k = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (k != i->second.mode.end()))){
								// Выполняем отключение работы события
								k->second = event_mode_t::DISABLED;
								// Очищаем полученное событие
								this->_fds.at(j->second).revents = 0;
								// Выполняем сброс файлового дескриптора
								this->_fds.at(j->second).fd = -1;
								// Выполняем удаление события из списка отслеживания
								this->_fds.erase(std::next(this->_fds.begin(), j->second));
								// Выполняем перебор всех индексов
								for(auto l = this->_indexes.find(fd); l != this->_indexes.end(); ++l){
									// Если мы нашли нужный нам файловый дескриптор и индекс совпадает
									if((l->first == fd) && (l->second == j->second)){
										// Выполняем удаление индекса из списка
										this->_indexes.erase(l);
										// Выходим из цикла
										break;
									}
								}
								// Выполняем удаление типа события
								i->second.mode.erase(k);
							}
							// Выполняем удаление индекса из списка
							i->second.indexes.erase(j);
						}
					}
				}
				// Если список режимов событий пустой
				if(i->second.mode.empty())
					// Выполняем удаление всего события
					this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Получаем индекс искомого нами типа события
				auto j = this->_indexes.find(fd);
				// Если индекс события найден
				if((result = (j != this->_indexes.end()))){
					// Выполняем поиск типа события и его режим работы
					auto k = i->second.mode.find(type);
					// Если режим работы события получен
					if((result = (k != i->second.mode.end()))){
						// Выполняем отключение работы события
						k->second = event_mode_t::DISABLED;
						// Определяем тип переданного события
						switch(static_cast <uint8_t> (type)){
							// Если событие установлено как отслеживание закрытия подключения
							case static_cast <uint8_t> (event_type_t::CLOSE):
								// Выполняем удаление флагов отслеживания закрытия подключения
								this->_change.at(j->second).events ^= (EPOLLRDHUP | EPOLLHUP);
							break;
							// Если событие установлено как отслеживание события чтения из сокета
							case static_cast <uint8_t> (event_type_t::READ):
								// Выполняем удаление флагов отслеживания получения данных из сокета
								this->_change.at(j->second).events ^= EPOLLIN;
							break;
							// Если событие установлено как отслеживание события записи в сокет
							case static_cast <uint8_t> (event_type_t::WRITE):
								// Выполняем удаление флагов отслеживания записи данных в сокет
								this->_change.at(j->second).events ^= EPOLLOUT;
							break;
						}
						// Выполняем изменение параметров события
						if(!(result = (epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &this->_change.at(j->second)) == 0)))
							// Выводим сообщение об ошибке
							this->_log->print("Remove event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
						// Выполняем удаление типа события
						i->second.mode.erase(k);
						// Если список режимов событий пустой
						if(i->second.mode.empty()){
							// Выполняем изменение параметров события
							if(!(result = (epoll_ctl(this->_efd, EPOLL_CTL_DEL, fd, &this->_change.at(j->second)) == 0)))
								// Выводим сообщение об ошибке
								this->_log->print("Remove event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
							// Выполняем удаление события из списка отслеживания
							this->_change.erase(std::next(this->_change.begin(), j->second));
							// Определяем тип переданного события
							switch(static_cast <uint8_t> (type)){
								// Если событие установлено как отслеживание закрытия подключения
								case static_cast <uint8_t> (event_type_t::CLOSE): {
									// Результат удаления события
									bool erased = false;
									// Выполняем поиск идентификатора события
									for(auto l = this->_events.begin(); l != this->_events.end(); ++l){
										// Если идентификатор события найден
										if((erased = ((l->data.fd == fd) && (l->events & (EPOLLRDHUP | EPOLLHUP))))){
											// Выполняем удаление события
											this->_events.erase(l);
											// Выходим из цикла
											break;
										}
									}
									// Если событие в списке не найдено
									if(!erased)
										// Удаляем произвольное событие из списка
										this->_events.erase(std::next(this->_events.begin(), j->second));
								} break;
								// Если событие установлено как отслеживание события чтения из сокета
								case static_cast <uint8_t> (event_type_t::READ): {
									// Результат удаления события
									bool erased = false;
									// Выполняем поиск идентификатора события
									for(auto l = this->_events.begin(); l != this->_events.end(); ++l){
										// Если идентификатор события найден
										if((erased = ((l->data.fd == fd) && (l->events & EPOLLIN)))){
											// Выполняем удаление события
											this->_events.erase(l);
											// Выходим из цикла
											break;
										}
									}
									// Если событие в списке не найдено
									if(!erased)
										// Удаляем произвольное событие из списка
										this->_events.erase(std::next(this->_events.begin(), j->second));
								} break;
								// Если событие установлено как отслеживание события записи в сокет
								case static_cast <uint8_t> (event_type_t::WRITE): {
									// Результат удаления события
									bool erased = false;
									// Выполняем поиск идентификатора события
									for(auto l = this->_events.begin(); l != this->_events.end(); ++l){
										// Если идентификатор события найден
										if((erased = ((l->data.fd == fd) && (l->events & EPOLLOUT)))){
											// Выполняем удаление события
											this->_events.erase(l);
											// Выходим из цикла
											break;
										}
									}
									// Если событие в списке не найдено
									if(!erased)
										// Удаляем произвольное событие из списка
										this->_events.erase(std::next(this->_events.begin(), j->second));
								} break;
							}
						}
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty())
						// Выполняем удаление индекса из списка
						this->_indexes.erase(j);
				}
				// Если список режимов событий пустой
				if(i->second.mode.empty())
					// Выполняем удаление всего события
					this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Определяем тип переданного события
				switch(static_cast <uint8_t> (type)){
					// Если событие установлено как отслеживание закрытия подключения
					case static_cast <uint8_t> (event_type_t::CLOSE): {
						// Выполняем поиск типа события и его режим работы
						auto k = i->second.mode.find(type);
						// Если режим работы события получен
						if((result = (k != i->second.mode.end()))){
							// Выполняем отключение работы события
							k->second = event_mode_t::DISABLED;
							// Выполняем удаление типа события
							i->second.mode.erase(k);
						}
					} break;
					// Если событие установлено как отслеживание события чтения из сокета
					case static_cast <uint8_t> (event_type_t::READ):
					// Если событие установлено как отслеживание события записи в сокет
					case static_cast <uint8_t> (event_type_t::WRITE): {
						// Получаем индекс искомого нами типа события
						auto j = i->second.indexes.find(type);
						// Если индекс события найден
						if((result = (j != i->second.indexes.end()))){
							// Выполняем поиск типа события и его режим работы
							auto k = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (k != i->second.mode.end()))){
								// Выполняем отключение работы события
								k->second = event_mode_t::DISABLED;
								// Определяем тип переданного события
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как отслеживание события чтения из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										// Результат удаления события
										bool erased = false;
										// Выполняем отключение работы события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_READ, EV_DISABLE, 0, 0, 0);
										// Выполняем очистку объекта события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_READ, EV_CLEAR, 0, 0, 0);
										// Выполняем удаление объекта события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем сброс файлового дескриптора
										this->_change.at(j->second).ident = -1;
										// Выполняем поиск идентификатора события
										for(auto l = this->_events.begin(); l != this->_events.end(); ++l){
											// Если идентификатор события найден
											if((erased = ((l->ident == fd) && (l->filter & EVFILT_READ)))){
												// Выполняем удаление события
												this->_events.erase(l);
												// Выходим из цикла
												break;
											}
										}
										// Если событие в списке не найдено
										if(!erased)
											// Удаляем произвольное событие из списка
											this->_events.erase(std::next(this->_events.begin(), j->second));
									} break;
									// Если событие установлено как отслеживание события записи в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										// Результат удаления события
										bool erased = false;
										// Выполняем отключение работы события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_WRITE, EV_DISABLE, 0, 0, 0);
										// Выполняем очистку объекта события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_WRITE, EV_CLEAR, 0, 0, 0);
										// Выполняем удаление объекта события
										EV_SET(&this->_change.at(j->second), fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
										// Выполняем сброс файлового дескриптора
										this->_change.at(j->second).ident = -1;
										// Выполняем поиск идентификатора события
										for(auto l = this->_events.begin(); l != this->_events.end(); ++l){
											// Если идентификатор события найден
											if((erased = ((l->ident == fd) && (l->filter & EVFILT_WRITE)))){
												// Выполняем удаление события
												this->_events.erase(l);
												// Выходим из цикла
												break;
											}
										}
										// Если событие в списке не найдено
										if(!erased)
											// Удаляем произвольное событие из списка
											this->_events.erase(std::next(this->_events.begin(), j->second));
									} break;
								}
								// Выполняем удаление события из списка отслеживания
								this->_change.erase(std::next(this->_change.begin(), j->second));
								// Выполняем перебор всех индексов
								for(auto l = this->_indexes.find(fd); l != this->_indexes.end(); ++l){
									// Если мы нашли нужный нам файловый дескриптор и индекс совпадает
									if((l->first == fd) && (l->second == j->second)){
										// Выполняем удаление индекса из списка
										this->_indexes.erase(l);
										// Выходим из цикла
										break;
									}
								}
								// Выполняем удаление типа события
								i->second.mode.erase(k);
							}
							// Выполняем удаление индекса из списка
							i->second.indexes.erase(j);
						}
					} break;
				}
				// Если список режимов событий пустой
				if(i->second.mode.empty())
					// Выполняем удаление всего события
					this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * add Метод добавления файлового дескриптора в базу событий
 * @param fd       файловый дескриптор для добавления
 * @param type     тип отслеживаемого события
 * @param callback функция обратного вызова при получении события
 * @return         результат работы функции
 */
bool awh::Base::add(const SOCKET fd, const event_type_t type, callback_t callback) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if(fd > -1){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Если количество добавленных файловых дескрипторов для отслеживания не достигло предела
		if(this->_items.size() < static_cast <size_t> (this->_maxCount)){
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_items.find(fd);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_items.end()))){
					// Если ещё такой тип события не существует
					if((result = (i->second.mode.find(type) == i->second.mode.end()))){
						// Выключаем установку событий модуля
						i->second.mode.emplace(type, event_mode_t::DISABLED);
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
						// Определяем тип переданного события
						switch(static_cast <uint8_t> (type)){
							// Если событие установлено как отслеживание события чтения из сокета
							case static_cast <uint8_t> (event_type_t::READ): {
								// Устанавливаем файловый дескриптор в список для отслеживания
								this->_fds.push_back(WSAPOLLFD);
								// Выполняем установку файлового дескриптора
								this->_fds.back().fd = fd;
								// Выполняем установку событие для отслеживания
								this->_fds.back().events = POLLIN;
								// Добавляем в список индексов текущий файловй дескриптор
								this->_indexes.emplace(fd, this->_fds.size() - 1);
								// Выполняем установку соответствия индексу типу события
								i->second.indexes.emplace(type, this->_fds.size() - 1);
							} break;
							// Если событие установлено как отслеживание события записи в сокет
							case static_cast <uint8_t> (event_type_t::WRITE): {
								// Устанавливаем файловый дескриптор в список для отслеживания
								this->_fds.push_back(WSAPOLLFD);
								// Выполняем установку файлового дескриптора
								this->_fds.back().fd = fd;
								// Выполняем установку событие для отслеживания
								this->_fds.back().events = POLLOUT;
								// Добавляем в список индексов текущий файловй дескриптор
								this->_indexes.emplace(fd, this->_fds.size() - 1);
								// Выполняем установку соответствия индексу типу события
								i->second.indexes.emplace(type, this->_fds.size() - 1);
							} break;
						}
					}
				// Если файлового дескриптора в базе событий нет
				} else {
					// Выполняем добавление в список параметров для отслеживания
					auto ret = this->_items.emplace(fd, item_t());
					// Выполняем установку файлового дескриптора
					ret.first->second.fd = fd;
					// Выключаем установку событий модуля
					ret.first->second.mode.emplace(type, event_mode_t::DISABLED);
					// Если функция обратного вызова передана
					if(callback != nullptr)
						// Выполняем установку функции обратного вызова
						ret.first->second.callback = callback;
					// Определяем тип переданного события
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Устанавливаем файловый дескриптор в список для отслеживания
							this->_fds.push_back(WSAPOLLFD);
							// Выполняем установку файлового дескриптора
							this->_fds.back().fd = fd;
							// Выполняем установку событие для отслеживания
							this->_fds.back().events = POLLIN;
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_fds.size() - 1);
							// Выполняем установку соответствия индексу типу события
							ret.first->second.indexes.emplace(type, this->_fds.size() - 1);
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Устанавливаем файловый дескриптор в список для отслеживания
							this->_fds.push_back(WSAPOLLFD);
							// Выполняем установку файлового дескриптора
							this->_fds.back().fd = fd;
							// Выполняем установку событие для отслеживания
							this->_fds.back().events = POLLOUT;
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_fds.size() - 1);
							// Выполняем установку соответствия индексу типу события
							ret.first->second.indexes.emplace(type, this->_fds.size() - 1);
						} break;
					}
					// Выводим результат добавления
					result = ret.second;
				}
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_items.find(fd);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_items.end()))){
					// Если ещё такой тип события не существует
					if((result = (i->second.mode.find(type) == i->second.mode.end()))){
						// Выключаем установку событий модуля
						i->second.mode.emplace(type, event_mode_t::DISABLED);
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
						// Получаем список уже добавленных индексов
						auto ret = this->_indexes.equal_range(fd);
						// Выполняем перебор всех индексов
						for(auto j = ret.first; j != ret.second; ++j){
							// Определяем тип переданного события
							switch(static_cast <uint8_t> (type)){
								// Если событие установлено как отслеживание закрытия подключения
								case static_cast <uint8_t> (event_type_t::CLOSE):
									// Выполняем установку флагов отслеживания закрытия подключения
									this->_change.at(j->second).events |= (EPOLLRDHUP | EPOLLHUP);
								break;
								// Если событие установлено как отслеживание события чтения из сокета
								case static_cast <uint8_t> (event_type_t::READ):
									// Выполняем установку флагов отслеживания получения данных из сокета
									this->_change.at(j->second).events |= EPOLLIN;
								break;
								// Если событие установлено как отслеживание события записи в сокет
								case static_cast <uint8_t> (event_type_t::WRITE):
									// Выполняем установку флагов отслеживания записи данных в сокет
									this->_change.at(j->second).events |= EPOLLOUT;
								break;
							}
							// Выполняем изменение параметров события
							if(!(result = (epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &this->_change.at(j->second)) == 0)))
								// Выводим сообщение об ошибке
								this->_log->print("Add event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
						}
					}
				// Если файлового дескриптора в базе событий нет
				} else {
					// Выполняем добавление в список параметров для отслеживания
					auto ret = this->_items.emplace(fd, item_t());
					// Выполняем установку файлового дескриптора
					ret.first->second.fd = fd;
					// Выключаем установку событий модуля
					ret.first->second.mode.emplace(type, event_mode_t::DISABLED);
					// Если функция обратного вызова передана
					if(callback != nullptr)
						// Выполняем установку функции обратного вызова
						ret.first->second.callback = callback;
					// Определяем тип переданного события
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct epoll_event){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct epoll_event){});
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_change.size() - 1);
							// Устанавливаем файловый дескриптор для отслеживания
							this->_change.back().data.fd = fd;
							// Устанавливаем флаг ожидания отключения сокета
							this->_change.back().events = (EPOLLRDHUP | EPOLLHUP | EPOLLERR);
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct epoll_event){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct epoll_event){});
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_change.size() - 1);
							// Устанавливаем файловый дескриптор для отслеживания
							this->_change.back().data.fd = fd;
							// Устанавливаем флаг ожидания чтения данных из сокета
							this->_change.back().events = (EPOLLIN | EPOLLERR);
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct epoll_event){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct epoll_event){});
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_change.size() - 1);
							// Устанавливаем файловый дескриптор для отслеживания
							this->_change.back().data.fd = fd;
							// Устанавливаем флаг ожидания записи данных в сокет
							this->_change.back().events = (EPOLLOUT | EPOLLERR);
						} break;
					}
					// Выполняем изменение параметров события
					if(!(result = (epoll_ctl(this->_efd, EPOLL_CTL_ADD, fd, &this->_change.back()) == 0)))
						// Выводим сообщение об ошибке
						this->_log->print("Add event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
				}
			/**
			 * Если это FreeBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_items.find(fd);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_items.end()))){
					// Если ещё такой тип события не существует
					if((result = (i->second.mode.find(type) == i->second.mode.end()))){
						// Выключаем установку событий модуля
						i->second.mode.emplace(type, event_mode_t::DISABLED);
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
						// Определяем тип переданного события
						switch(static_cast <uint8_t> (type)){
							// Если событие установлено как отслеживание события чтения из сокета
							case static_cast <uint8_t> (event_type_t::READ): {
								// Устанавливаем новый объект для изменений события
								this->_change.push_back((struct kevent){});
								// Устанавливаем новый объект для отслеживания события
								this->_events.push_back((struct kevent){});
								// Добавляем в список индексов текущий файловй дескриптор
								this->_indexes.emplace(fd, this->_change.size() - 1);
								// Выполняем установку соответствия индексу типу события
								i->second.indexes.emplace(type, this->_change.size() - 1);
								// Добавляем файловый дескриптор в список для отслеживания
								EV_SET(&this->_change.back(), fd, EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, 0);
							} break;
							// Если событие установлено как отслеживание события записи в сокет
							case static_cast <uint8_t> (event_type_t::WRITE): {
								// Устанавливаем новый объект для изменений события
								this->_change.push_back((struct kevent){});
								// Устанавливаем новый объект для отслеживания события
								this->_events.push_back((struct kevent){});
								// Добавляем в список индексов текущий файловй дескриптор
								this->_indexes.emplace(fd, this->_change.size() - 1);
								// Выполняем установку соответствия индексу типу события
								i->second.indexes.emplace(type, this->_change.size() - 1);
								// Добавляем файловый дескриптор в список для отслеживания
								EV_SET(&this->_change.back(), fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, 0);
							} break;
						}
					}
				// Если файлового дескриптора в базе событий нет
				} else {
					// Выполняем добавление в список параметров для отслеживания
					auto ret = this->_items.emplace(fd, item_t());
					// Выполняем установку файлового дескриптора
					ret.first->second.fd = fd;
					// Выключаем установку событий модуля
					ret.first->second.mode.emplace(type, event_mode_t::DISABLED);
					// Если функция обратного вызова передана
					if(callback != nullptr)
						// Выполняем установку функции обратного вызова
						ret.first->second.callback = callback;
					// Определяем тип переданного события
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct kevent){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct kevent){});
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_change.size() - 1);
							// Выполняем установку соответствия индексу типу события
							ret.first->second.indexes.emplace(type, this->_change.size() - 1);
							// Добавляем файловый дескриптор в список для отслеживания
							EV_SET(&this->_change.back(), fd, EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, 0);
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct kevent){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct kevent){});
							// Добавляем в список индексов текущий файловй дескриптор
							this->_indexes.emplace(fd, this->_change.size() - 1);
							// Выполняем установку соответствия индексу типу события
							ret.first->second.indexes.emplace(type, this->_change.size() - 1);
							// Добавляем файловый дескриптор в список для отслеживания
							EV_SET(&this->_change.back(), fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, 0);
						} break;
					}
					// Выводим результат добавления
					result = ret.second;
				}
			#endif
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		// Выводим сообщение об ошибке
		} else this->_log->print("SOCKET=%d cannot be added because the number of events being monitored has already reached the limit of %d", log_t::flag_t::WARNING, fd, this->_maxCount);
	}
	// Выводим результат
	return result;
}
/**
 * mode Метод установки режима работы модуля
 * @param fd   файловый дескриптор для установки режима работы
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Base::mode(const SOCKET fd, const event_type_t type, const event_mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if(fd > -1){
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Выполняем поиск файлового дескриптора в базе событий
		auto i = this->_items.find(fd);
		// Если файловый дескриптор есть в базе событий
		if(i != this->_items.end()){
			// Выполняем поиск события модуля
			auto j = i->second.mode.find(type);
			// Если событие для изменения режима работы модуля найдено
			if((result = (j != i->second.mode.end()))){
				// Выполняем установку режима работы модуля
				j->second = mode;
				/**
				 * Если это FreeBSD или MacOS X
				 */
				#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__)
					// Выполняем поиск соответствия индексу типа события
					auto l = i->second.indexes.find(type);
					// Если индекс соответствия найден
					if(l != i->second.indexes.end()){
						// Получаем список уже добавленных индексов
						auto ret = this->_indexes.equal_range(fd);
						// Выполняем перебор всех индексов
						for(auto k = ret.first; k != ret.second; ++k){
							// Если мы нашли наше событие для файлового дескриптора
							if(k->second == l->second){
								// Определяем тип события
								switch(static_cast <uint8_t> (type)){
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&this->_change.at(k->second), fd, EVFILT_READ, EV_ENABLE, 0, 0, 0);
											break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&this->_change.at(k->second), fd, EVFILT_READ, EV_DISABLE, 0, 0, 0);
											break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&this->_change.at(k->second), fd, EVFILT_WRITE, EV_ENABLE, 0, 0, 0);
											break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&this->_change.at(k->second), fd, EVFILT_WRITE, EV_DISABLE, 0, 0, 0);
											break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					}
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * launched Метод проверки запущена ли в данный момент база событий
 * @return результат проверки запущена ли база событий
 */
bool awh::Base::launched() const noexcept {
	// Выполняем проверку запущена ли работа базы событий
	return this->_launched;
}
/**
 * clear Метод очистки списка событий
 */
void awh::Base::clear() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем блокировку чтения базы событий
	this->_locker = true;
	// Выполняем перебор всех файловых дескрипторов
	for(auto i = this->_items.begin(); i != this->_items.end();){
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем закрытие подключения
			::closesocket(i->first);
			// Выполняем перебор всего списка индексов
			for(auto j = this->_indexes.begin(); j != this->_indexes.end();){
				// Очищаем полученное событие
				this->_fds.at(j->second).revents = 0;
				// Выполняем сброс файлового дескриптора
				this->_fds.at(j->second).fd = -1;
				// Выполняем удаление события из списка отслеживания
				this->_fds.erase(std::next(this->_fds.begin(), j->second));
				// Выполняем удаление индекса
				j = this->_indexes.erase(j);
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем закрытие подключения
			::close(i->first);
			// Выполняем перебор всего списка индексов
			for(auto j = this->_indexes.begin(); j != this->_indexes.end();){
				// Выполняем изменение параметров события
				if(epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->first, &this->_change.at(j->second)) != 0)
					// Выводим сообщение об ошибке
					this->_log->print("Remove event SOCKET=%d to event base: %s", log_t::flag_t::CRITICAL, i->first, this->_socket.message().c_str());
				// Выполняем удаление события из списка изменений
				this->_change.erase(std::next(this->_change.begin(), j->second));
				// Выполняем удаление события из списка отслеживания
				this->_events.erase(std::next(this->_events.begin(), j->second));
				// Выполняем удаление индекса
				j = this->_indexes.erase(j);
			}
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем закрытие подключения
			::close(i->first);
			// Выполняем перебор всего списка индексов
			for(auto j = this->_indexes.begin(); j != this->_indexes.end();){
				// Выполняем отключение работы события
				EV_SET(&this->_change.at(j->second), i->first, EVFILT_READ | EVFILT_WRITE, EV_DISABLE, 0, 0, 0);
				// Выполняем очистку объекта события
				EV_SET(&this->_change.at(j->second), i->first, EVFILT_READ | EVFILT_WRITE, EV_CLEAR, 0, 0, 0);
				// Выполняем удаление объекта события
				EV_SET(&this->_change.at(j->second), i->first, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
				// Выполняем удаление события из списка изменений
				this->_change.erase(std::next(this->_change.begin(), j->second));
				// Выполняем удаление события из списка отслеживания
				this->_events.erase(std::next(this->_events.begin(), j->second));
				// Выполняем удаление индекса
				j = this->_indexes.erase(j);
			}
		#endif
		// Выполняем удаление события из базы событий
		i = this->_items.erase(i);
	}
	// Выполняем разблокировку чтения базы событий
	this->_locker = false;
}
/**
 * kick Метод отправки пинка
 */
void awh::Base::kick() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если работа базы событий запущена
	if(this->_mode){
		// Снимаем флаг работы базы событий
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Запускаем ожидание остановки работы базы событий
		while(this->_launched)
			// Ожидаем завершения работы базы событий
			std::this_thread::sleep_for(10ms);
		// Выполняем запуск работы базы событий
		this->start();
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * stop Метод остановки чтения базы событий
 */
void awh::Base::stop() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если работа базы событий запущена
	if(this->_mode){
		// Снимаем флаг работы базы событий
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Выполняем очистку списка событий
		this->clear();
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * start Метод запуска чтения базы событий
 */
void awh::Base::start() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если работа базы событий не запущена
	if(!this->_mode){
		// Устанавливаем флаг работы базы событий
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Переменная опроса события
		int32_t poll = 0;
		// Количество событий для опроса
		size_t count = 0;
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__)
			// Создаём объект временного таймаута
			struct timespec timeout = {0, 0};
			// Если установлен конкретный таймаут
			if((this->_timeout > 0) && !this->_easily){
				// Устанавливаем время в секундах
				timeout.tv_sec = (this->_timeout / 1000);
				// Устанавливаем время счётчика (наносекунды)
				timeout.tv_nsec = (((this->_timeout % 1000) * 1000) * 1000000);
			}
		#endif
		// Устанавливаем флаг запущенного опроса базы событий
		this->_launched = this->_mode;
		// Выполняем запуск базы события
		while(this->_mode){
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если опрос базы событий не заблокирован
				if(!this->_locker){
					// Если в списке достаточно событий для опроса
					if(!this->_fds.empty()){
						// Выполняем опрос базы событий
						poll = WSAPoll(this->_fds.data(), this->_fds.size(), (!this->_easily ? this->_timeout : 0));
						// Если мы получили ошибку
						if(poll == SOCKET_ERROR)
							// Выводим сообщение об ошибке
							this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
						// Если сработал таймаут
						else if(poll == 0) {
							// Компенсируем условие
							poll = 0;
						// Если опрос прошёл успешно
						} else {
							// Выполняем блокировку потока
							const lock_guard <std::recursive_mutex> lock(this->_mtx);
							// Получаем количество файловых дескрипторов для проверки
							count = this->_fds.size();
							// Выполняем перебор всех файловых дескрипторов
							for(size_t i = 0; i < count; i++){
								// Если записей достаточно в списке
								if(i < this->_fds.size()){
									// Если произошёл дисконнект
									if(this->_fds.at(i).revents & POLLHUP){
										// Получаем файловый дескриптор
										SOCKET fd = this->_fds.at(i).fd;
										// Выполняем поиск указанной записи
										auto j = this->_items.find(fd);
										// Если сокет в списке найден
										if(j != this->_items.end()){
											// Если функция обратного вызова установлена
											if(j->second.callback != nullptr){
												// Выполняем поиск события на отключение присутствует в базе событий
												auto k = j->second.mode.find(event_type_t::CLOSE);
												// Если событие найдено и оно активированно
												if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
													// Выполняем функцию обратного вызова
													j->second.callback(fd, event_type_t::CLOSE);
											}
										// Выводим сообщение об ошибке
										} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
									// Если дисконнекта не получилось
									} else {
										// Если в сокете появились данные для чтения
										if(this->_fds.at(i).revents & POLLIN) {
											// Получаем файловый дескриптор
											SOCKET fd = this->_fds.at(i).fd;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на получение данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::READ);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::READ);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
										// Если сокет доступен для записи
										if(this->_fds.at(i).revents & POLLOUT){
											// Получаем файловый дескриптор
											SOCKET fd = this->_fds.at(i).fd;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::WRITE);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
									}
									// Обнуляем количество событий
									this->_fds.at(i).revents = 0;
								// Выходим из цикла
								} else break;
							}
						}
						// Если активирован простой режим работы чтения базы событий
						if(this->_easily){
							// Если время установленно
							if(this->_timeout > 0)
								// Выполняем задержку времени на указанное количество времени
								std::this_thread::sleep_for(chrono::milliseconds(this->_timeout));
							// Устанавливаем задержку времени по умолчанию
							else std::this_thread::sleep_for(10ms);
						}
					// Замораживаем поток на период времени частоты обновления базы событий
					} else std::this_thread::sleep_for(100ms);
				}
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Если опрос базы событий не заблокирован
				if(!this->_locker){
					// Если в списке достаточно событий для опроса
					if(!this->_change.empty()){
						// Выполняем опрос базы событий
						poll = epoll_wait(this->_efd, this->_events.data(), this->_maxCount, (!this->_easily ? this->_timeout : 0));
						// Если мы получили ошибку
						if(poll == INVALID_SOCKET)
							// Выводим сообщение об ошибке
							this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
						// Если сработал таймаут
						else if(poll == 0) {
							// Компенсируем условие
							poll = 0;
						// Если опрос прошёл успешно
						} else {
							// Выполняем блокировку потока
							const lock_guard <std::recursive_mutex> lock(this->_mtx);
							// Выполняем перебор всех событий в которых мы получили изменения
							for(int32_t i = 0; i < poll; i++){
								// Если записей достаточно в списке
								if(static_cast <size_t> (i) < this->_events.size()){
									// Если мы получили ошибку
									if(this->_events.at(i).events & EPOLLERR)
										// Выводим сообщение об ошибке
										this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
									// Если произошло отключение сокета
									else if(this->_events.at(i).events & (EPOLLRDHUP | EPOLLHUP)) {
										// Получаем файловый дескриптор
										SOCKET fd = this->_events.at(i).data.fd;
										// Выполняем поиск указанной записи
										auto j = this->_items.find(fd);
										// Если сокет в списке найден
										if(j != this->_items.end()){
											// Если функция обратного вызова установлена
											if(j->second.callback != nullptr){
												// Выполняем поиск события на отключение присутствует в базе событий
												auto k = j->second.mode.find(event_type_t::CLOSE);
												// Если событие найдено и оно активированно
												if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
													// Выполняем функцию обратного вызова
													j->second.callback(fd, event_type_t::CLOSE);
											}
										// Выводим сообщение об ошибке
										} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
									// Если дисконнекта не получилось
									} else {
										// Если в сокете появились данные для чтения
										if(this->_events.at(i).events & EPOLLIN){
											// Получаем файловый дескриптор
											SOCKET fd = this->_events.at(i).data.fd;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на получение данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::READ);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::READ);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
										// Если сокет доступен для записи
										if(this->_events.at(i).events & EPOLLOUT){
											// Получаем файловый дескриптор
											SOCKET fd = this->_events.at(i).data.fd;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::WRITE);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
									}
								// Выходим из цикла
								} else break;
							}
						}
						// Если активирован простой режим работы чтения базы событий
						if(this->_easily){
							// Если время установленно
							if(this->_timeout > 0)
								// Выполняем задержку времени на указанное количество времени
								std::this_thread::sleep_for(chrono::milliseconds(this->_timeout));
							// Устанавливаем задержку времени по умолчанию
							else std::this_thread::sleep_for(10ms);
						}
					// Замораживаем поток на период времени частоты обновления базы событий
					} else std::this_thread::sleep_for(100ms);
				}
			/**
			 * Если это FreeBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__
				// Если опрос базы событий не заблокирован
				if(!this->_locker){
					// Если в списке достаточно событий для опроса
					if(!this->_change.empty()){
						// Выполняем опрос базы событий
						poll = kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_timeout > -1) || this->_easily ? &timeout : nullptr));
						// Если мы получили ошибку
						if(poll == INVALID_SOCKET)
							// Выводим сообщение об ошибке
							this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
						// Если сработал таймаут
						else if(poll == 0) {
							// Компенсируем условие
							poll = 0;
						// Если опрос прошёл успешно
						} else {
							// Выполняем блокировку потока
							const lock_guard <std::recursive_mutex> lock(this->_mtx);
							// Выполняем перебор всех событий в которых мы получили изменения
							for(int32_t i = 0; i < poll; i++){
								// Если записей достаточно в списке
								if(static_cast <size_t> (i) < this->_events.size()){
									// Если мы получили ошибку
									if(this->_events.at(i).flags & EV_ERROR)
										// Выводим сообщение об ошибке
										this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message(this->_events.at(i).data).c_str());
									// Если произошло отключение сокета
									else if(this->_events.at(i).flags & EV_EOF) {
										// Получаем файловый дескриптор
										SOCKET fd = this->_events.at(i).ident;
										// Выполняем поиск указанной записи
										auto j = this->_items.find(fd);
										// Если сокет в списке найден
										if(j != this->_items.end()){
											// Если функция обратного вызова установлена
											if(j->second.callback != nullptr){
												// Выполняем поиск события на отключение присутствует в базе событий
												auto k = j->second.mode.find(event_type_t::CLOSE);
												// Если событие найдено и оно активированно
												if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
													// Выполняем функцию обратного вызова
													j->second.callback(fd, event_type_t::CLOSE);
											}
										// Выводим сообщение об ошибке
										} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
									// Если дисконнекта не получилось
									} else {
										// Если в сокете появились данные для чтения
										if(this->_events.at(i).filter & EVFILT_READ){
											// Получаем файловый дескриптор
											SOCKET fd = this->_events.at(i).ident;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на получение данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::READ);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::READ);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
										// Если сокет доступен для записи
										if(this->_events.at(i).filter & EVFILT_WRITE){
											// Получаем файловый дескриптор
											SOCKET fd = this->_events.at(i).ident;
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::WRITE);
												}
											// Выводим сообщение об ошибке
											} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
									}
								// Выходим из цикла
								} else break;
							}
						}
						// Если активирован простой режим работы чтения базы событий
						if(this->_easily){
							// Если время установленно
							if(this->_timeout > 0)
								// Выполняем задержку времени на указанное количество времени
								std::this_thread::sleep_for(chrono::milliseconds(this->_timeout));
							// Устанавливаем задержку времени по умолчанию
							else std::this_thread::sleep_for(10ms);
						}
					// Замораживаем поток на период времени частоты обновления базы событий
					} else std::this_thread::sleep_for(100ms);
				}
			#endif
		}
		// Снимаем флаг запущенного опроса базы событий
		this->_launched = this->_mode;
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Base::rebase() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если работа базы событий запущена
	if(this->_mode){
		// Снимаем флаг работы базы событий
		this->_mode = !this->_mode;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Запускаем ожидание остановки работы базы событий
		while(this->_launched)
			// Ожидаем завершения работы базы событий
			std::this_thread::sleep_for(10ms);
		// Выполняем очистку всех параметров
		this->clear();
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Выполняем деинициализацию базы событий
		this->init(false);
		// Выполняем инициализацию базы событий
		this->init(true);
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Выполняем запуск работы базы событий
		this->start();
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Base::freeze(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем активацию блокировки
	this->_locker = mode;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации
 */
void awh::Base::easily(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Выполняем установку флага активации простого режима чтения базы событий
	this->_easily = mode;
	// Если активирован простой режим работы чтения базы событий
	if(!this->_easily)
		// Выполняем сброс таймаута
		this->_timeout = -1;
}
/**
 * frequency Метод установки частоты обновления базы событий
 * @param msec частота обновления базы событий в миллисекундах
 */
void awh::Base::frequency(const uint32_t msec) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если количество секунд передано верно
	if(msec > 0)
		// Выполняем установку времени ожидания
		this->_timeout = static_cast <int32_t> (msec);
	// Выполняем сброс таймаута
	else this->_timeout = -1;
}
/**
 * Base Конструктор
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 * @param count максимальное количество обрабатываемых сокетов
 */
awh::Base::Base(const fmk_t * fmk, const log_t * log, const uint32_t count) noexcept :
 _mode(false), _easily(false), _locker(false), _launched(false), _timeout(-1), _maxCount(count), _socket(fmk, log), _fmk(fmk), _log(log) {
	// Выполняем инициализацию базы событий
	this->init(true);
}
/**
 * ~Base Деструктор
 */
awh::Base::~Base() noexcept {
	// Выполняем деинициализацию базы событий
	this->init(false);
}
