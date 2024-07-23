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
/**
 * Если это FreeBSD или MacOS X
 */
// #elif __APPLE__ || __MACH__ || __FreeBSD__
	/**
	 * redistribution Метод перераспределения таймеров
	 */
	/*
	void awh::Base::redistribution() noexcept {
		// Если список активных таймеров не пустой
		if(!this->_timers.empty()){
			// Значение текущего индекса
			size_t index = 0;
			// Выполняем перебор всего списка активных таймеров
			for(auto i = this->_timers.begin(); i != this->_timers.end();){
				// Если событие пришло от таймера
				if(i->second->filter & EVFILT_TIMER){
					// Получаем объект текущего события
					item_t * item = reinterpret_cast <item_t *> (i->second->udata);
					// Если объект текущего события получен
					if(item != nullptr){
						// Запоминаем идентификатор файлового дескриптора
						SOCKET fd = item->fd;
						// Определяем сколько прошло времени
						const time_t elapsed = (this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS) - item->date);
						// Определяем вышло ли время выделенное для ожидания
						const bool timed = (((i->second->data > elapsed) ? (i->second->data - elapsed) : 0) == 0);
						// Если время вышло
						if(timed)
							// Выполняем смену режима работы отлова события
							EV_SET(i->second, fd, EVFILT_TIMER, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, 0);
						// Если функция обратного вызова установлена
						if(timed && (item->callback != nullptr)){
							// Выполняем поиск события таймера присутствует в базе событий
							auto j = item->mode.find(event_type_t::TIMER);
							// Если событие найдено и оно активированно
							if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
								// Выполняем функцию обратного вызова
								item->callback(item->fd, event_type_t::TIMER);
						}
						// Если таймеров в списке больше нет
						if(this->_timers.empty())
							// Выходим из цикла
							break;
						// Если список таймеров ещё не пустой
						else {
							// Выполняем поиск идентификатор файлового дескриптора
							auto j = this->_timers.find(fd);
							// Если таймер найден в списке
							if(j != this->_timers.end()){
								// Обновляем значение текущей даты
								item->date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
								// Если время ещё не вышло
								if(!timed)
									// Выполняем установку оставшегося времени
									j->second->data -= elapsed;
								// Если время уже вышло
								else
									// Выполняем смену режима работы отлова события
									EV_SET(j->second, fd, EVFILT_TIMER, EV_ADD | EV_CLEAR | EV_ENABLE, NOTE_NSECONDS, item->delay * 1000000, item);
								// Увеличиваем значение текущего индекса
								index++;
								// Выполняем смещение в цикле
								++i;
							// Продолжаем перебор дальше
							} else i = std::next(this->_timers.begin(), index);
						}
					// Выводим сообщение об ошибке
					} else this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, i->second->ident);
				}
			}
		}
	}
	*/
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
				int32_t error = 0;
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
			if((this->_efd = ::epoll_create(this->_maxCount)) == INVALID_SOCKET){
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
			if((this->_kq = kqueue()) == INVALID_SOCKET){
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
 * @param id идентификатор записи
 * @param fd файловый дескриптор для удаления
 * @return   результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET fd) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_fds.begin(); j != this->_fds.end(); ++j){
					// Если файловый дескриптор найден
					if(j->fd == fd){
						// Очищаем полученное событие
						j->revents = 0;
						// Если событие является таймером
						if(i->second.delay > 0){
							// Выполняем закрытие подключения
							::_close(i->second.fd);
							// Выполняем сброс файлового дескриптора
							j->fd = INVALID_SOCKET;
						// Если событие является обычным
						} else {
							// Выполняем закрытие подключения
							::closesocket(j->fd);
							// Выполняем сброс файлового дескриптора
							j->fd = INVALID_SOCKET;
						}
						// Выполняем удаление события из списка отслеживания
						this->_fds.erase(j);
						// Выходим из цикла
						break;
					}
				}
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
			if((result = (i != this->_items.end()) && (i->second.id == id))){
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если файловый дескриптор найден
					if((reinterpret_cast <item_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <item_t *> (j->data.ptr)->id == id)){
						// Выполняем изменение параметров события
						result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.fd, &(* j)) == 0);
						// Выполняем закрытие подключения
						::close(i->second.fd);
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если файловый дескриптор найден
					if((reinterpret_cast <item_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <item_t *> (j->data.ptr)->id == id)){
						// Если событие ещё не удалено из базы событий
						if(!erased){
							// Выполняем изменение параметров события
							result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.fd, &(* j)) == 0);
							// Выполняем закрытие подключения
							::close(i->second.fd);
						}
						// Выполняем удаление события из списка изменений
						this->_change.erase(j);
						// Выходим из цикла
						break;
					}
				}
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
			if((result = (i != this->_items.end()) && (i->second.id == id))){
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если файловый дескриптор найден
					if((erased = (j->ident == fd))){
						// Если событие является таймером
						if(i->second.delay > 0){
							// Выполняем удаление события таймера
							EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(i->second.fd);
							// Выполняем закрытие таймера
							::close(i->second.timer);
						// Если событие является обычным
						} else {
							// Выполняем удаление объекта события
							EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(j->ident);
						}
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если файловый дескриптор найден
					if(j->ident == fd){
						// Если событие ещё не удалено из базы событий
						if(!erased){
							// Если событие является таймером
							if(i->second.delay > 0){
								// Выполняем удаление события таймера
								EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
								// Выполняем закрытие подключения
								::close(i->second.fd);
								// Выполняем закрытие таймера
								::close(i->second.timer);
							// Если событие является обычным
							} else {
								// Выполняем удаление объекта события
								EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
								// Выполняем закрытие подключения
								::close(j->ident);
							}
						}
						// Выполняем удаление события из списка изменений
						this->_change.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_items.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("Del event in event base: %s", log_t::flag_t::CRITICAL, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * del Метод удаления файлового дескриптора из базы событий для указанного события
 * @param id   идентификатор записи
 * @param fd   файловый дескриптор для удаления
 * @param type тип отслеживаемого события
 * @return     результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET fd, const event_type_t type) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((fd != INVALID_SOCKET) || (type == event_type_t::TIMER)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_items.find(fd);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_items.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					// Определяем тип переданного события
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как таймер
						case static_cast <uint8_t> (event_type_t::TIMER): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_fds.begin(); k != this->_fds.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->fd == fd))){
										// Очищаем полученное событие
										k->revents = 0;
										// Выполняем закрытие подключения
										::_close(i->second.fd);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка отслеживания
										this->_fds.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_fds.begin(); k != this->_fds.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->fd == fd))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end()))
											// Выполняем удаление события из списка отслеживания
											this->_fds.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_fds.begin(); k != this->_fds.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->fd == fd))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на запись
										k->events ^= POLLOUT;
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end()))
											// Выполняем удаление события из списка отслеживания
											this->_fds.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						}
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end())))
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
				if((result = (i != this->_items.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					// Выполняем поиск типа события и его режим работы
					auto j = i->second.mode.find(type);
					// Если режим работы события получен
					if((result = (j != i->second.mode.end()))){
						// Флаг удалённого события из базы событий
						bool erased = false;
						// Выполняем отключение работы события
						j->second = event_mode_t::DISABLED;
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если файловый дескриптор найден
							if((erased = (reinterpret_cast <item_t *> (k->data.ptr) == &i->second))){
								// Определяем тип переданного события
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER):
										// Выполняем удаление флагов отслеживания таймера
										k->events ^= (EPOLLIN | EPOLLET);
									break;
									// Если событие установлено как отслеживание закрытия подключения
									case static_cast <uint8_t> (event_type_t::CLOSE):
										// Выполняем удаление флагов отслеживания закрытия подключения
										k->events ^= (EPOLLRDHUP | EPOLLHUP);
									break;
									// Если событие установлено как отслеживание события чтения из сокета
									case static_cast <uint8_t> (event_type_t::READ):
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= EPOLLIN;
									break;
									// Если событие установлено как отслеживание события записи в сокет
									case static_cast <uint8_t> (event_type_t::WRITE):
										// Выполняем удаление флагов отслеживания записи данных в сокет
										k->events ^= EPOLLOUT;
									break;
								}
								// Выполняем удаление типа события
								i->second.mode.erase(j);
								// Если список режимов событий пустой
								if(i->second.mode.empty()){
									// Выполняем изменение параметров события
									result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.fd, &(* k)) == 0);
									// Выполняем удаление события из списка изменений
									this->_change.erase(k);
								// Выполняем изменение параметров события
								} else result = (::epoll_ctl(this->_efd, EPOLL_CTL_MOD, i->second.fd, &(* k)) == 0);
								// Если событие является таймером
								if(i->second.delay > 0)
									// Выполняем закрытие подключения
									::close(i->second.fd);
								// Выходим из цикла
								break;
							}
						}
						// Если удаление события небыло произведено
						if(!erased)
							// Выполняем удаление типа события
							i->second.mode.erase(j);
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty()){
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
							// Если файловый дескриптор найден
							if((reinterpret_cast <item_t *> (k->data.ptr) == &i->second) &&
							   (reinterpret_cast <item_t *> (k->data.ptr)->id == id)){
								// Если событие является таймером
								if(i->second.delay > 0)
									// Выполняем закрытие подключения
									::close(i->second.fd);
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
								// Выходим из цикла
								break;
							}
						}
						// Выполняем удаление всего события
						this->_items.erase(i);
					}
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
				if((result = (i != this->_items.end()) && (i->second.id == id))){
					// Выполняем блокировку чтения базы событий
					this->_locker = true;
					// Определяем тип переданного события
					switch(static_cast <uint8_t> (type)){
						// Если событие установлено как отслеживание закрытия подключения
						case static_cast <uint8_t> (event_type_t::CLOSE): {
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем удаление типа события
								i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как таймер
						case static_cast <uint8_t> (event_type_t::TIMER): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->ident == fd))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем закрытие подключения
										::close(i->second.fd);
										// Выполняем закрытие таймера
										::close(i->second.timer);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка изменений
										this->_change.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события чтения из сокета
						case static_cast <uint8_t> (event_type_t::READ): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->ident == fd))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, &i->second);
										// Выполняем поиск типа события
										auto l = i->second.mode.find(event_type_t::WRITE);
										// Если режим записи данных найден и он активирован
										if((l != i->second.mode.end()) && (l->second == event_mode_t::ENABLED))
											// Выполняем активацию события на запись
											EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end()))
											// Выполняем удаление события из списка изменений
											this->_change.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
						// Если событие установлено как отслеживание события записи в сокет
						case static_cast <uint8_t> (event_type_t::WRITE): {
							// Флаг удалённого события из базы событий
							bool erased = false;
							// Выполняем поиск типа события и его режим работы
							auto j = i->second.mode.find(type);
							// Если режим работы события получен
							if((result = (j != i->second.mode.end()))){
								// Выполняем отключение работы события
								j->second = event_mode_t::DISABLED;
								// Выполняем поиск файлового дескриптора из списка событий
								for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (k->ident == fd))){
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, &i->second);
										// Выполняем поиск типа события
										auto l = i->second.mode.find(event_type_t::READ);
										// Если режим чтения данных найден и он активирован
										if((l != i->second.mode.end()) && (l->second == event_mode_t::ENABLED))
											// Выполняем активацию события на чтение
											EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end()))
											// Выполняем удаление события из списка изменений
											this->_change.erase(k);
										// Выходим из цикла
										break;
									}
								}
								// Если удаление события небыло произведено
								if(!erased)
									// Выполняем удаление типа события
									i->second.mode.erase(j);
							}
						} break;
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end()))){
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
							// Если файловый дескриптор найден
							if(k->ident == fd){
								// Если событие является таймером
								if(i->second.delay > 0){
									// Выполняем удаление события таймера
									EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
									// Выполняем закрытие подключения
									::close(i->second.fd);
									// Выполняем закрытие таймера
									::close(i->second.timer);
								// Выполняем полное удаление события из базы событий
								} else EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
								// Выходим из цикла
								break;
							}
						}
						// Выполняем удаление всего события
						this->_items.erase(i);
					}
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение об ошибке
			this->_log->print("Del event in event base: %s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
/**
 * add Метод добавления файлового дескриптора в базу событий
 * @param id       идентификатор записи
 * @param fd       файловый дескриптор для добавления
 * @param callback функция обратного вызова при получении события
 * @param delay    задержка времени для создания таймеров
 * @param series   флаг серийного таймаута
 * @return         результат работы функции
 */
bool awh::Base::add(const uint64_t id, SOCKET & fd, callback_t callback, const time_t delay, const bool series) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((fd != INVALID_SOCKET) || (delay > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
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
					if((result = (i != this->_items.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						item_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Создаём объект пайпа
							auto pipe = shared_ptr <pipe_t> (new pipe_t(this->_fmk, this->_log));
							// Устанавливаем тип пайпа
							pipe->type(pipe_t::type_t::NETWORK);
							// Выполняем создание сокетов
							auto fds = pipe->create();
							// Выполняем инициализацию таймера
							if(fds[0] == INVALID_SOCKET)
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							fd = fds[0];
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_items.emplace(fd, item_t());
							// Выполняем установку объекта пайпа
							ret.first->second.pipe = pipe;
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_items.emplace(fd, item_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Выполняем установку файлового дескриптора события
							item->fd = fd;
							// Устанавливаем идентификатор записи
							item->id = id;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем файловый дескриптор в список для отслеживания
							this->_fds.push_back((WSAPOLLFD){});
							// Выполняем установку файлового дескриптора
							this->_fds.back().fd = fd;
						}
					}
				/**
				 * Если это Linux
				 */
				#elif __linux__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_items.find(fd);
					// Если файловый дескриптор есть в базе событий
					if((result = (i != this->_items.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание нового таймера
							fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
							// Если таймер не создан
							if(fd == INVALID_SOCKET){
								// Выводим сообщение об ошибке
								this->_log->print("Add event timer to event base: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								// Выходим из функции
								return result;
							}
						}
						// Выполняем добавление в список параметров для отслеживания
						auto ret = this->_items.emplace(fd, item_t());
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Получаем переданное количество секунд с дробной частью
							const double seconds = (delay / static_cast <double> (1000));
							// Получаем количество остатка наносекунд
							const uint64_t nanoSeconds = ((seconds >= 1.) ? (::fmod(seconds, static_cast <int32_t> (seconds)) * 1000000000) : (seconds * 1000000000));
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем установку количества наносекунд для начального таймера
							ret.first->second.timer.it_value.tv_nsec = nanoSeconds;
							// Выполняем установку количества секунд для начального таймера
							ret.first->second.timer.it_value.tv_sec = static_cast <uint32_t> (seconds);
							// Если таймер является повторяющимся
							if(ret.first->second.series){
								// Выполняем установку количества наносекунд для повторяющегося таймера
								ret.first->second.timer.it_interval.tv_nsec = nanoSeconds;
								// Выполняем установку количества секунд для повторяющегося таймера
								ret.first->second.timer.it_interval.tv_sec = static_cast <uint32_t> (seconds);
							// Если таймер одноразовый
							} else {
								// Выполняем установку количества наносекунд для повторяющегося таймера
								ret.first->second.timer.it_interval.tv_nsec = 0;
								// Выполняем установку количества секунд для повторяющегося таймера
								ret.first->second.timer.it_interval.tv_sec = 0;
							}
							// Выполняем активацию таймера
							if(::timerfd_settime(fd, 0, &ret.first->second.timer, nullptr) == -1){
								// Выводим сообщение об ошибке
								this->_log->print("Activation event timer in event base: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								// Выходим из функции
								return result;
							}
						// Если необходимо создать обычное событие
						} else {
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED}
							};
						}
						// Выполняем установку файлового дескриптора
						ret.first->second.fd = fd;
						// Устанавливаем идентификатор записи
						ret.first->second.id = id;
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							ret.first->second.callback = callback;
						// Устанавливаем новый объект для изменений события
						this->_change.push_back((struct epoll_event){});
						// Устанавливаем новый объект для отслеживания события
						this->_events.push_back((struct epoll_event){});
						// Устанавливаем флаг ожидания отключения сокета
						this->_change.back().events = EPOLLERR;
						// Выполняем установку указателя на основное событие
						this->_change.back().data.ptr = &ret.first->second;
						// Выполняем изменение параметров события
						if(!(result = (::epoll_ctl(this->_efd, EPOLL_CTL_ADD, fd, &this->_change.back()) == 0)))
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
					if((result = (i != this->_items.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						item_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Создаём объект пайпа
							auto pipe = shared_ptr <pipe_t> (new pipe_t(this->_fmk, this->_log));
							// Устанавливаем тип пайпа
							pipe->type(pipe_t::type_t::NATIVE);
							// Выполняем создание сокетов
							auto fds = pipe->create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							fd = fds[0];
							// Делаем сокет неблокирующим
							this->_socket.blocking(fd, socket_t::mode_t::DISABLE);
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_items.emplace(fd, item_t());
							// Выполняем установку объекта пайпа
							ret.first->second.pipe = pipe;
							// Выполняем установку файлового дескриптора таймера
							ret.first->second.timer = fds[1];
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_items.emplace(fd, item_t());
							// Выключаем установку событий модуля
							ret.first->second.mode = {
								{event_type_t::READ, event_mode_t::DISABLED},
								{event_type_t::WRITE, event_mode_t::DISABLED},
								{event_type_t::CLOSE, event_mode_t::DISABLED}
							};
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						}
						// Если объект текущего события получен
						if((result = (item != nullptr))){
							// Выполняем установку файлового дескриптора события
							item->fd = fd;
							// Устанавливаем идентификатор записи
							item->id = id;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем новый объект для изменений события
							this->_change.push_back((struct kevent){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct kevent){});
							// Выполняем заполнение нулями всю структуру изменений
							::memset(&this->_change.back(), 0, sizeof(this->_change.back()));
							// Выполняем заполнение нулями всю структуру событий
							::memset(&this->_events.back(), 0, sizeof(this->_events.back()));
							// Устанавливаем идентификатор файлового дескриптора
							this->_change.back().ident = fd;
							// Выполняем смену режима работы отлова события
							EV_SET(&this->_change.back(), this->_change.back().ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, item);
						}
					}
				#endif
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			// Выводим сообщение об ошибке
			} else this->_log->print("SOCKET=%d cannot be added because the number of events being monitored has already reached the limit of %d", log_t::flag_t::WARNING, fd, this->_maxCount);
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение об ошибке
			this->_log->print("Add event to event base: %s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
/**
 * mode Метод установки режима работы модуля
 * @param id   идентификатор записи
 * @param fd   файловый дескриптор для установки режима работы
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Base::mode(const uint64_t id, const SOCKET fd, const event_type_t type, const event_mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((fd != INVALID_SOCKET) || (type == event_type_t::TIMER)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <std::recursive_mutex> lock(this->_mtx);
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((i != this->_items.end()) && (i->second.id == id)){
				// Выполняем поиск события модуля
				auto j = i->second.mode.find(type);
				// Если событие для изменения режима работы модуля найдено
				if((result = ((j != i->second.mode.end()) && (j->second != mode)))){
					// Выполняем установку режима работы модуля
					j->second = mode;
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_fds.begin(); k != this->_fds.end(); ++k){
							// Если файловый дескриптор найден
							if(k->fd == fd){
								// Очищаем полученное событие
								k->revents = 0;
								// Определяем тип события
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLIN;
												// Выполняем активацию таймера на указанное время
												this->_timeout.set(i->second.fd, i->second.delay * 1000000, i->second.pipe->port());
											} break;
											// Если нужно деактивировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= POLLIN;
												// Выполняем деактивацию таймера
												this->_timeout.del(i->second.fd);
											} break;
										}
									} break;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= POLLIN;
											break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED):
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= POLLIN;
											break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Устанавливаем флаг отслеживания записи данных в сокет
												k->events |= POLLOUT;
											break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED):
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= POLLOUT;
											break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					/**
					 * Если это Linux
					 */
					#elif __linux__
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если файловый дескриптор найден
							if(reinterpret_cast <item_t *> (k->data.ptr)->fd == fd){
								// Определяем тип события
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode enable event TIMER for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
											// Если нужно деактивировать событие таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode disable event TIMER for SOCKET=%d", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
										}
									} break;
									// Если событие установлено как отслеживание закрытия подключения
									case static_cast <uint8_t> (event_type_t::CLOSE): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Выполняем установку флагов отслеживания закрытия подключения
												k->events |= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode enable event CLOSE for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем удаление флагов отслеживания закрытия подключения
												k->events ^= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode disable event CLOSE for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
										}
									} break;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode enable event READ for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode disable event READ for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг отслеживания записи данных в сокет
												k->events |= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode enable event WRITE for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0)
													// Выводим сообщение об ошибке
													this->_log->print("Mode disable event WRITE for SOCKET=%d: %s", log_t::flag_t::CRITICAL, fd, this->_socket.message().c_str());
											} break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					/**
					 * Если это FreeBSD или MacOS X
					 */
					#elif __APPLE__ || __MACH__ || __FreeBSD__
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если файловый дескриптор найден
							if(k->ident == fd){
								// Определяем тип события
								switch(static_cast <uint8_t> (type)){
									// Если событие установлено как таймер
									case static_cast <uint8_t> (event_type_t::TIMER): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												// Выполняем активацию таймера на указанное время
												this->_timeout.set(i->second.timer, i->second.delay * 1000000);
											} break;
											// Если нужно деактивировать событие работы таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
												// Выполняем деактивацию таймера
												this->_timeout.del(i->second.timer);
											} break;
										}
									} break;
									// Если событие является чтением данных из сокета
									case static_cast <uint8_t> (event_type_t::READ): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
											break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
												// Если событие на запись включено
												if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED)
													// Выполняем активацию события на запись
													EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
											} break;
										}
									} break;
									// Если событие является записи данных в сокет
									case static_cast <uint8_t> (event_type_t::WRITE): {
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::ENABLED):
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
											break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем смену режима работы отлова события
												EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
												// Если событие на чтение включено
												if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED)
													// Выполняем активацию события на чтение
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
											} break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					#endif
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Выводим сообщение об ошибке
			this->_log->print("Set mode event base: %s", log_t::flag_t::CRITICAL, error.what());
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
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_fds.begin(); i != this->_fds.end();){
				// Очищаем полученное событие
				i->revents = 0;
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_items.find(i->fd);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_items.end()){
					// Если событие является таймером
					if(j->second.delay > 0){
						// Выполняем закрытие подключения
						::_close(j->second.fd);
						// Выполняем сброс файлового дескриптора
						i->fd = INVALID_SOCKET;
					// Если событие является обычным
					} else {
						// Выполняем закрытие подключения
						::closesocket(i->fd);
						// Выполняем сброс файлового дескриптора
						i->fd = INVALID_SOCKET;
					}
				}
				// Выполняем удаление события из списка отслеживания
				i = this->_fds.erase(i);
			}
		/**
		 * Если это Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем изменение параметров события
				::epoll_ctl(this->_efd, EPOLL_CTL_DEL, reinterpret_cast <item_t *> (i->data.ptr)->fd, &(* i));
				// Выполняем закрытие подключения
				::close(reinterpret_cast <item_t *> (i->data.ptr)->fd);
				// Выполняем удаление события из списка изменений
				i = this->_change.erase(i);
			}
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();)
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
		/**
		 * Если это FreeBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_items.find(i->ident);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_items.end()){
					// Если событие является таймером
					if(j->second.delay > 0){
						// Выполняем удаление события таймера
						EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(j->second.fd);
						// Выполняем закрытие таймера
						::close(j->second.timer);
					// Если событие является обычным
					} else {
						// Выполняем удаление объекта события
						EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(i->ident);
					}
					// Выполняем удаление события
					this->_items.erase(j);
				}
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_items.find(i->ident);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_items.end()){
					// Если событие является таймером
					if(j->second.delay > 0){
						// Выполняем удаление события таймера
						EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(j->second.fd);
						// Выполняем закрытие таймера
						::close(j->second.timer);
					// Если событие является обычным
					} else {
						// Выполняем удаление объекта события
						EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(i->ident);
					}
					// Выполняем удаление события
					this->_items.erase(j);
				}
				// Выполняем удаление события из списка изменений
				i = this->_change.erase(i);
			}
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("Clear event base: %s", log_t::flag_t::CRITICAL, error.what());
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
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Переменная опроса события
			int32_t poll = 0;
			// Количество событий для опроса
			size_t count = 0;
			/**
			 * Если это FreeBSD или MacOS X
			 */
			#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__)
				// Создаём объект временного таймаута
				struct timespec baseDelay = {0, 0};
				// Если установлен конкретный таймаут
				if((this->_baseDelay > 0) && !this->_easily){
					// Устанавливаем время в секундах
					baseDelay.tv_sec = (this->_baseDelay / 1000);
					// Устанавливаем время счётчика (наносекунды)
					baseDelay.tv_nsec = (((this->_baseDelay % 1000) * 1000) * 1000000);
				}
			#endif
			// Запускаем работу таймеров скрина
			this->_timeout.start();
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
							poll = WSAPoll(this->_fds.data(), this->_fds.size(), (!this->_easily ? this->_baseDelay : 0));
							// Если мы получили ошибку
							if(poll == SOCKET_ERROR)
								// Выводим сообщение об ошибке
								this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
							// Если сработал таймаут
							else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <std::recursive_mutex> lock(this->_mtx);
								// Получаем количество файловых дескрипторов для проверки
								count = this->_fds.size();
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET fd = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех файловых дескрипторов
								for(size_t i = 0; i < count; i++){
									// Если записей достаточно в списке
									if(i < this->_fds.size()){
										// Зануляем идентификатор события
										id = 0;
										// Получаем файловый дескриптор
										fd = this->_fds.at(i).fd;
										// Получаем флаг достуности чтения из сокета
										isRead = (this->_fds.at(i).revents & POLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (this->_fds.at(i).revents & POLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (this->_fds.at(i).revents & POLLERR);
										// Получаем флаг закрытия подключения
										isClose = (this->_fds.at(i).revents & POLLHUP);
										// Обнуляем количество событий
										this->_fds.at(i).revents = 0;
										// Если флаг на чтение данных из сокета установлен
										if(isRead){
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Получаем идентификатор события
												id = j->second.id;
												// Если событие является таймером
												if(j->second.delay > 0){
													// Количество прочитанных байт
													int32_t bytes = -1;
													// Устанавливаем временное значение буфера
													vector <char> buffer(sizeof(time_t), 0);
													// Если чтение выполнено удачно
													if((bytes = j->second.pipe->read(fd, buffer.data(), buffer.size())) > 0){
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																j->second.callback(fd, event_type_t::TIMER);
														}
														// Выполняем поиск указанной записи
														j = this->_items.find(fd);
														// Если сокет в списке найден
														if((j != this->_items.end()) && (id == j->second.id)){
															// Если таймер установлен как серийный
															if(j->second.series){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем активацию таймера на указанное время
																	this->_timeout.set(j->second.fd, j->second.delay * 1000000, j->second.pipe->port());
															}
														}
													// Выполняем закрытие подключения
													} else if(bytes == 0)
														// Удаляем файловый дескриптор из базы событий
														this->del(j->second.id, j->second.fd);
												// Если событие не является таймером
												} else {
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Выполняем поиск события на получение данных присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::READ);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															j->second.callback(fd, event_type_t::READ);
													}
												}
											// Если файловый дескриптор не нулевой
											} else if(fd > 0)
												// Выводим сообщение об ошибке
												this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, fd);
										}
										// Если сокет доступен для записи
										if(isWrite){
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if((j != this->_items.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														j->second.callback(fd, event_type_t::WRITE);
												}
											}
										}
										// Если сокет отключился или произошла ошибка
										if(isClose || isError){
											// Если мы реально получили ошибку
											if(WSAGetLastError() > 0)
												// Выводим сообщение об ошибке
												this->_log->print("Event base dispatch: %s, SOCKET=%d", log_t::flag_t::CRITICAL, this->_socket.message().c_str(), fd);
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if((j != this->_items.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Получаем функцию обратного вызова
													auto callback = std::bind(j->second.callback, fd, event_type_t::CLOSE);
													// Выполняем поиск события на отключение присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::CLOSE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
														// Удаляем файловый дескриптор из базы событий
														this->del(j->second.id, fd);
														// Выполняем функцию обратного вызова
														callback();
														// Продолжаем обход дальше
														continue;
													}
												}
												// Удаляем файловый дескриптор из базы событий
												this->del(j->second.id, fd);
											}
										}
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_baseDelay > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
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
							poll = ::epoll_wait(this->_efd, this->_events.data(), this->_maxCount, (!this->_easily ? this->_baseDelay : 0));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET)
								// Выводим сообщение об ошибке
								this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
							// Если сработал таймаут
							else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <std::recursive_mutex> lock(this->_mtx);
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET fd = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех событий в которых мы получили изменения
								for(int32_t i = 0; i < poll; i++){
									// Если записей достаточно в списке
									if(static_cast <size_t> (i) < this->_events.size()){
										// Получаем флаг достуности чтения из сокета
										isRead = (this->_events.at(i).events & EPOLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (this->_events.at(i).events & EPOLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (this->_events.at(i).events & EPOLLERR);
										// Получаем флаг закрытия подключения
										isClose = (this->_events.at(i).events & (EPOLLRDHUP | EPOLLHUP));
										// Получаем объект текущего события
										item_t * item = reinterpret_cast <item_t *> (this->_events.at(i).data.ptr);
										// Если объект текущего события получен
										if(item != nullptr){
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											fd = item->fd;
											// Если в сокете появились данные для чтения
											if(isRead){
												// Если событие является таймером
												if(item->delay > 0){
													// Количество сработок таймера
													uint64_t value = 0;
													// Количество прочитанных байт
													int32_t bytes = -1;
													// Если чтение выполнено удачно
													if((bytes = ::read(item->fd, &value, sizeof(value))) > 0){
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto j = item->mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																item->callback(item->fd, event_type_t::TIMER);
														}
													}
												// Если событие не является таймером
												} else {
													// Если функция обратного вызова установлена
													if(item->callback != nullptr){
														// Выполняем поиск события на получение данных присутствует в базе событий
														auto j = item->mode.find(event_type_t::READ);
														// Если событие найдено и оно активированно
														if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															item->callback(item->fd, event_type_t::READ);
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if((i != this->_items.end()) && (id == i->second.id)){
													// Если функция обратного вызова установлена
													if(i->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto j = i->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															i->second.callback(i->second.fd, event_type_t::WRITE);
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError)
													// Выводим сообщение об ошибке
													this->_log->print("Event base dispatch: %s, SOCKET=%d", log_t::flag_t::CRITICAL, this->_socket.message().c_str(), fd);
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if((i != this->_items.end()) && (id == i->second.id)){
													// Если функция обратного вызова установлена
													if(i->second.callback != nullptr){
														// Получаем функцию обратного вызова
														auto callback = std::bind(i->second.callback, i->second.fd, event_type_t::CLOSE);
														// Выполняем поиск события на отключение присутствует в базе событий
														auto j = i->second.mode.find(event_type_t::CLOSE);
														// Если событие найдено и оно активированно
														if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED)){
															// Удаляем файловый дескриптор из базы событий
															this->del(i->second.id, i->second.fd);
															// Выполняем функцию обратного вызова
															callback();
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем файловый дескриптор из базы событий
													this->del(i->second.id, i->second.fd);
												}
											}
										// Если файловый дескриптор не нулевой
										} else if(this->_events.at(i).data.fd > 0)
											// Выводим сообщение об ошибке
											this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, this->_events.at(i).data.fd);
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_baseDelay > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
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
							poll = kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_baseDelay > -1) || this->_easily ? &baseDelay : nullptr));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET)
								// Выводим сообщение об ошибке
								this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
							// Если сработал таймаут
							else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <std::recursive_mutex> lock(this->_mtx);
								// Идентификатор события
								uint64_t id = 0;
								// Код ошибки полученный от ядра
								int32_t code = 0;
								// Файловый дескриптор события
								SOCKET fd = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false, isClose = false, isError = false;
								// Выполняем перебор всех событий в которых мы получили изменения
								for(int32_t i = 0; i < poll; i++){
									// Если записей достаточно в списке
									if(static_cast <size_t> (i) < this->_events.size()){
										// Получаем код ошибки переданный ядром
										code = this->_events.at(i).data;
										// Получаем флаг закрытия подключения
										isClose = (this->_events.at(i).flags & EV_EOF);
										// Получаем флаг получения ошибки сокета
										isError = (this->_events.at(i).flags & EV_ERROR);
										// Получаем флаг достуности чтения из сокета
										isRead = (this->_events.at(i).filter & EVFILT_READ);
										// Получаем флаг доступности сокета на запись
										isWrite = (this->_events.at(i).filter & EVFILT_WRITE);
										// Выполняем поиск файлового дескриптора в базе событий
										auto j = this->_items.find(this->_events.at(i).ident);
										// Если файловый дескриптор есть в базе событий
										if(j != this->_items.end()){
											// Получаем объект текущего события
											item_t * item = &j->second;
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											fd = item->fd;
											// Если в сокете появились данные для чтения
											if(isRead){
												// Если событие является таймером
												if(item->delay > 0){
													// Количество прочитанных байт
													int32_t bytes = -1;
													// Устанавливаем временное значение буфера
													vector <char> buffer(sizeof(time_t), 0);
													// Если чтение выполнено удачно
													if((bytes = item->pipe->read(item->fd, buffer.data(), buffer.size())) > 0){
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto k = item->mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																item->callback(item->fd, event_type_t::TIMER);
														}
														// Выполняем поиск файлового дескриптора в базе событий
														j = this->_items.find(fd);
														// Если файловый дескриптор есть в базе событий
														if((j != this->_items.end()) && (id == j->second.id)){
															// Если таймер установлен как серийный
															if(j->second.series){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем активацию таймера на указанное время
																	this->_timeout.set(j->second.timer, j->second.delay * 1000000);
															}
														}
													// Выполняем закрытие подключения
													} else if(bytes == 0)
														// Удаляем файловый дескриптор из базы событий
														this->del(item->id, item->fd);
												// Если событие не является таймером
												} else {
													// Если функция обратного вызова установлена
													if(item->callback != nullptr){
														// Выполняем поиск события на получение данных присутствует в базе событий
														auto k = item->mode.find(event_type_t::READ);
														// Если событие найдено и оно активированно
														if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															item->callback(fd, event_type_t::READ);
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if((j != this->_items.end()) && (id == j->second.id)){
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															j->second.callback(j->second.fd, event_type_t::WRITE);
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError)
													// Выводим сообщение об ошибке
													this->_log->print("Event base dispatch: %s, SOCKET=%d", log_t::flag_t::CRITICAL, this->_socket.message(code).c_str(), fd);
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if((j != this->_items.end()) && (id == j->second.id)){
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Получаем функцию обратного вызова
														auto callback = std::bind(j->second.callback, j->second.fd, event_type_t::CLOSE);
														// Выполняем поиск события на отключение присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::CLOSE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
															// Удаляем файловый дескриптор из базы событий
															this->del(j->second.id, j->second.fd);
															// Выполняем функцию обратного вызова
															callback();
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем файловый дескриптор из базы событий
													this->del(j->second.id, j->second.fd);
												}
											}
										// Если файловый дескриптор не нулевой
										} else if(this->_events.at(i).ident > 0)
											// Выводим сообщение об ошибке
											this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, this->_events.at(i).ident);
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_baseDelay > 0)
									// Выполняем задержку времени на указанное количество времени
									std::this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
								// Устанавливаем задержку времени по умолчанию
								else std::this_thread::sleep_for(10ms);
							}
						// Замораживаем поток на период времени частоты обновления базы событий
						} else std::this_thread::sleep_for(100ms);
					}
				#endif
			}
			// Останавливаем работу таймеров скрина
			this->_timeout.stop();
			// Снимаем флаг запущенного опроса базы событий
			this->_launched = this->_mode;
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			// Если не происходит отключение работы базы событий
			if(this->_mode)
				// Выводим сообщение об ошибке
				this->_log->print("Event base dispatch: %s", log_t::flag_t::CRITICAL, error.what());
			// Снимаем флаг запущенного опроса базы событий
			else this->_launched = this->_mode;
		}
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
		// Запоминаем список активных событий
		std::map <SOCKET, item_t> items = this->_items;
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
		// Если список активных событий не пустой
		if(!items.empty()){
			// Выполняем перебор всего списка активных событий
			for(auto & item : items){
				// Выполняем добавление события в базу событий
				if(!this->add(item.second.id, item.second.fd, item.second.callback))
					// Выводим сообщение что событие не вышло активировать
					this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.fd);
			}
		}
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
		this->_baseDelay = -1;
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
		this->_baseDelay = static_cast <int32_t> (msec);
	// Выполняем сброс таймаута
	else this->_baseDelay = -1;
}
/**
 * Base Конструктор
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 * @param count максимальное количество обрабатываемых сокетов
 */
