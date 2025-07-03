/**
 * @file: evbase.cpp
 * @date: 2024-06-26
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
#include <events/evbase.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Для операционной системы OS Windows
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
 * id Метод получения идентификатора потока
 * @return идентификатор потока для получения
 */
uint64_t awh::Base::id() const noexcept {
	// Создаём объект хэширования
	hash <thread::id> hasher;
	// Устанавливаем идентификатор потока
	return hasher(this_thread::get_id());
}
/**
 * stream Метод проверки запущен ли модуль в дочернем потоке
 * @return результат проверки
 */
bool awh::Base::stream() const noexcept {
	// Выполняем проверку
	return (this->_id != this->id());
}
/**
 * init Метод инициализации базы событий
 * @param mode флаг инициализации
 */
void awh::Base::init(const event_mode_t mode) noexcept {
	// Определяем флаг инициализации
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать сетевые методы
		case static_cast <uint8_t> (event_mode_t::ENABLED): {
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если WinSocksAPI ещё не инициализирована
				if(!(this->_winSockInit = winsockInitialized())){
					// Идентификатор ошибки
					int32_t error = 0;
					// Выполняем инициализацию сетевого контекста
					if((error = WSAStartup(MAKEWORD(2, 2), &this->_wsaData)) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message(error).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(error).c_str());
						#endif
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
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем инициализацию EPoll
				if((this->_efd = ::epoll_create(this->_maxCount)) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message().c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем открытие файлового дескриптора
				::fcntl(this->_efd, F_SETFD, FD_CLOEXEC);
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем инициализацию Kqueue
				if((this->_kq = kqueue()) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message().c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
				// Выполняем открытие файлового дескриптора
				::fcntl(this->_kq, F_SETFD, FD_CLOEXEC);
			#endif
		} break;
		// Если необходимо деактивировать сетевые методы
		case static_cast <uint8_t> (event_mode_t::DISABLED): {
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если WinSocksAPI была инициализированна в этой базе событий
				if(!this->_winSockInit)
					// Очищаем сетевой контекст
					WSACleanup();
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем закрытие подключения
				::close(this->_efd);
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем закрытие подключения
				::close(this->_kq);
			#endif
		} break;
	}
}
/**
 * upstream Метод получения событий верхнеуровневых потоков
 * @param sid  идентификатор верхнеуровневого потока
 * @param fd   файловый дескриптор верхнеуровневого потока
 * @param type тип отслеживаемого события
 */
void awh::Base::upstream(const uint64_t sid, const SOCKET fd, const event_type_t type) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем поиск указанного верхнеуровневого потока
	auto i = this->_upstreams.find(sid);
	// Если верхнеуровневый поток обнаружен
	if(i != this->_upstreams.end()){
		// Определяем тип входящего события
		switch(static_cast <uint8_t> (type)){
			// Если событие является чтением
			case static_cast <uint8_t> (event_type_t::READ): {
				// Трансферный идентификатор
				uint64_t tid = 0;
				// Если чтение выполнено удачно
				if(i->second.pipe->read(i->second.read, reinterpret_cast <char *> (&tid), sizeof(tid)) == sizeof(tid)){
					// Если функция обратного вызова существует
					if(i->second.callback != nullptr)
						// Выполняем функцию обратного вызова
						apply(i->second.callback, make_tuple(tid));
				}
			} break;
			// Если событие является закрытием подключения
			case static_cast <uint8_t> (event_type_t::CLOSE): {
				// Выполняем удаление верхнеуровневого потока
				this->eraseUpstream(sid);
				// Выводим сообщение об удалении верхнеуровневого потока
				this->_log->print("Removing upstream ID=%llu in event base", log_t::flag_t::WARNING, sid);
			} break;
		}
	}
}
/**
 * del Метод удаления файлового дескриптора из базы событий
 * @param fd файловый дескриптор для удаления
 * @return   результат работы функции
 */
bool awh::Base::del(const SOCKET fd) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		/**
		 * Для операционной системы OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_fds.begin(); i != this->_fds.end(); ++i){
				// Если файловый дескриптор найден
				if(i->fd == fd){
					// Очищаем полученное событие
					i->revents = 0;
					// Выполняем поиск файлового дескриптора в базе событий
					auto j = this->_items.find(i->fd);
					// Если файловый дескриптор есть в базе событий
					if(j != this->_items.end()){
						// Если событие является таймером
						if(j->second.delay > 0){
							// Выполняем удаление таймера
							this->_evtimer.del(i->timer);
							// Выполняем закрытие подключения
							::closesocket(i->fd);
							// Выполняем закрытие таймера
							::closesocket(i->timer);
						// Выполняем закрытие подключения
						} else ::closesocket(i->fd);
					// Выполняем закрытие подключения
					} else ::closesocket(i->fd);
					// Выполняем сброс файлового дескриптора
					i->fd = INVALID_SOCKET;
					// Выполняем удаление события из списка отслеживания
					this->_fds.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Флаг удалённого события из базы событий
			bool erased = false;
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если файловый дескриптор найден
				if((i->data.ptr != nullptr) && (reinterpret_cast <item_t *> (i->data.ptr)->fd == fd)){
					// Выполняем изменение параметров события
					result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, fd, &(* i)) == 0);
					// Выполняем закрытие подключения
					::close(fd);
					// Если событие является таймером
					if(reinterpret_cast <item_t *> (i->data.ptr)->delay > 0){
						// Выполняем удаление таймера
						this->_evtimer.del(reinterpret_cast <item_t *> (i->data.ptr)->timer);
						// Выполняем закрытие таймера
						::close(reinterpret_cast <item_t *> (i->data.ptr)->timer);
						// Выполняем поиск таймера в списке таймеров
						auto j = this->_timers.find(reinterpret_cast <item_t *> (i->data.ptr)->timer);
						// Если таймер в списке таймеров найден, удаляем его
						if(j != this->_timers.end())
							// Выполняем удаление таймера
							this->_timers.erase(j);
					}
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end(); ++i){
				// Если файловый дескриптор найден
				if((i->data.ptr != nullptr) && (reinterpret_cast <item_t *> (i->data.ptr)->fd == fd)){
					// Если событие ещё не удалено из базы событий
					if(!erased){
						// Выполняем изменение параметров события
						result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, fd, &(* i)) == 0);
						// Выполняем закрытие подключения
						::close(fd);
						// Если событие является таймером
						if(reinterpret_cast <item_t *> (i->data.ptr)->delay > 0){
							// Выполняем удаление таймера
							this->_evtimer.del(reinterpret_cast <item_t *> (i->data.ptr)->timer);
							// Выполняем закрытие таймера
							::close(reinterpret_cast <item_t *> (i->data.ptr)->timer);
							// Выполняем поиск таймера в списке таймеров
							auto j = this->_timers.find(reinterpret_cast <item_t *> (i->data.ptr)->timer);
							// Если таймер в списке таймеров найден, удаляем его
							if(j != this->_timers.end())
								// Выполняем удаление таймера
								this->_timers.erase(j);
						}
					}
					// Выполняем удаление события из списка изменений
					this->_change.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Если удаление не выполненно
			if(!result){
				// Выполняем изменение параметров события
				result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, fd, nullptr) == 0);
				// Выполняем закрытие подключения
				::close(fd);
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Флаг удалённого события из базы событий
			bool erased = false;
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if(i != this->_items.end()){
				// Если событие является таймером
				if(i->second.delay > 0){
					// Выполняем удаление таймера
					this->_evtimer.del(i->second.timer);
					// Выполняем закрытие таймера
					::close(i->second.timer);
					// Выполняем поиск таймера в списке таймеров
					auto j = this->_timers.find(i->second.timer);
					// Если таймер в списке таймеров найден, удаляем его
					if(j != this->_timers.end())
						// Выполняем удаление таймера
						this->_timers.erase(j);
				}
			}
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если файловый дескриптор найден
				if((erased = (i->ident == fd))){
					// Выполняем удаление объекта события
					EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
					// Выполняем закрытие подключения
					::close(i->ident);
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end(); ++i){
				// Если файловый дескриптор найден
				if(i->ident == fd){
					// Если событие ещё не удалено из базы событий
					if(!erased){
						// Выполняем удаление объекта события
						EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(i->ident);
					}
					// Выполняем удаление события из списка изменений
					this->_change.erase(i);
					// Выходим из цикла
					break;
				}
			}
			// Выполняем разблокировку чтения базы событий
			this->_locker = false;
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
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
		const lock_guard <recursive_mutex> lock(this->_mtx);
		/**
		 * Для операционной системы OS Windows
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
							// Выполняем удаление таймера
							this->_evtimer.del(j->timer);
							// Выполняем закрытие подключения
							::closesocket(j->fd);
							// Выполняем закрытие таймера
							::closesocket(j->timer);
						// Выполняем закрытие подключения
						} else ::closesocket(j->fd);
						// Выполняем сброс файлового дескриптора
						j->fd = INVALID_SOCKET;
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
		 * Для операционной системы Linux
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
				// Выполняем удаление таймера
				this->_evtimer.del(i->second.timer);
				// Если событие является таймером
				if(i->second.delay > 0){
					// Выполняем закрытие таймера
					::close(i->second.timer);
					// Выполняем поиск таймера в списке таймеров
					auto j = this->_timers.find(i->second.timer);
					// Если таймер в списке таймеров найден, удаляем его
					if(j != this->_timers.end())
						// Выполняем удаление таймера
						this->_timers.erase(j);
				}
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
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_items.find(fd);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_items.end()) && (i->second.id == id))){
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем удаление таймера
				this->_evtimer.del(i->second.timer);
				// Если событие является таймером
				if(i->second.delay > 0){
					// Выполняем закрытие таймера
					::close(i->second.timer);
					// Выполняем поиск таймера в списке таймеров
					auto j = this->_timers.find(i->second.timer);
					// Если таймер в списке таймеров найден, удаляем его
					if(j != this->_timers.end())
						// Выполняем удаление таймера
						this->_timers.erase(j);
				}
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
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
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
			const lock_guard <recursive_mutex> lock(this->_mtx);
			/**
			 * Для операционной системы OS Windows
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
										// Выполняем удаление таймера
										this->_evtimer.del(k->timer);
										// Выполняем закрытие подключения
										::closesocket(k->fd);
										// Выполняем закрытие таймера
										::closesocket(k->timer);
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
			 * Для операционной системы Linux
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
								if(i->second.delay > 0){
									// Выполняем удаление таймера
									this->_evtimer.del(i->second.timer);
									// Выполняем закрытие подключения
									::close(i->second.fd);
									// Выполняем закрытие таймера
									::close(i->second.timer);
									// Выполняем поиск таймера в списке таймеров
									auto j = this->_timers.find(i->second.timer);
									// Если таймер в списке таймеров найден, удаляем его
									if(j != this->_timers.end())
										// Выполняем удаление таймера
										this->_timers.erase(j);
								}
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
								if(i->second.delay > 0){
									// Выполняем удаление таймера
									this->_evtimer.del(i->second.timer);
									// Выполняем закрытие подключения
									::close(i->second.fd);
									// Выполняем закрытие таймера
									::close(i->second.timer);
									// Выполняем поиск таймера в списке таймеров
									auto j = this->_timers.find(i->second.timer);
									// Если таймер в списке таймеров найден, удаляем его
									if(j != this->_timers.end())
										// Выполняем удаление таймера
										this->_timers.erase(j);
								}
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
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
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
										// Выполняем удаление таймера
										this->_evtimer.del(i->second.timer);
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем закрытие подключения
										::close(i->second.fd);
										// Выполняем закрытие таймера
										::close(i->second.timer);
										// Выполняем поиск таймера в списке таймеров
										auto l = this->_timers.find(i->second.timer);
										// Если таймер в списке таймеров найден, удаляем его
										if(l != this->_timers.end())
											// Выполняем удаление таймера
											this->_timers.erase(l);
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
									// Выполняем удаление таймера
									this->_evtimer.del(i->second.timer);
									// Выполняем удаление события таймера
									EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
									// Выполняем закрытие подключения
									::close(i->second.fd);
									// Выполняем закрытие таймера
									::close(i->second.timer);
									// Выполняем поиск таймера в списке таймеров
									auto j = this->_timers.find(i->second.timer);
									// Если таймер в списке таймеров найден, удаляем его
									if(j != this->_timers.end())
										// Выполняем удаление таймера
										this->_timers.erase(j);
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
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
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
bool awh::Base::add(const uint64_t id, SOCKET & fd, callback_t callback, const uint32_t delay, const bool series) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((fd != INVALID_SOCKET) || (delay > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(this->_mtx);
			// Если количество добавленных файловых дескрипторов для отслеживания не достигло предела
			if(this->_items.size() < static_cast <size_t> (this->_maxCount)){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				/**
				 * Для операционной системы OS Windows
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
							auto pipe = make_shared <evpipe_t> (this->_fmk, this->_log);
							// Устанавливаем тип пайпа
							pipe->type(evpipe_t::type_t::NATIVE);
							// Выполняем создание сокетов
							auto fds = pipe->create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							fd = fds[0];
							// Делаем сокет неблокирующим
							this->_socket.blocking(fd, socket_t::mode_t::DISABLED);
							// Выполняем добавление таймера в список таймеров
							this->_timers.emplace(fds[1]);
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
							// Устанавливаем файловый дескриптор в список для отслеживания
							this->_fds.push_back((WSAPOLLFD){});
							// Выполняем установку файлового дескриптора
							this->_fds.back().fd = fd;
						}
					}
				/**
				 * Для операционной системы Linux
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
						// Объект текущего события
						item_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Создаём объект пайпа
							auto pipe = make_shared <evpipe_t> (this->_fmk, this->_log);
							// Устанавливаем тип пайпа
							pipe->type(evpipe_t::type_t::NATIVE);
							// Выполняем создание сокетов
							auto fds = pipe->create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							fd = fds[0];
							// Делаем сокет неблокирующим
							this->_socket.blocking(fd, socket_t::mode_t::DISABLED);
							// Выполняем добавление таймера в список таймеров
							this->_timers.emplace(fds[1]);
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
							this->_change.push_back((struct epoll_event){});
							// Устанавливаем новый объект для отслеживания события
							this->_events.push_back((struct epoll_event){});
							// Выполняем установку указателя на основное событие
							this->_change.back().data.ptr = item;
							// Устанавливаем флаг ожидания отключения сокета
							this->_change.back().events = EPOLLERR;
							// Выполняем изменение параметров события
							if(!(result = (::epoll_ctl(this->_efd, EPOLL_CTL_ADD, fd, &this->_change.back()) == 0))){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, delay, series), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								#endif
							}
						}
					}
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
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
							auto pipe = make_shared <evpipe_t> (this->_fmk, this->_log);
							// Устанавливаем тип пайпа
							pipe->type(evpipe_t::type_t::NATIVE);
							// Выполняем создание сокетов
							auto fds = pipe->create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							fd = fds[0];
							// Делаем сокет неблокирующим
							this->_socket.blocking(fd, socket_t::mode_t::DISABLED);
							// Выполняем добавление таймера в список таймеров
							this->_timers.emplace(fds[1]);
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
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, delay, series), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
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
			const lock_guard <recursive_mutex> lock(this->_mtx);
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
					 * Для операционной системы OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Если тип установлен как не закрытие подключения
						if(type != event_type_t::CLOSE){
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
													this->_evtimer.set(i->second.timer, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Снимаем флаг ожидания готовности файлового дескриптора на чтение
													k->events ^= POLLIN;
													// Выполняем деактивацию таймера
													this->_evtimer.del(i->second.timer);
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
						}
					/**
					 * Для операционной системы Linux
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												// Выполняем активацию таймера на указанное время
												} else this->_evtimer.set(i->second.timer, i->second.delay);
											} break;
											// Если нужно деактивировать событие таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												// Выполняем деактивацию таймера
												} else this->_evtimer.del(i->second.timer);
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем удаление флагов отслеживания закрытия подключения
												k->events ^= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
											} break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, fd, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
											} break;
										}
									} break;
								}
								// Выходим из цикла
								break;
							}
						}
					/**
					 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
					 */
					#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
						// Если тип установлен как не закрытие подключения
						if(type != event_type_t::CLOSE){
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
													this->_evtimer.set(i->second.timer, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
													// Выполняем деактивацию таймера
													this->_evtimer.del(i->second.timer);
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
						}
					#endif
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, fd, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
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
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем блокировку чтения базы событий
		this->_locker = true;
		/**
		 * Для операционной системы OS Windows
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
						// Выполняем удаление таймера
						this->_evtimer.del(i->timer);
						// Выполняем закрытие подключения
						::closesocket(i->fd);
						// Выполняем закрытие таймера
						::closesocket(i->timer);
					// Выполняем закрытие подключения
					} else ::closesocket(i->fd);
				// Выполняем закрытие подключения
				} else ::closesocket(i->fd);
				// Выполняем сброс файлового дескриптора
				i->fd = INVALID_SOCKET;
				// Выполняем удаление события из списка отслеживания
				i = this->_fds.erase(i);
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем изменение параметров события
				::epoll_ctl(this->_efd, EPOLL_CTL_DEL, reinterpret_cast <item_t *> (i->data.ptr)->fd, &(* i));
				// Выполняем закрытие подключения
				::close(reinterpret_cast <item_t *> (i->data.ptr)->fd);
				// Если событие является таймером
				if(reinterpret_cast <item_t *> (i->data.ptr)->delay > 0){
					// Выполняем удаление таймера
					this->_evtimer.del(reinterpret_cast <item_t *> (i->data.ptr)->timer);
					// Выполняем закрытие таймера
					::close(reinterpret_cast <item_t *> (i->data.ptr)->timer);
					// Выполняем поиск таймера в списке таймеров
					auto j = this->_timers.find(reinterpret_cast <item_t *> (i->data.ptr)->timer);
					// Если таймер в списке таймеров найден, удаляем его
					if(j != this->_timers.end())
						// Выполняем удаление таймера
						this->_timers.erase(j);
				}
				// Выполняем удаление события из списка изменений
				i = this->_change.erase(i);
			}
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();)
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_items.find(i->ident);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_items.end()){
					// Если событие является таймером
					if(j->second.delay > 0){
						// Выполняем удаление таймера
						this->_evtimer.del(j->second.timer);
						// Выполняем удаление события таймера
						EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(j->second.fd);
						// Выполняем закрытие таймера
						::close(j->second.timer);
						// Выполняем поиск таймера в списке таймеров
						auto k = this->_timers.find(j->second.timer);
						// Если таймер в списке таймеров найден, удаляем его
						if(k != this->_timers.end())
							// Выполняем удаление таймера
							this->_timers.erase(k);
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
						// Выполняем удаление таймера
						this->_evtimer.del(j->second.timer);
						// Выполняем удаление события таймера
						EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
						// Выполняем закрытие подключения
						::close(j->second.fd);
						// Выполняем закрытие таймера
						::close(j->second.timer);
						// Выполняем поиск таймера в списке таймеров
						auto k = this->_timers.find(j->second.timer);
						// Если таймер в списке таймеров найден, удаляем его
						if(k != this->_timers.end())
							// Выполняем удаление таймера
							this->_timers.erase(k);
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
		// Выполняем разблокировку чтения базы событий
		this->_locker = false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Выполняем разблокировку чтения базы событий
		this->_locker = false;
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * kick Метод отправки пинка
 */
void awh::Base::kick() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Если работа базы событий запущена
		if(this->_started){
			// Выполняем активацию блокировки
			this->_locker = this->_started;
			// Запоминаем список активных событий
			map <SOCKET, item_t> items = this->_items;
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Выполняем очистку всех параметров
			this->clear();
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Если список активных событий не пустой
			if(!items.empty()){
				// Выполняем перебор всего списка активных событий
				for(auto & item : items){
					// Выполняем добавление события в базу событий
					if(!this->add(item.second.id, item.second.fd, item.second.callback, item.second.delay, item.second.series))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.fd);
					// Если событие добавленно удачно
					else {
						// Выполняем перебор всех разрешений на запуск событий
						for(auto & mode : item.second.mode)
							// Выполняем активацию события на запуск
							this->mode(item.second.id, item.second.fd, mode.first, mode.second);
					}
				}
			}
		// Выполняем разблокировку потока
		} else this->_mtx.unlock();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * stop Метод остановки чтения базы событий
 */
void awh::Base::stop() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Если работа базы событий запущена
		if(this->_started){
			// Снимаем флаг работы базы событий
			this->_started = !this->_started;
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Выполняем очистку списка событий
			this->clear();
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
			// Выполняем разблокировку потока
			this->_mtx.unlock();
		// Выполняем разблокировку потока
		} else this->_mtx.unlock();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * start Метод запуска чтения базы событий
 */
void awh::Base::start() noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если работа базы событий не запущена
	if(!this->_started){
		// Устанавливаем флаг работы базы событий
		this->_started = !this->_started;
		// Переменная опроса события
		int32_t poll = 0;
		// Количество событий для опроса
		size_t count = 0;
		// Получаем идентификатор потока
		this->_id = this->id();
		/**
		 * Если это FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
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
		this->_evtimer.start();
		// Устанавливаем флаг запущенного опроса базы событий
		this->_launched = this->_started;
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем запуск базы события
			while(this->_started){
				/**
				 * Для операционной системы OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_fds.empty()){
							// Выполняем опрос базы событий
							poll = WSAPoll(this->_fds.data(), this->_fds.size(), (!this->_easily ? this->_baseDelay : 0));
							// Если мы получили ошибку
							if(poll == SOCKET_ERROR){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <recursive_mutex> lock(this->_mtx);
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
										// Получаем объект файлового дескриптора
										auto & event = this->_fds.at(i);
										// Получаем файловый дескриптор
										fd = event.fd;
										// Получаем флаг достуности чтения из сокета
										isRead = (event.revents & POLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.revents & POLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (event.revents & POLLERR);
										// Получаем флаг закрытия подключения
										isClose = (event.revents & POLLHUP);
										// Обнуляем количество событий
										event.revents = 0;
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
													// Фремя погрешности работы таймера
													uint64_t infelicity = 0;
													// Если чтение выполнено удачно
													if((bytes = j->second.pipe->read(fd, &infelicity, sizeof(infelicity))) > 0){
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																apply(j->second.callback, make_tuple(fd, event_type_t::TIMER));
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
																	this->_evtimer.set(j->second.fd, j->second.delay, j->second.pipe->port());
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
															apply(j->second.callback, make_tuple(fd, event_type_t::READ));
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
														apply(j->second.callback, make_tuple(fd, event_type_t::WRITE));
												}
											}
										}
										// Если сокет отключился или произошла ошибка
										if(isClose || isError){
											// Если мы реально получили ошибку
											if(WSAGetLastError() > 0){
												/**
												 * Если включён режим отладки
												 */
												#if defined(DEBUG_MODE)
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
												#endif
											}
											// Выполняем поиск указанной записи
											auto j = this->_items.find(fd);
											// Если сокет в списке найден
											if(j != this->_items.end()){
												// Если идентификаторы соответствуют
												if((id == j->second.id) || (id == 0)){
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
															apply(callback, make_tuple());
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем файловый дескриптор из базы событий
													this->del(j->second.id, fd);
												}
											// Выполняем удаление фантомного файлового дескриптора
											} else this->del(fd);
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
									this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
								// Устанавливаем задержку времени по умолчанию
								else this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							// Выполняем опрос базы событий
							poll = ::epoll_wait(this->_efd, this->_events.data(), this->_maxCount, (!this->_easily ? this->_baseDelay : 0));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <recursive_mutex> lock(this->_mtx);
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
										// Получаем объект файлового дескриптора
										const auto & event = this->_events.at(i);
										// Получаем флаг достуности чтения из сокета
										isRead = (event.events & EPOLLIN);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.events & EPOLLOUT);
										// Получаем флаг получения ошибки сокета
										isError = (event.events & EPOLLERR);
										// Получаем флаг закрытия подключения
										isClose = (event.events & (EPOLLRDHUP | EPOLLHUP));
										// Получаем объект текущего события
										item_t * item = reinterpret_cast <item_t *> (event.data.ptr);
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
													// Количество прочитанных байт
													int32_t bytes = -1;
													// Фремя погрешности работы таймера
													uint64_t infelicity = 0;
													// Если чтение выполнено удачно
													if((bytes = item->pipe->read(item->fd, &infelicity, sizeof(infelicity))) > 0){
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto j = item->mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																apply(item->callback, make_tuple(item->fd, event_type_t::TIMER));
														}
														// Выполняем поиск файлового дескриптора в базе событий
														auto j = this->_items.find(fd);
														// Если файловый дескриптор есть в базе событий
														if((j != this->_items.end()) && (id == j->second.id)){
															// Если таймер установлен как серийный
															if(j->second.series){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем активацию таймера на указанное время
																	this->_evtimer.set(j->second.timer, j->second.delay);
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
														auto j = item->mode.find(event_type_t::READ);
														// Если событие найдено и оно активированно
														if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															apply(item->callback, make_tuple(item->fd, event_type_t::READ));
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
															apply(i->second.callback, make_tuple(i->second.fd, event_type_t::WRITE));
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd), log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if(i != this->_items.end()){
													// Если идентификаторы соответствуют
													if(id == i->second.id){
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
																apply(callback, make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем файловый дескриптор из базы событий
														this->del(i->second.id, i->second.fd);
													}
												// Если файловый дескриптор не принадлежит таймерам
												} else if(this->_timers.find(fd) == this->_timers.end())
													// Выполняем удаление фантомного файлового дескриптора
													this->del(fd);
											}
										// Если файловый дескриптор не нулевой
										} else if(event.data.fd > 0)
											// Выводим сообщение об ошибке
											this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, event.data.fd);
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_baseDelay > 0)
									// Выполняем задержку времени на указанное количество времени
									this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
								// Устанавливаем задержку времени по умолчанию
								else this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							// Выполняем опрос базы событий
							poll = kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_baseDelay > -1) || this->_easily ? &baseDelay : nullptr));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message().c_str());
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Выполняем блокировку потока
								const lock_guard <recursive_mutex> lock(this->_mtx);
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
										// Получаем объект файлового дескриптора
										const auto & event = this->_events.at(i);
										// Получаем код ошибки переданный ядром
										code = event.data;
										// Получаем флаг закрытия подключения
										isClose = (event.flags & EV_EOF);
										// Получаем флаг получения ошибки сокета
										isError = (event.flags & EV_ERROR);
										// Получаем флаг достуности чтения из сокета
										isRead = (event.filter & EVFILT_READ);
										// Получаем флаг доступности сокета на запись
										isWrite = (event.filter & EVFILT_WRITE);
										// Выполняем поиск файлового дескриптора в базе событий
										auto j = this->_items.find(event.ident);
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
													// Фремя погрешности работы таймера
													uint64_t infelicity = 0;
													// Если чтение выполнено удачно
													if((bytes = item->pipe->read(item->fd, &infelicity, sizeof(infelicity))) > 0){
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события таймера присутствует в базе событий
															auto k = item->mode.find(event_type_t::TIMER);
															// Если событие найдено и оно активированно
															if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																apply(item->callback, make_tuple(item->fd, event_type_t::TIMER));
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
																	this->_evtimer.set(j->second.timer, j->second.delay);
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
															apply(item->callback, make_tuple(fd, event_type_t::READ));
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
															apply(j->second.callback, make_tuple(j->second.fd, event_type_t::WRITE));
													}
												}
											}
											// Если сокет отключился или произошла ошибка
											if(isClose || isError){
												// Если была вызвана ошибка
												if(isError){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fd), log_t::flag_t::CRITICAL, this->_socket.message(code).c_str());
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(code).c_str());
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_items.find(fd);
												// Если файловый дескриптор есть в базе событий
												if(j != this->_items.end()){
													// Если идентификаторы соответствуют
													if(id == j->second.id){
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
																apply(callback, make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем файловый дескриптор из базы событий
														this->del(j->second.id, j->second.fd);
													}
												// Если файловый дескриптор не принадлежит таймерам
												} else if(this->_timers.find(fd) == this->_timers.end())
													// Выполняем удаление фантомного файлового дескриптора
													this->del(fd);
											}
										// Если файловый дескриптор не нулевой
										} else if(event.ident > 0)
											// Выводим сообщение об ошибке
											this->_log->print("SOCKET=%d is not in the event list but is in the event database", log_t::flag_t::CRITICAL, event.ident);
									// Выходим из цикла
									} else break;
								}
							}
							// Если активирован простой режим работы чтения базы событий
							if(this->_easily){
								// Если время установленно
								if(this->_baseDelay > 0)
									// Выполняем задержку времени на указанное количество времени
									this_thread::sleep_for(chrono::milliseconds(this->_baseDelay));
								// Устанавливаем задержку времени по умолчанию
								else this_thread::sleep_for(10ms);
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					this_thread::sleep_for(100ms);
				#endif
			}
			// Останавливаем работу таймеров скрина
			this->_evtimer.stop();
			// Снимаем флаг запущенного опроса базы событий
			this->_launched = this->_started;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если не происходит отключение работы базы событий
			if(this->_started){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Снимаем флаг запущенного опроса базы событий
			} else this->_launched = this->_started;
		}
	// Выполняем разблокировку потока
	} else this->_mtx.unlock();
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Base::rebase() noexcept {
	// Если метод запущен в дочернем потоке
	if(this->stream())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"rebase\" cannot be called in a child thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Если работа базы событий запущена
			if(this->_started){
				// Выполняем разблокировку потока
				this->_mtx.unlock();
				// Запоминаем список активных событий
				map <SOCKET, item_t> items = this->_items;
				// Выполняем остановку работы базы событий
				this->stop();
				// Если список активных событий не пустой
				if(!items.empty()){
					// Выполняем перебор всего списка активных событий
					for(auto & item : items){
						// Выполняем добавление события в базу событий
						if(!this->add(item.second.id, item.second.fd, item.second.callback))
							// Выводим сообщение что событие не вышло активировать
							this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.fd);
						// Если событие добавленно удачно
						else {
							// Выполняем перебор всех разрешений на запуск событий
							for(auto & mode : item.second.mode)
								// Выполняем активацию события на запуск
								this->mode(item.second.id, item.second.fd, mode.first, mode.second);
						}
					}
				}
				// Выполняем запуск работы базы событий
				this->start();
			// Выполняем разблокировку потока
			} else this->_mtx.unlock();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * freeze Метод заморозки чтения данных
 * @param mode флаг активации
 */