awh::Base::Base(const fmk_t * fmk, const log_t * log, const uint32_t count) noexcept :
 _mode(false), _easily(false), _locker(false), _launched(false), _baseDelay(-1),
 _maxCount(count), _socket(fmk, log), _timeout(fmk, log), _fmk(fmk), _log(log) {
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
/**
 * type Метод получения типа события
 * @return установленный тип события
 */
awh::Event::type_t awh::Event::type() const noexcept {
	// Выводим тип установленного события
	return this->_type;
}
/**
 * set Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Event::set(base_t * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем базу данных событий
	this->_base = base;
}
/**
 * set Метод установки файлового дескриптора
 * @param fd файловый дескриптор для установки
 */
void awh::Event::set(const SOCKET fd) noexcept {
	// Получаем флаг перезапуска работы события
	const bool restart = (this->_mode && (fd != INVALID_SOCKET) && (this->_fd != INVALID_SOCKET) && (fd != this->_fd));
	// Если необходимо выполнить перезапуск события
	if(restart)
		// Выполняем остановку работы события
		this->stop();
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Устанавливаем файловый дескриптор
			this->_fd = fd;
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Socket for a event timeout cannot be set", log_t::flag_t::WARNING);
		break;
	}
	// Выполняем разблокировку потока
	this->_mtx.unlock();
	// Если необходимо выполнить перезапуск события
	if(restart)
		// Выполняем запуск работы события
		this->start();
}
/**
 * set Метод установки функции обратного вызова
 * @param callback функция обратного вызова
 */
void awh::Event::set(base_t::callback_t callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Устанавливаем функцию обратного вызова
	this->_callback = callback;
}
/**
 * Метод удаления типа события
 * @param type тип события для удаления
 */
void awh::Event::del(const base_t::event_type_t type) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0))
			// Выполняем удаление события
			this->_base->del(this->_id, this->_fd, type);
		// Выводим сообщение об ошибке
		else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * timeout Метод установки задержки времени таймера
 * @param delay  задержка времени в миллисекундах
 * @param series флаг серийного таймаута
 */
void awh::Event::timeout(const time_t delay, const bool series) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER): {
			// Устанавливаем задержки времени в миллисекундах
			this->_delay = delay;
			// Выполняем установку флага серийности таймера
			this->_series = series;
		} break;
	}
}
/**
 * mode Метод установки режима работы модуля
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Event::mode(const base_t::event_type_t type, const base_t::event_mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если работа базы событий активированна
	if((this->_base != nullptr) && ((this->_fd != INVALID_SOCKET) || (this->_delay > 0)))
		// Выполняем установку режима работы модуля
		return this->_base->mode(this->_id, this->_fd, type, mode);
	// Сообщаем, что режим работы не установлен
	return false;
}
/**
 * stop Метод остановки работы события
 */
void awh::Event::stop() noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Снимаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем удаление всех событий
			this->_base->del(this->_id, this->_fd, base_t::event_type_t::READ);
			this->_base->del(this->_id, this->_fd, base_t::event_type_t::TIMER);
			this->_base->del(this->_id, this->_fd, base_t::event_type_t::WRITE);
			this->_base->del(this->_id, this->_fd, base_t::event_type_t::CLOSE);
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * start Метод запуска работы события
 */