void awh::Base::freeze(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем активацию блокировки
	this->_locker = mode;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации
 */
void awh::Base::easily(const bool mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если количество секунд передано верно
	if(msec > 0)
		// Выполняем установку времени ожидания
		this->_baseDelay = static_cast <int32_t> (msec);
	// Выполняем сброс таймаута
	else this->_baseDelay = -1;
}
/**
 * eraseUpstream Метод удаления верхнеуровневого потока
 * @param sid идентификатор верхнеуровневого потока
 */
void awh::Base::eraseUpstream(const uint64_t sid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем поиск указанного верхнеуровневого потока
	auto i = this->_upstreams.find(sid);
	// Если верхнеуровневый поток обнаружен
	if(i != this->_upstreams.end()){
		// Выполняем удаление события сокета из базы событий
		if(!this->del(i->first, i->second.read))
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Failed remove upstream event for SOCKET=%d", log_t::flag_t::WARNING, i->second.read);
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем закрытие открытого сокета на запись
			::close(i->second.write);
		#endif
		// Выполняем удаление верхнеуровневого потока из списка
		this->_upstreams.erase(i);
	}
}
/**
 * launchUpstream Метод запуска верхнеуровневого потока
 * @param sid идентификатор верхнеуровневого потока
 * @param tid идентификатор трансферной передачи
 */
void awh::Base::launchUpstream(const uint64_t sid, const uint64_t tid) noexcept {
	// Если метод запущен в основном потоке
	if(!this->stream())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"launchUpstream\" cannot be called in a main thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем поиск указанного верхнеуровневого потока
		auto i = this->_upstreams.find(sid);
		// Если верхнеуровневый поток обнаружен
		if(i != this->_upstreams.end()){
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем запись в сокет данных
				i->second.pipe->write(i->second.write, reinterpret_cast <const char *> (&tid), sizeof(tid), i->second.pipe->port());
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#else
				// Выполняем запись в сокет данных
				i->second.pipe->write(i->second.write, reinterpret_cast <const char *> (&tid), sizeof(tid));
			#endif
		}
	}
}
/**
 * emplaceUpstream Метод создания верхнеуровневого потока
 * @param callback функция обратного вызова
 * @return         идентификатор верхнеуровневого потока
 */