void awh::Event::start() noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если работа события ещё не запущена
	if(!this->_mode && (this->_base != nullptr)){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Устанавливаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем генерацию идентификатора записи
			this->_id = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
			// Определяем тип установленного события
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип является обычным событием
				case static_cast <uint8_t> (type_t::EVENT): {
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_fd, this->_callback))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, this->_fd);
				} break;
				// Если тип события является таймером
				case static_cast <uint8_t> (type_t::TIMER): {
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_fd, this->_callback, this->_delay, this->_series))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate timer", log_t::flag_t::WARNING);
				} break;
			}
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * Оператор [=] для установки базы событий
 * @param base база событий для установки
 * @return     текущий объект
 */
awh::Event & awh::Event::operator = (base_t * base) noexcept {
	// Выполняем установку базы событий
	this->set(base);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки файлового дескриптора
 * @param fd файловый дескриптор для установки
 * @return   текущий объект
 */
awh::Event & awh::Event::operator = (const SOCKET fd) noexcept {
	// Выполняем установку файлового дескриптора
	this->set(fd);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки задержки времени таймера
 * @param delay задержка времени в миллисекундах
 * @return      текущий объект
 */
awh::Event & awh::Event::operator = (const time_t delay) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER):
			// Устанавливаем задержки времени в миллисекундах
			this->_delay = delay;
		break;
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки функции обратного вызова
 * @param callback функция обратного вызова
 * @return         текущий объект
 */
awh::Event & awh::Event::operator = (base_t::callback_t callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->set(callback);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * ~Event Деструктор
 */
awh::Event::~Event() noexcept {
	// Выполняем остановку работы события
	this->stop();
}