uint64_t awh::Base::emplaceUpstream(function <void (const uint64_t)> callback) noexcept {
	// Результат работы функции
	uint64_t result = 0;
	// Если метод запущен в дочернем потоке
	if(this->stream())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"emplaceUpstream\" cannot be called in a child thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_mtx);
		/**
		 * Для операционной системы OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Создаём объект пайпа
			auto pipe = make_shared <evpipe_t> (this->_fmk, this->_log);
			// Устанавливаем тип пайпа
			pipe->type(evpipe_t::type_t::NETWORK);
			// Выполняем создание сокетов
			auto fds = pipe->create();
			// Выполняем инициализацию таймера
			if(fds[0] == INVALID_SOCKET)
				// Выходим из функции
				return result;
			// Выполняем генерацию идентификатора верхнеуровневого потока
			result = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
			// Выполняем добавление в список верхнеуровневых потоков, новый поток
			auto ret = this->_upstreams.emplace(result, upstream_t());
			// Выполняем установку объекта пайпа
			ret.first->second.pipe = pipe;
			// Выполняем установку файлового дескриптора на чтение
			ret.first->second.read = fds[0];
			// Выполняем установку файлового дескриптора на запись
			ret.first->second.write = fds[0];
			// Выполняем установку функции обратного вызова
			ret.first->second.callback = callback;
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#else
			// Создаём объект пайпа
			auto pipe = make_shared <evpipe_t> (this->_fmk, this->_log);
			// Устанавливаем тип пайпа
			pipe->type(evpipe_t::type_t::NATIVE);
			// Выполняем создание сокетов
			auto fds = pipe->create();
			// Выполняем инициализацию таймера
			if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
				// Выходим из функции
				return result;
			// Делаем сокет неблокирующим
			this->_socket.blocking(fds[0], socket_t::mode_t::DISABLED);
			// Выполняем генерацию идентификатора верхнеуровневого потока
			result = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
			// Выполняем добавление в список верхнеуровневых потоков, новый поток
			auto ret = this->_upstreams.emplace(result, upstream_t());
			// Выполняем установку объекта пайпа
			ret.first->second.pipe = pipe;
			// Выполняем установку файлового дескриптора на чтение
			ret.first->second.read = fds[0];
			// Выполняем установку файлового дескриптора на запись
			ret.first->second.write = fds[1];
			// Выполняем установку функции обратного вызова
			ret.first->second.callback = callback;
		#endif
		// Выполняем добавление события в базу событий
		if(!this->add(result, ret.first->second.read, std::bind(&base_t::upstream, this, result, _1, _2)))
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Failed activate upstream event for SOCKET=%d", log_t::flag_t::WARNING, ret.first->second.read);
		// Если событие в базу событий успешно добавленно, активируем событие чтения на сокет верхнеуровневого потока
		else if(!this->mode(result, ret.first->second.read, event_type_t::READ, event_mode_t::ENABLED))
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Failed enabled read upstream event for SOCKET=%d", log_t::flag_t::WARNING, ret.first->second.read);
		// Если событие в базу событий успешно добавленно, активируем событие закрытие подключения верхнеуровневого потока
		else if(!this->mode(result, ret.first->second.read, event_type_t::CLOSE, event_mode_t::ENABLED))
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Failed enabled close upstream event for SOCKET=%d", log_t::flag_t::WARNING, ret.first->second.read);
	}
	// Выводим результат
	return result;
}
/**
 * Base Конструктор
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 * @param count максимальное количество обрабатываемых сокетов
 */
awh::Base::Base(const fmk_t * fmk, const log_t * log, const uint32_t count) noexcept :
 _id(0), _easily(false), _locker(false),
 _started(false), _launched(false), _baseDelay(-1),
 _maxCount(count), _socket(fmk, log),
 _evtimer(fmk, log), _fmk(fmk), _log(log) {
	// Получаем идентификатор потока
	this->_id = this->id();
	// Выполняем инициализацию базы событий
	this->init(event_mode_t::ENABLED);
}
/**
 * ~Base Деструктор
 */
awh::Base::~Base() noexcept {
	// Выполняем деинициализацию базы событий
	this->init(event_mode_t::DISABLED);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Устанавливаем функцию обратного вызова
	this->_callback = callback;
}
/**
 * Метод удаления типа события
 * @param type тип события для удаления
 */
void awh::Event::del(const base_t::event_type_t type) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
void awh::Event::timeout(const uint32_t delay, const bool series) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Снимаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем удаление всех событий
			this->_base->del(this->_id, this->_fd);
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * start Метод запуска работы события
 */
void awh::Event::start() noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
	// Если работа события ещё не запущена
	if(!this->_mode && (this->_base != nullptr)){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Устанавливаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем генерацию идентификатора записи
			this->_id = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
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
awh::Event & awh::Event::operator = (const uint32_t delay) noexcept {
	// Выполняем блокировку потока
	const lock_guard <recursive_mutex> lock(this->_mtx);
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
