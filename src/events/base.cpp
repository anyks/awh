/**
 * @file: base.cpp
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
#include <events/base.hpp>

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
#if _WIN32 || _WIN64
	/**
	 * winsockInitialized Метод проверки на инициализацию WinSocksAPI
	 * @return результат проверки
	 */
	static bool winsockInitialized() noexcept {
		// Выполняем создание сокета
		SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		// Если сокет не создан
		if(sock == INVALID_SOCKET)
			// Сообщаем, что сокет не создан а значит WinSocksAPI не инициализирован
			return false;
		// Выполняем закрытие открытого сокета
		::closesocket(sock);
		// Сообщаем, что WinSocksAPI уже инициализирован
		return true;
	}
#endif
/**
 * wid Метод получения идентификатора потока
 * @return идентификатор потока для получения
 */
uint64_t awh::Base::wid() const noexcept {
	// Создаём объект хэширования
	std::hash <std::thread::id> hasher;
	// Устанавливаем идентификатор потока
	return hasher(std::this_thread::get_id());
}
/**
 * isChildThread Метод проверки запущен ли модуль в дочернем потоке
 * @return результат проверки
 */
bool awh::Base::isChildThread() const noexcept {
	// Выполняем проверку
	return (this->_wid != this->wid());
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
			#if _WIN32 || _WIN64
				// Если WinSocksAPI ещё не инициализирована
				if(!(this->_winSockInit = ::winsockInitialized())){
					// Идентификатор ошибки
					int32_t error = 0;
					// Выполняем инициализацию сетевого контекста
					if((error = ::WSAStartup(MAKEWORD(2, 2), &this->_wsaData)) != 0){
						// Создаём буфер сообщения ошибки
						wchar_t message[256] = {0};
						// Сбрасываем значение сокета на чтение
						this->_socks[0] = INVALID_SOCKET;
						// Сбрасываем значение сокета на запись
						this->_socks[1] = INVALID_SOCKET;
						// Выполняем формирование текста ошибки
						::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
						#endif
						// Очищаем сетевой контекст
						::WSACleanup();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
					// Выполняем проверку версии WinSocket
					if((2 != LOBYTE(this->_wsaData.wVersion)) || (2 != HIBYTE(this->_wsaData.wVersion))){
						// Выводим сообщение об ошибке
						this->_log->print("Events base is not init", log_t::flag_t::CRITICAL);
						// Очищаем сетевой контекст
						::WSACleanup();
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				}
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем создание порта
				this->_pfd = ::port_create();
				// Если порт не создан
				if(this->_pfd == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
					// Выходим принудительно из приложения
					::exit(EXIT_FAILURE);
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем инициализацию EPoll
				if((this->_efd = ::epoll_create(static_cast <uint32_t> (this->_maxCount))) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
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
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
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
			#if _WIN32 || _WIN64
				// Если WinSocksAPI была инициализированна в этой базе событий
				if(!this->_winSockInit)
					// Очищаем сетевой контекст
					::WSACleanup();
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем закрытие подключения
				::close(this->_pfd);
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
 * @param sock файловый дескриптор верхнеуровневого потока
 * @param type тип отслеживаемого события
 */
void awh::Base::upstream(const uint64_t sid, const SOCKET sock, const event_type_t type) noexcept {
	/*
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
				if(i->second.pipe->read(i->second.read, tid) == sizeof(tid)){
					// Если функция обратного вызова существует
					if(i->second.callback != nullptr)
						// Выполняем функцию обратного вызова
						std::apply(i->second.callback, std::make_tuple(tid));
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
	*/
}
/**
 * del Метод удаления файлового дескриптора из базы событий
 * @param sock файловый дескриптор для удаления
 * @return     результат работы функции
 */
bool awh::Base::del(const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_fds.begin(); i != this->_fds.end(); ++i){
				// Если файловый дескриптор найден
				if(i->fd == sock){
					// Очищаем полученное событие
					i->revents = 0;
					// Выполняем поиск файлового дескриптора в базе событий
					auto j = this->_peers.find(i->fd);
					// Если файловый дескриптор есть в базе событий
					if(j != this->_peers.end()){
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (j->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление таймера
								this->_watch.away(j->second.socks[0]);
								// Выполняем поиск соучастника в списке соучастникам
								auto k = this->_partners.find(j->second.socks[1]);
								// Если соучастник в списке соучастников найден, удаляем его
								if(k != this->_partners.end())
									// Выполняем удаление соучастника
									this->_partners.erase(k);
							} break;
						}
					}
					// Выполняем закрытие подключения
					::closesocket(i->fd);
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
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем блокировку чтения базы событий
			this->_locker = true;
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если файловый дескриптор найден
				if(static_cast <SOCKET> (i->portev_object) == sock){
					// Удаляем сокет из порта и закрываем
					::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
					// Выполняем поиск файлового дескриптора в базе событий
					auto j = this->_peers.find(sock);
					// Если файловый дескриптор есть в базе событий
					if(j != this->_peers.end()){
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (j->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление таймера
								this->_watch.away(j->second.socks[0]);
								// Выполняем поиск соучастника в списке соучастникам
								auto k = this->_partners.find(j->second.socks[1]);
								// Если соучастник в списке соучастников найден, удаляем его
								if(k != this->_partners.end())
									// Выполняем удаление соучастника
									this->_partners.erase(k);
							} break;
						}
					}
					// Выполняем закрытие подключения
					::close(sock);
					// Выполняем удаление события из списка отслеживания
					this->_events.erase(i);
					// Выполняем деинициализацию базы событий
					this->init(event_mode_t::DISABLED);
					// Выполняем инициализацию базы событий
					this->init(event_mode_t::ENABLED);
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
				if((i->data.ptr != nullptr) && (reinterpret_cast <peer_t *> (i->data.ptr)->socks[0] == sock)){
					// Выполняем изменение параметров события
					result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, &(* i)) == 0);
					// Выполняем закрытие подключения
					::close(sock);
					// Определяем тип события к которому принадлежит сокет
					switch(static_cast <uint8_t> (reinterpret_cast <peer_t *> (i->data.ptr)->type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем удаление таймера
							this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->socks[0]);
							// Выполняем поиск соучастника в списке соучастникам
							auto j = this->_partners.find(reinterpret_cast <peer_t *> (i->data.ptr)->socks[1]);
							// Если соучастник в списке соучастников найден, удаляем его
							if(j != this->_partners.end())
								// Выполняем удаление соучастника
								this->_partners.erase(j);
						} break;
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
				if((i->data.ptr != nullptr) && (reinterpret_cast <peer_t *> (i->data.ptr)->socks[0] == sock)){
					// Если событие ещё не удалено из базы событий
					if(!erased){
						// Выполняем изменение параметров события
						result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, &(* i)) == 0);
						// Выполняем закрытие подключения
						::close(sock);
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (reinterpret_cast <peer_t *> (i->data.ptr)->type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление таймера
								this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->socks[0]);							
								// Выполняем поиск соучастника в списке соучастникам
								auto j = this->_partners.find(reinterpret_cast <peer_t *> (i->data.ptr)->socks[1]);
								// Если соучастник в списке соучастников найден, удаляем его
								if(j != this->_partners.end())
									// Выполняем удаление соучастника
									this->_partners.erase(j);
							} break;
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
				result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, sock, nullptr) == 0);
				// Выполняем закрытие подключения
				::close(sock);
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
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if(i != this->_peers.end()){
				// Определяем тип события к которому принадлежит сокет
				switch(static_cast <uint8_t> (i->second.type)){
					// Если событие принадлежит к таймеру
					case static_cast <uint8_t> (event_type_t::TIMER):
					// Если событие принадлежит к потоку
					case static_cast <uint8_t> (event_type_t::STREAM): {
						// Выполняем удаление таймера
						this->_watch.away(i->second.socks[0]);					
						// Выполняем поиск соучастника в списке соучастникам
						auto j = this->_partners.find(i->second.socks[1]);
						// Если соучастник в списке соучастников найден, удаляем его
						if(j != this->_partners.end())
							// Выполняем удаление соучастника
							this->_partners.erase(j);
					} break;
				}
			}
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end(); ++i){
				// Если файловый дескриптор найден
				if((erased = (i->ident == sock))){
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
				if(i->ident == sock){
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
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, error.what());
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
 * @param sock файловый дескриптор для удаления
 * @return   результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_fds.begin(); j != this->_fds.end(); ++j){
					// Если файловый дескриптор найден
					if(j->fd == sock){
						// Очищаем полученное событие
						j->revents = 0;
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (i->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление таймера
								this->_watch.away(i->second.socks[0]);
								// Выполняем поиск соучастника в списке соучастникам
								auto j = this->_partners.find(i->second.socks[1]);
								// Если соучастник в списке соучастников найден, удаляем его
								if(j != this->_partners.end())
									// Выполняем удаление соучастника
									this->_partners.erase(j);
							} break;
						}
						// Выполняем закрытие подключения
						::closesocket(j->fd);
						// Выполняем сброс файлового дескриптора
						j->fd = INVALID_SOCKET;
						// Выполняем удаление события из списка отслеживания
						this->_fds.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если файловый дескриптор найден
					if(static_cast <SOCKET> (j->portev_object) == sock){
						// Удаляем сокет из порта и закрываем
						::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (i->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление таймера
								this->_watch.away(i->second.socks[0]);
								// Выполняем поиск соучастника в списке соучастникам
								auto j = this->_partners.find(i->second.socks[1]);
								// Если соучастник в списке соучастников найден, удаляем его
								if(j != this->_partners.end())
									// Выполняем удаление соучастника
									this->_partners.erase(j);
							} break;
						}
						// Выполняем закрытие подключения
						::close(sock);
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выполняем деинициализацию базы событий
						this->init(event_mode_t::DISABLED);
						// Выполняем инициализацию базы событий
						this->init(event_mode_t::ENABLED);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем удаление таймера
				this->_watch.away(i->second.socks[0]);
				// Определяем тип события к которому принадлежит сокет
				switch(static_cast <uint8_t> (i->second.type)){
					// Если событие принадлежит к таймеру
					case static_cast <uint8_t> (event_type_t::TIMER):
					// Если событие принадлежит к потоку
					case static_cast <uint8_t> (event_type_t::STREAM): {
						// Выполняем поиск соучастника в списке соучастникам
						auto j = this->_partners.find(i->second.socks[1]);
						// Если соучастник в списке соучастников найден, удаляем его
						if(j != this->_partners.end())
							// Выполняем удаление соучастника
							this->_partners.erase(j);
					} break;
				}
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если файловый дескриптор найден
					if((reinterpret_cast <peer_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <peer_t *> (j->data.ptr)->id == id)){
						// Выполняем изменение параметров события
						result = erased = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.socks[0], &(* j)) == 0);
						// Выполняем закрытие подключения
						::close(i->second.socks[0]);
						// Выполняем удаление события из списка отслеживания
						this->_events.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем поиск файлового дескриптора из списка изменений
				for(auto j = this->_change.begin(); j != this->_change.end(); ++j){
					// Если файловый дескриптор найден
					if((reinterpret_cast <peer_t *> (j->data.ptr) == &i->second) &&
					   (reinterpret_cast <peer_t *> (j->data.ptr)->id == id)){
						// Если событие ещё не удалено из базы событий
						if(!erased){
							// Выполняем изменение параметров события
							result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.socks[0], &(* j)) == 0);
							// Выполняем закрытие подключения
							::close(i->second.socks[0]);
						}
						// Выполняем удаление события из списка изменений
						this->_change.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			}
		/**
		 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if((result = (i != this->_peers.end()) && (i->second.id == id))){
				// Флаг удалённого события из базы событий
				bool erased = false;
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				// Выполняем удаление таймера
				this->_watch.away(i->second.socks[0]);
				// Определяем тип события к которому принадлежит сокет
				switch(static_cast <uint8_t> (i->second.type)){
					// Если событие принадлежит к таймеру
					case static_cast <uint8_t> (event_type_t::TIMER):
					// Если событие принадлежит к потоку
					case static_cast <uint8_t> (event_type_t::STREAM): {
						// Выполняем поиск соучастника в списке соучастникам
						auto j = this->_partners.find(i->second.socks[1]);
						// Если соучастник в списке соучастников найден, удаляем его
						if(j != this->_partners.end())
							// Выполняем удаление соучастника
							this->_partners.erase(j);
					} break;
				}
				// Выполняем поиск файлового дескриптора из списка событий
				for(auto j = this->_events.begin(); j != this->_events.end(); ++j){
					// Если файловый дескриптор найден
					if((erased = (j->ident == sock))){
						// Определяем тип события к которому принадлежит сокет
						switch(static_cast <uint8_t> (i->second.type)){
							// Если событие принадлежит к таймеру
							case static_cast <uint8_t> (event_type_t::TIMER):
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Выполняем удаление события таймера
								EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
								// Выполняем закрытие подключения
								::close(i->second.socks[0]);
							} break;
							// Если это другие события
							default: {
								// Выполняем удаление объекта события
								EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
								// Выполняем закрытие подключения
								::close(j->ident);
							}
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
					if(j->ident == sock){
						// Если событие ещё не удалено из базы событий
						if(!erased){
							// Определяем тип события к которому принадлежит сокет
							switch(static_cast <uint8_t> (i->second.type)){
								// Если событие принадлежит к таймеру
								case static_cast <uint8_t> (event_type_t::TIMER):
								// Если событие принадлежит к потоку
								case static_cast <uint8_t> (event_type_t::STREAM): {
									// Выполняем удаление события таймера
									EV_SET(&(* j), j->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
									// Выполняем закрытие подключения
									::close(i->second.socks[0]);
								} break;
								// Если это другие события
								default: {
									// Выполняем удаление объекта события
									EV_SET(&(* j), j->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
									// Выполняем закрытие подключения
									::close(j->ident);
								}
							}
						}
						// Выполняем удаление события из списка изменений
						this->_change.erase(j);
						// Выходим из цикла
						break;
					}
				}
				// Выполняем удаление всего события
				this->_peers.erase(i);
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
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock), log_t::flag_t::CRITICAL, error.what());
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
 * @param sock   файловый дескриптор для удаления
 * @param type тип отслеживаемого события
 * @return     результат работы функции
 */
bool awh::Base::del(const uint64_t id, const SOCKET sock, const event_type_t type) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((sock != INVALID_SOCKET) || (type == event_type_t::TIMER)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы OS Windows
			 */
			#if _WIN32 || _WIN64
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
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
									if((erased = (k->fd == sock))){
										// Очищаем полученное событие
										k->revents = 0;
										// Удаляем флаг ожидания готовности файлового дескриптора на чтение
										k->events ^= POLLIN;
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем закрытие подключения
										::closesocket(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto l = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(l != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(l);
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
									if((erased = (k->fd == sock))){
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
									if((erased = (k->fd == sock))){
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
						this->_peers.erase(i);
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
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
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (static_cast <SOCKET> (k->portev_object) == sock))){
										// Удаляем сокет из порта и закрываем
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем закрытие подключения
										::close(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto l = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(l != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(l);
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Выполняем удаление события из списка отслеживания
										this->_events.erase(k);
										// Выполняем деинициализацию базы событий
										this->init(event_mode_t::DISABLED);
										// Выполняем инициализацию базы событий
										this->init(event_mode_t::ENABLED);
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
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (static_cast <SOCKET> (k->portev_object) == sock))){
										// Удаляем сокет из порта и закрываем
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::WRITE) == i->second.mode.end())){
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
											// Выполняем деинициализацию базы событий
											this->init(event_mode_t::DISABLED);
											// Выполняем инициализацию базы событий
											this->init(event_mode_t::ENABLED);
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
								for(auto k = this->_events.begin(); k != this->_events.end(); ++k){
									// Если файловый дескриптор найден
									if((erased = (static_cast <SOCKET> (k->portev_object) == sock))){
										// Удаляем сокет из порта и закрываем
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Выполняем удаление типа события
										i->second.mode.erase(j);
										// Если список режимов событий пустой
										if(i->second.mode.empty() || (i->second.mode.find(event_type_t::READ) == i->second.mode.end())){
											// Выполняем удаление события из списка отслеживания
											this->_events.erase(k);
											// Выполняем деинициализацию базы событий
											this->init(event_mode_t::DISABLED);
											// Выполняем инициализацию базы событий
											this->init(event_mode_t::ENABLED);
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
						}
					}
					// Если список режимов событий пустой
					if(i->second.mode.empty() || ((i->second.mode.size() == 1) && (i->second.mode.find(event_type_t::CLOSE) != i->second.mode.end())))
						// Выполняем удаление всего события
						this->_peers.erase(i);
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
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
							if((erased = (reinterpret_cast <peer_t *> (k->data.ptr) == &i->second))){
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
									result = (::epoll_ctl(this->_efd, EPOLL_CTL_DEL, i->second.socks[0], &(* k)) == 0);
									// Выполняем удаление события из списка изменений
									this->_change.erase(k);
								// Выполняем изменение параметров события
								} else result = (::epoll_ctl(this->_efd, EPOLL_CTL_MOD, i->second.socks[0], &(* k)) == 0);
								// Определяем тип события к которому принадлежит сокет
								switch(static_cast <uint8_t> (i->second.type)){
									// Если событие принадлежит к таймеру
									case static_cast <uint8_t> (event_type_t::TIMER):
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем закрытие подключения
										::close(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto j = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(j != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(j);
									} break;
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
							if((reinterpret_cast <peer_t *> (k->data.ptr) == &i->second) &&
							   (reinterpret_cast <peer_t *> (k->data.ptr)->id == id)){
								// Определяем тип события к которому принадлежит сокет
								switch(static_cast <uint8_t> (i->second.type)){
									// Если событие принадлежит к таймеру
									case static_cast <uint8_t> (event_type_t::TIMER):
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем закрытие подключения
										::close(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto j = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(j != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(j);
									} break;
								}
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
								// Выходим из цикла
								break;
							}
						}
						// Выполняем удаление всего события
						this->_peers.erase(i);
					}
					// Выполняем разблокировку чтения базы событий
					this->_locker = false;
				}
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Выполняем поиск файлового дескриптора в базе событий
				auto i = this->_peers.find(sock);
				// Если файловый дескриптор есть в базе событий
				if((result = (i != this->_peers.end()) && (i->second.id == id))){
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
									if((erased = (k->ident == sock))){
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем удаление работы события
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем закрытие подключения
										::close(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto l = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(l != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(l);
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
									if((erased = (k->ident == sock))){
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
									if((erased = (k->ident == sock))){
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
							if(k->ident == sock){
								// Определяем тип события к которому принадлежит сокет
								switch(static_cast <uint8_t> (i->second.type)){
									// Если событие принадлежит к таймеру
									case static_cast <uint8_t> (event_type_t::TIMER):
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Выполняем удаление таймера
										this->_watch.away(i->second.socks[0]);
										// Выполняем удаление события таймера
										EV_SET(&(* k), k->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
										// Выполняем закрытие подключения
										::close(i->second.socks[0]);
										// Выполняем поиск соучастника в списке соучастникам
										auto j = this->_partners.find(i->second.socks[1]);
										// Если соучастник в списке соучастников найден, удаляем его
										if(j != this->_partners.end())
											// Выполняем удаление соучастника
											this->_partners.erase(j);
									} break;
									// Если это другие события
									default:
										// Выполняем полное удаление события из базы событий
										EV_SET(&(* k), k->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
								}
								// Выполняем удаление события из списка событий
								this->_events.erase(k);
								// Выходим из цикла
								break;
							}
						}
						// Выполняем удаление всего события
						this->_peers.erase(i);
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
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, error.what());
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
 * @param sock     файловый дескриптор для добавления
 * @param callback функция обратного вызова при получении события
 * @param delay    задержка времени для создания таймеров
 * @param series   флаг серийного таймаута
 * @return         результат работы функции
 */
bool awh::Base::add(const uint64_t id, SOCKET & sock, callback_t callback, const uint32_t delay, const bool series) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((sock != INVALID_SOCKET) || (delay > 0)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Если количество добавленных файловых дескрипторов для отслеживания не достигло предела
			if(this->_peers.size() < static_cast <size_t> (this->_maxCount)){
				// Выполняем блокировку чтения базы событий
				this->_locker = true;
				/**
				 * Для операционной системы OS Windows
				 */
				#if _WIN32 || _WIN64
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если файловый дескриптор есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							auto fds = this->_watch.create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							sock = fds[0];
							// Выполняем добавление таймера в список соучастников
							this->_partners.emplace(fds[1]);
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку файлового дескриптора таймера
							ret.first->second.socks[1] = fds[1];
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
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
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->socks[0] = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Устанавливаем файловый дескриптор в список для отслеживания
							this->_fds.push_back((WSAPOLLFD){});
							// Выполняем установку файлового дескриптора
							this->_fds.back().fd = sock;
							// Сбрасываем состояние события
							this->_fds.back().revents = 0;
						}
					}
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif __sun__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если файловый дескриптор есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							auto fds = this->_watch.create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							sock = fds[0];
							// Выполняем добавление таймера в список соучастников
							this->_partners.emplace(fds[1]);
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку файлового дескриптора таймера
							ret.first->second.socks[1] = fds[1];
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
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
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->socks[0] = sock;
							// Если функция обратного вызова передана
							if(callback != nullptr)
								// Выполняем установку функции обратного вызова
								item->callback = callback;
							// Добавляем в список событий новое событие
							this->_events.push_back(port_event_t());
						}
					}
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если файловый дескриптор есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							auto fds = this->_watch.create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							sock = fds[0];
							// Выполняем добавление таймера в список соучастников
							this->_partners.emplace(fds[1]);
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку файлового дескриптора таймера
							ret.first->second.socks[1] = fds[1];
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
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
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->socks[0] = sock;
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
							if(!(result = (::epoll_ctl(this->_efd, EPOLL_CTL_ADD, sock, &this->_change.back()) == 0))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, delay, series), log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							}
						}
					}
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Выполняем поиск файлового дескриптора в базе событий
					auto i = this->_peers.find(sock);
					// Если файловый дескриптор есть в базе событий
					if((result = (i != this->_peers.end()) && (i->second.id == id))){
						// Если функция обратного вызова передана
						if(callback != nullptr)
							// Выполняем установку функции обратного вызова
							i->second.callback = callback;
					// Если файлового дескриптора в базе событий нет
					} else {
						// Объект текущего события
						peer_t * item = nullptr;
						// Если нам необходимо создать таймер
						if(delay > 0){
							// Выполняем создание сокетов
							auto fds = this->_watch.create();
							// Выполняем инициализацию таймера
							if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
								// Выходим из функции
								return result;
							// Выполняем установку файлового дескриптора таймера
							sock = fds[0];
							// Выполняем добавление таймера в список соучастников
							this->_partners.emplace(fds[1]);
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
							// Выполняем установку файлового дескриптора таймера
							ret.first->second.socks[1] = fds[1];
							// Выполняем установку задержки времени таймера
							ret.first->second.delay = delay;
							// Выполняем установку флага серийной работы таймера
							ret.first->second.series = series;
							// Выполняем установку типа таймера
							ret.first->second.type = event_type_t::TIMER;
							// Выполняем установку событий таймера
							ret.first->second.mode.emplace(event_type_t::TIMER, event_mode_t::DISABLED);
							// Выполняем получение объекта текущего события
							item = &ret.first->second;
						// Если нам необходимо создать обычное событие
						} else {
							// Выполняем добавление в список параметров для отслеживания
							auto ret = this->_peers.emplace(sock, peer_t());
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
							// Устанавливаем идентификатор записи
							item->id = id;
							// Выполняем установку файлового дескриптора события
							item->socks[0] = sock;
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
							this->_change.back().ident = sock;
							// Выполняем смену режима работы отлова события
							EV_SET(&this->_change.back(), this->_change.back().ident, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, item);
						}
					}
				#endif
				// Выполняем разблокировку чтения базы событий
				this->_locker = false;
			// Выводим сообщение об ошибке
			} else this->_log->print("SOCKET=%d cannot be added because the number of events being monitored has already reached the limit of %d", log_t::flag_t::WARNING, sock, static_cast <uint32_t> (this->_maxCount));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, delay, series), log_t::flag_t::CRITICAL, error.what());
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
 * @param sock файловый дескриптор для установки режима работы
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Base::mode(const uint64_t id, const SOCKET sock, const event_type_t type, const event_mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если файловый дескриптор передан верный
	if((sock != INVALID_SOCKET) || (type == event_type_t::TIMER)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем поиск файлового дескриптора в базе событий
			auto i = this->_peers.find(sock);
			// Если файловый дескриптор есть в базе событий
			if((i != this->_peers.end()) && (i->second.id == id)){
				// Выполняем поиск события модуля
				auto j = i->second.mode.find(type);
				// Если событие для изменения режима работы модуля найдено
				if((result = ((j != i->second.mode.end()) && (j->second != mode)))){
					// Выполняем установку режима работы модуля
					j->second = mode;
					/**
					 * Для операционной системы OS Windows
					 */
					#if _WIN32 || _WIN64
						// Если тип установлен как не закрытие подключения
						if(type != event_type_t::CLOSE){
							// Выполняем поиск файлового дескриптора из списка событий
							for(auto k = this->_fds.begin(); k != this->_fds.end(); ++k){
								// Если файловый дескриптор найден
								if(k->fd == sock){
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
													this->_watch.wait(k->fd, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Снимаем флаг ожидания готовности файлового дескриптора на чтение
													k->events ^= POLLIN;
													// Выполняем деактивацию таймера
													this->_watch.away(k->fd);
												} break;
											}
										} break;
										// Если событие принадлежит к потоку
										case static_cast <uint8_t> (event_type_t::STREAM): {
											// Устанавливаем тип события сокета
											i->second.type = type;
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
					 * Для операционной системы Sun Solaris
					 */
					#elif __sun__
						// Определяем тип события
						switch(static_cast <uint8_t> (type)){
							// Если событие установлено как таймер
							case static_cast <uint8_t> (event_type_t::TIMER): {
								// Определяем режим работы модуля
								switch(static_cast <uint8_t> (mode)){
									// Если нужно активировать событие работы таймера
									case static_cast <uint8_t> (event_mode_t::ENABLED): {
										
										
										// Выполняем подписку на получение событий межпротоковой передачи данных
										if(::port_associate(static_cast <uintptr_t> (sock), PORT_SOURCE_USER, static_cast <uintptr_t> (1), 0, (void *) 1) == INVALID_SOCKET) {
											
											cout << " ^^^^^^^^^^ TIMER ENABLED2 " << endl;
											
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										// Выполняем активацию таймера на указанное время
										} else this->_watch.wait(sock, i->second.delay);

										
									} break;
									// Если нужно деактивировать событие работы таймера
									case static_cast <uint8_t> (event_mode_t::DISABLED): {
										
										cout << " ^^^^^^^^^^ TIMER DISABLED " << endl;
										
										// Выполняем отписку от событий сокета
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Выполняем отписку от межпотокового события
										::port_dissociate(this->_pfd, PORT_SOURCE_USER, static_cast <uintptr_t> (1));
										// Выполняем деактивацию таймера
										this->_watch.away(sock);
									} break;
								}
							} break;
							// Если событие принадлежит к потоку
							case static_cast <uint8_t> (event_type_t::STREAM): {
								// Устанавливаем тип события сокета
								i->second.type = type;
								// Определяем режим работы модуля
								switch(static_cast <uint8_t> (mode)){
									// Если нужно активировать событие чтения из сокета
									case static_cast <uint8_t> (event_mode_t::ENABLED): {
										// Ассоциируем сокет: ждём готовности к записи (соединение установлено) + ошибки
										if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), POLLOUT | POLLERR | POLLHUP, nullptr) == INVALID_SOCKET){
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										// Выполняем подписку на получение событий межпротоковой передачи данных
										} else if(::port_associate(this->_pfd, PORT_SOURCE_USER, static_cast <uintptr_t> (1), 0, (void *) 1) == INVALID_SOCKET) {
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если нужно деактивировать событие чтения из сокета
									case static_cast <uint8_t> (event_mode_t::DISABLED): {
										// Выполняем отписку от событий сокета
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Выполняем отписку от межпотокового события
										::port_dissociate(this->_pfd, PORT_SOURCE_USER, static_cast <uintptr_t> (1));
									} break;
								}
							} break;
							// Если событие установлено как отслеживание закрытия подключения
							case static_cast <uint8_t> (event_type_t::CLOSE): {
								// Определяем режим работы модуля
								switch(static_cast <uint8_t> (mode)){
									// Если нужно активировать событие чтения из сокета
									case static_cast <uint8_t> (event_mode_t::ENABLED): {
										// Флаги установки ассоциаций сокета
										short flags = (POLLERR | POLLHUP);
										// Если событие на чтение включено
										if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на чтение
											flags |= POLLIN;
										// Если событие на запись включено
										if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на запись
											flags |= POLLOUT;
										// Ассоциируем сокет: ждём закрытия подключения + ошибки
										if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), flags, nullptr) == INVALID_SOCKET){
											
											cout << " ^^^^^^^^^^ CLOSE ENABLED " << endl;
											
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если нужно деактивировать событие чтения из сокета
									case static_cast <uint8_t> (event_mode_t::DISABLED): {
										// Выполняем отписку от событий сокета
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Флаги установки ассоциаций сокета
										short flags = 0;
										// Если событие на чтение включено
										if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на чтение
											flags |= POLLIN;
										// Если событие на запись включено
										if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на запись
											flags |= POLLOUT;
										// Если события ещё активированны
										if(flags > 0){
											// Ассоциируем сокет: ждём поступления новых данных + ошибки
											if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), flags, nullptr) == INVALID_SOCKET){
												
												cout << " ^^^^^^^^^^ CLOSE DISABLED " << endl;
												
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
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
										// Флаги установки ассоциаций сокета
										short flags = (POLLIN | POLLERR | POLLHUP);
										// Если событие на запись включено
										if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на запись
											flags |= POLLOUT;
										// Ассоциируем сокет: ждём поступления новых данных + ошибки
										if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), flags, nullptr) == INVALID_SOCKET){
											
											cout << " ^^^^^^^^^^ READ ENABLED " << endl;
											
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если нужно деактивировать событие чтения из сокета
									case static_cast <uint8_t> (event_mode_t::DISABLED): {
										// Выполняем отписку от событий сокета
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Если событие на запись включено
										if(i->second.mode.at(event_type_t::WRITE) == event_mode_t::ENABLED){
											// Ассоциируем сокет: ждём готовности к записи (соединение установлено) + ошибки
											if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), POLLOUT | POLLERR | POLLHUP, nullptr) == INVALID_SOCKET){
												
												cout << " ^^^^^^^^^^ READ DISABLED " << endl;
												
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
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
										// Флаги установки ассоциаций сокета
										short flags = (POLLOUT | POLLERR | POLLHUP);
										// Если событие на чтение включено
										if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED)
											// Устанавливаем флаг ассоциации сокета на чтение
											flags |= POLLIN;
										// Ассоциируем сокет: ждём готовности к записи (соединение установлено) + ошибки
										if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), flags, nullptr) == INVALID_SOCKET){
											
											cout << " ^^^^^^^^^^ WRITE ENABLED " << endl;
											
											/**
											 * Если включён режим отладки
											 */
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											 * Если режим отладки не включён
											 */
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
									} break;
									// Если нужно деактивировать событие записи в сокет
									case static_cast <uint8_t> (event_mode_t::DISABLED): {
										// Выполняем отписку от событий сокета
										::port_dissociate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock));
										// Если событие на чтение включено
										if(i->second.mode.at(event_type_t::READ) == event_mode_t::ENABLED){
											// Ассоциируем сокет: ждём поступления новых данных + ошибки
											if(::port_associate(this->_pfd, PORT_SOURCE_FD, static_cast <uintptr_t> (sock), POLLIN | POLLERR | POLLHUP, nullptr) == INVALID_SOCKET){
												
												cout << " ^^^^^^^^^^ WRITE DISABLED " << endl;
												
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
												/**
												 * Если режим отладки не включён
												 */
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
												#endif
											}
										}
									} break;
								}
							} break;
						}
					/**
					 * Для операционной системы Linux
					 */
					#elif __linux__
						// Выполняем поиск файлового дескриптора из списка событий
						for(auto k = this->_change.begin(); k != this->_change.end(); ++k){
							// Если файловый дескриптор найден
							if(reinterpret_cast <peer_t *> (k->data.ptr)->socks[0] == sock){
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем активацию таймера на указанное время
												} else this->_watch.wait(sock, i->second.delay);
											} break;
											// Если нужно деактивировать событие таймера
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												// Выполняем деактивацию таймера
												} else this->_watch.away(sock);
											} break;
										}
									} break;
									// Если событие принадлежит к потоку
									case static_cast <uint8_t> (event_type_t::STREAM): {
										// Устанавливаем тип события сокета
										i->second.type = type;
										// Определяем режим работы модуля
										switch(static_cast <uint8_t> (mode)){
											// Если нужно активировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::ENABLED): {
												// Устанавливаем флаг ожидания готовности файлового дескриптора на чтение
												k->events |= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= (EPOLLIN | EPOLLET);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Выполняем удаление флагов отслеживания закрытия подключения
												k->events ^= (EPOLLRDHUP | EPOLLHUP);
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие чтения из сокета
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на чтение
												k->events ^= EPOLLIN;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
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
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
											} break;
											// Если нужно деактивировать событие записи в сокет
											case static_cast <uint8_t> (event_mode_t::DISABLED): {
												// Снимаем флаг ожидания готовности файлового дескриптора на запись
												k->events ^= EPOLLOUT;
												// Выполняем изменение параметров события
												if(::epoll_ctl(this->_efd, EPOLL_CTL_MOD, sock, &(* k)) != 0){
													/**
													 * Если включён режим отладки
													 */
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													 * Если режим отладки не включён
													 */
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
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
								if(k->ident == sock){
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
													this->_watch.wait(k->ident, i->second.delay);
												} break;
												// Если нужно деактивировать событие работы таймера
												case static_cast <uint8_t> (event_mode_t::DISABLED): {
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
													// Выполняем деактивацию таймера
													this->_watch.away(k->ident);
												} break;
											}
										} break;
										// Если событие принадлежит к потоку
										case static_cast <uint8_t> (event_type_t::STREAM): {
											// Устанавливаем тип события сокета
											i->second.type = type;
											// Определяем режим работы модуля
											switch(static_cast <uint8_t> (mode)){
												// Если нужно активировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::ENABLED):
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, &i->second);
												break;
												// Если нужно деактивировать событие чтения из сокета
												case static_cast <uint8_t> (event_mode_t::DISABLED):
													// Выполняем смену режима работы отлова события
													EV_SET(&(* k), k->ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_DISABLE, 0, 0, &i->second);
												break;
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
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, sock, static_cast <uint16_t> (type), static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
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
		// Выполняем блокировку чтения базы событий
		this->_locker = true;
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_fds.begin(); i != this->_fds.end();){
				// Очищаем полученное событие
				i->revents = 0;
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(i->fd);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_peers.end()){
					// Определяем тип события к которому принадлежит сокет
					switch(static_cast <uint8_t> (j->second.type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем удаление таймера
							this->_watch.away(j->second.socks[0]);
							// Выполняем поиск соучастника в списке соучастникам
							auto k = this->_partners.find(j->second.socks[1]);
							// Если соучастник в списке соучастников найден, удаляем его
							if(k != this->_partners.end())
								// Выполняем удаление соучастника
								this->_partners.erase(k);
						} break;
					}
				}
				// Выполняем закрытие подключения
				::closesocket(i->fd);
				// Выполняем сброс файлового дескриптора
				i->fd = INVALID_SOCKET;
				// Выполняем удаление события из списка отслеживания
				i = this->_fds.erase(i);
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем поиск файлового дескриптора из списка событий
			for(auto i = this->_events.begin(); i != this->_events.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(static_cast <SOCKET> (i->portev_object));
				// Если файловый дескриптор есть в базе событий
				if(j != this->_peers.end()){
					// Определяем тип события к которому принадлежит сокет
					switch(static_cast <uint8_t> (j->second.type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем удаление таймера
							this->_watch.away(j->second.socks[0]);
							// Выполняем поиск соучастника в списке соучастникам
							auto k = this->_partners.find(j->second.socks[1]);
							// Если соучастник в списке соучастников найден, удаляем его
							if(k != this->_partners.end())
								// Выполняем удаление соучастника
								this->_partners.erase(k);
						} break;
					}
				}
				// Удаляем сокет из порта и закрываем
				::port_dissociate(this->_pfd, PORT_SOURCE_FD, i->portev_object);
				// Выполняем закрытие подключения
				::close(static_cast <SOCKET> (i->portev_object));
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем изменение параметров события
				::epoll_ctl(this->_efd, EPOLL_CTL_DEL, reinterpret_cast <peer_t *> (i->data.ptr)->socks[0], &(* i));
				// Выполняем закрытие подключения
				::close(reinterpret_cast <peer_t *> (i->data.ptr)->socks[0]);
				// Определяем тип события к которому принадлежит сокет
				switch(static_cast <uint8_t> (reinterpret_cast <peer_t *> (i->data.ptr)->type)){
					// Если событие принадлежит к таймеру
					case static_cast <uint8_t> (event_type_t::TIMER):
					// Если событие принадлежит к потоку
					case static_cast <uint8_t> (event_type_t::STREAM): {
						// Выполняем удаление таймера
						this->_watch.away(reinterpret_cast <peer_t *> (i->data.ptr)->socks[0]);
						// Выполняем поиск соучастника в списке соучастникам
						auto j = this->_partners.find(reinterpret_cast <peer_t *> (i->data.ptr)->socks[1]);
						// Если соучастник в списке соучастников найден, удаляем его
						if(j != this->_partners.end())
							// Выполняем удаление соучастника
							this->_partners.erase(j);
					} break;
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
				auto j = this->_peers.find(i->ident);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_peers.end()){
					// Определяем тип события к которому принадлежит сокет
					switch(static_cast <uint8_t> (j->second.type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем удаление таймера
							this->_watch.away(j->second.socks[0]);
							// Выполняем удаление события таймера
							EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(j->second.socks[0]);
							// Выполняем поиск соучастника в списке соучастникам
							auto k = this->_partners.find(j->second.socks[1]);
							// Если соучастник в списке соучастников найден, удаляем его
							if(k != this->_partners.end())
								// Выполняем удаление соучастника
								this->_partners.erase(k);
						} break;
						// Если это другое событие
						default: {
							// Выполняем удаление объекта события
							EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(i->ident);
						}
					}
					// Выполняем удаление события
					this->_peers.erase(j);
				}
				// Выполняем удаление события из списка отслеживания
				i = this->_events.erase(i);
			}
			// Выполняем поиск файлового дескриптора из списка изменений
			for(auto i = this->_change.begin(); i != this->_change.end();){
				// Выполняем поиск файлового дескриптора в базе событий
				auto j = this->_peers.find(i->ident);
				// Если файловый дескриптор есть в базе событий
				if(j != this->_peers.end()){
					// Определяем тип события к которому принадлежит сокет
					switch(static_cast <uint8_t> (j->second.type)){
						// Если событие принадлежит к таймеру
						case static_cast <uint8_t> (event_type_t::TIMER):
						// Если событие принадлежит к потоку
						case static_cast <uint8_t> (event_type_t::STREAM): {
							// Выполняем удаление таймера
							this->_watch.away(j->second.socks[0]);
							// Выполняем удаление события таймера
							EV_SET(&(* i), i->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(j->second.socks[0]);
							// Выполняем поиск соучастника в списке соучастникам
							auto k = this->_partners.find(j->second.socks[1]);
							// Если соучастник в списке соучастников найден, удаляем его
							if(k != this->_partners.end())
								// Выполняем удаление соучастника
								this->_partners.erase(k);
						} break;
						// Если это другое событие
						default: {
							// Выполняем удаление объекта события
							EV_SET(&(* i), i->ident, EVFILT_READ | EVFILT_WRITE, EV_DELETE, 0, 0, 0);
							// Выполняем закрытие подключения
							::close(i->ident);
						}
					}
					// Выполняем удаление события
					this->_peers.erase(j);
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
		#if DEBUG_MODE
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
		// Если работа базы событий запущена
		if(this->_started){
			// Выполняем активацию блокировки
			this->_locker = static_cast <bool> (this->_started);
			// Запоминаем список активных событий
			std::map <SOCKET, peer_t> items = this->_peers;
			// Выполняем очистку всех параметров
			this->clear();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
			// Если список активных событий не пустой
			if(!items.empty()){
				// Выполняем перебор всего списка активных событий
				for(auto & item : items){
					// Выполняем добавление события в базу событий
					if(!this->add(item.second.id, item.second.socks[0], item.second.callback, item.second.delay, item.second.series))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.socks[0]);
					// Если событие добавленно удачно
					else {
						// Выполняем перебор всех разрешений на запуск событий
						for(auto & mode : item.second.mode)
							// Выполняем активацию события на запуск
							this->mode(item.second.id, item.second.socks[0], mode.first, mode.second);
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
		#if DEBUG_MODE
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
		// Если работа базы событий запущена
		if(this->_started){
			// Снимаем флаг работы базы событий
			this->_started = !this->_started;
			// Выполняем очистку списка событий
			this->clear();
			// Выполняем деинициализацию базы событий
			this->init(event_mode_t::DISABLED);
			// Выполняем инициализацию базы событий
			this->init(event_mode_t::ENABLED);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
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
	// Если работа базы событий не запущена
	if(!this->_started){
		// Устанавливаем флаг работы базы событий
		this->_started = !this->_started;
		/**
		 * Для операционной системы Sun Solaris
		 */
		#if __sun__
			// Переменная опроса события
			uint32_t poll = 0;
		/**
		 * Для всех остальних операционных систем
		 */
		#else
			// Переменная опроса события
			int32_t poll = 0;
		#endif
		/**
		 * Если это  MacOS X, FreeBSD, NetBSD, OpenBSD или Sun Solaris
		 */
		#if __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __sun__
			// Создаём объект временного таймаута
			struct timespec baseDelay = {0, 0};
			// Если установлен конкретный таймаут
			if((this->_baseDelay > 0) && !this->_easily){
				// Устанавливаем время в секундах
				baseDelay.tv_sec = (this->_baseDelay / 1000);
				// Устанавливаем время счётчика (наносекунды)
				baseDelay.tv_nsec = ((this->_baseDelay % 1000) * 1000000L);
			}
		#endif
		// Запускаем работу часов
		this->_watch.start();
		// Получаем идентификатор потока
		this->_wid = this->wid();
		// Устанавливаем флаг запущенного опроса базы событий
		this->_launched = static_cast <bool> (this->_started);
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			/**
			 * Для операционной системы OS Windows
			 */
			#if _WIN32 || _WIN64
				// Количество событий для опроса
				size_t count = 0;
			#endif
			/**
			 * Выполняем запуск базы события
			 */
			while(this->_started){
				/**
				 * Для операционной системы OS Windows
				 */
				#if _WIN32 || _WIN64
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_fds.empty()){
							// Выполняем опрос базы событий
							poll = ::WSAPoll(this->_fds.data(), this->_fds.size(), (!this->_easily ? static_cast <int32_t> (this->_baseDelay) : 0));
							// Если мы получили ошибку
							if(poll == SOCKET_ERROR){
								// Создаём буфер сообщения ошибки
								wchar_t message[256] = {0};
								// Сбрасываем значение сокета на чтение
								this->_socks[0] = INVALID_SOCKET;
								// Сбрасываем значение сокета на запись
								this->_socks[1] = INVALID_SOCKET;
								// Выполняем формирование текста ошибки
								::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Получаем количество файловых дескрипторов для проверки
								count = this->_fds.size();
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
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
										sock = event.fd;
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
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Получаем идентификатор события
												id = j->second.id;
												// Определяем тип события к которому принадлежит сокет
												switch(static_cast <uint8_t> (j->second.type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::TIMER));
															}
															// Выполняем поиск указанной записи
															j = this->_peers.find(sock);
															// Если сокет в списке найден
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как серийный
																if(j->second.series){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.socks[0], j->second.delay);
																}
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(j->second.id, j->second.socks[0]);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события межпотоковое присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::STREAM);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::STREAM));
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(j->second.id, j->second.socks[0]);
													} break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(j->second.callback, std::make_tuple(sock, event_type_t::READ));
														}
													}
												}
											}
										}
										// Если сокет доступен для записи
										if(isWrite){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if((j != this->_peers.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														std::apply(j->second.callback, std::make_tuple(sock, event_type_t::WRITE));
												}
											}
										}
										// Если сокет отключился или произошла ошибка
										if(isClose || isError){
											// Если мы реально получили ошибку
											if(::WSAGetLastError() > 0){
												// Создаём буфер сообщения ошибки
												wchar_t message[256] = {0};
												// Сбрасываем значение сокета на чтение
												this->_socks[0] = INVALID_SOCKET;
												// Сбрасываем значение сокета на запись
												this->_socks[1] = INVALID_SOCKET;
												// Выполняем формирование текста ошибки
												::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
												/**
												 * Если включён режим отладки
												 */
												#if DEBUG_MODE
													// Выводим сообщение об ошибке
													this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
												/**
												* Если режим отладки не включён
												*/
												#else
													// Выводим сообщение об ошибке
													this->_log->print("%s", log_t::flag_t::CRITICAL, this->_fmk->convert(message).c_str());
												#endif
											}
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Если идентификаторы соответствуют
												if((id == j->second.id) || (id == 0)){
													// Получаем идентификатор события
													id = j->second.id;
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Получаем функцию обратного вызова
														auto callback = std::bind(j->second.callback, sock, event_type_t::CLOSE);
														// Выполняем поиск события на отключение присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::CLOSE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
															// Удаляем файловый дескриптор из базы событий
															this->del(j->second.id, sock);
															// Выполняем функцию обратного вызова
															std::apply(callback, std::make_tuple());
															// Продолжаем обход дальше
															continue;
														}
													}
													// Удаляем файловый дескриптор из базы событий
													this->del(j->second.id, sock);
												}
											// Если файловый дескриптор не принадлежит соучастнику
											} else if(this->_partners.find(sock) == this->_partners.end())
												// Выполняем удаление фантомного файлового дескриптора
												this->del(sock);
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
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы Sun Solaris
				 */
				#elif __sun__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_events.empty()){
							// Формируем количество получаемых сообщений
							poll = static_cast <uint32_t> (this->_events.size());
							// Выполняем ожидание входящих сообщений
							if(::port_getn(this->_pfd, this->_events.data(), poll, &poll, ((this->_baseDelay > -1) || this->_easily ? &baseDelay : nullptr)) == INVALID_SOCKET){
								// Если мы получили сообщение об ошибке
								if(errno != EINTR){
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
									/**
									* Если режим отладки не включён
									*/
									#else
										// Выводим сообщение об ошибке
										this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
									#endif
								}
								// Выполняем пропуск и идём дальше
								continue;
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Список полученных флагов события
								short events = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false,
								     isClose = false, isError = false, isEvent = false;
								// Выполняем перебор всех файловых дескрипторов
								for(int32_t i = 0; i < poll; i++){
									// Зануляем идентификатор события
									id = 0;
									// Получаем список полученных флагов событий
									events = this->_events.at(i).portev_events;
									// Получаем файловый дескриптор
									sock = static_cast <SOCKET> (this->_events.at(i).portev_object);
									// Получаем флаг достуности чтения из сокета
									isRead = (events & POLLIN);
									// Получаем флаг доступности сокета на запись
									isWrite = (events & POLLOUT);
									// Получаем флаг закрытия подключения
									isClose = (events & POLLHUP);
									// Получаем флаг получения ошибки сокета
									isError = ((events & POLLERR) || (events & POLLNVAL));
									// Если мы получили событие сетевого сокета
									if(this->_events.at(i).portev_source == PORT_SOURCE_FD){
										// Если флаг на чтение данных из сокета установлен
										if(isRead){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на получение данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::READ);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														std::apply(j->second.callback, std::make_tuple(sock, event_type_t::READ));
												}
											}
										}
										// Если сокет доступен для записи
										if(isWrite){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if((j != this->_peers.end()) && ((id == j->second.id) || (id == 0))){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Выполняем поиск события на запись данных присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::WRITE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
														// Выполняем функцию обратного вызова
														std::apply(j->second.callback, std::make_tuple(sock, event_type_t::WRITE));
												}
											}
										}
									// Если мы получили событие межпотокового сообщения
									} else if(this->_events.at(i).portev_source == PORT_SOURCE_USER) {
										// Если сообщение пришло то, что нам нужно
										if(this->_events.at(i).portev_user == (void *) 1){
											// Выполняем поиск указанной записи
											auto j = this->_peers.find(sock);
											// Если сокет в списке найден
											if(j != this->_peers.end()){
												// Получаем идентификатор события
												id = j->second.id;
												// Определяем тип события к которому принадлежит сокет
												switch(static_cast <uint8_t> (j->second.type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::TIMER));
															}
															// Выполняем поиск указанной записи
															j = this->_peers.find(sock);
															// Если сокет в списке найден
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как серийный
																if(j->second.series){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.socks[0], j->second.delay);
																}
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(id, sock);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(j->second.callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = j->second.mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(j->second.callback, std::make_tuple(sock, event_type_t::TIMER));
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(id, sock);
													} break;
												}
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
											#if DEBUG_MODE
												// Выводим сообщение об ошибке
												this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
											/**
											* Если режим отладки не включён
											*/
											#else
												// Выводим сообщение об ошибке
												this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
											#endif
										}
										// Выполняем поиск указанной записи
										auto j = this->_peers.find(sock);
										// Если сокет в списке найден
										if(j != this->_peers.end()){
											// Если идентификаторы соответствуют
											if((id == j->second.id) || (id == 0)){
												// Получаем идентификатор события
												id = j->second.id;
												// Если функция обратного вызова установлена
												if(j->second.callback != nullptr){
													// Получаем функцию обратного вызова
													auto callback = std::bind(j->second.callback, sock, event_type_t::CLOSE);
													// Выполняем поиск события на отключение присутствует в базе событий
													auto k = j->second.mode.find(event_type_t::CLOSE);
													// Если событие найдено и оно активированно
													if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
														// Удаляем файловый дескриптор из базы событий
														this->del(j->second.id, sock);
														// Выполняем функцию обратного вызова
														std::apply(callback, std::make_tuple());
														// Продолжаем обход дальше
														continue;
													}
												}
												// Удаляем файловый дескриптор из базы событий
												this->del(j->second.id, sock);
											}
										// Если файловый дескриптор не принадлежит соучастнику
										} else if(this->_partners.find(sock) == this->_partners.end())
											// Выполняем удаление фантомного файлового дескриптора
											this->del(sock);
									}
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
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы Linux
				 */
				#elif __linux__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							// Выполняем опрос базы событий
							poll = ::epoll_wait(this->_efd, this->_events.data(), static_cast <uint32_t> (this->_maxCount), (!this->_easily ? static_cast <int32_t> (this->_baseDelay) : 0));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
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
										peer_t * item = reinterpret_cast <peer_t *> (event.data.ptr);
										// Если объект текущего события получен
										if(item != nullptr){
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											sock = item->socks[0];
											// Если в сокете появились данные для чтения
											if(isRead){
												// Определяем тип события к которому принадлежит сокет
												switch(static_cast <uint8_t> (item->type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto j = item->mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->socks[0], event_type_t::TIMER));
															}
															// Выполняем поиск файлового дескриптора в базе событий
															auto j = this->_peers.find(sock);
															// Если файловый дескриптор есть в базе событий
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как серийный
																if(j->second.series){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.socks[0], j->second.delay);
																}
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(item->id, item->socks[0]);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события межпотоковое присутствует в базе событий
																auto j = item->mode.find(event_type_t::STREAM);
																// Если событие найдено и оно активированно
																if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->socks[0], event_type_t::STREAM));
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(item->id, item->socks[0]);
													} break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto j = item->mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((j != item->mode.end()) && (j->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(item->callback, std::make_tuple(item->socks[0], event_type_t::READ));
														}
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_peers.find(sock);
												// Если файловый дескриптор есть в базе событий
												if((i != this->_peers.end()) && (id == i->second.id)){
													// Если функция обратного вызова установлена
													if(i->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto j = i->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															std::apply(i->second.callback, std::make_tuple(i->second.socks[0], event_type_t::WRITE));
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
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(errno));
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												auto i = this->_peers.find(sock);
												// Если файловый дескриптор есть в базе событий
												if(i != this->_peers.end()){
													// Если идентификаторы соответствуют
													if(id == i->second.id){
														// Если функция обратного вызова установлена
														if(i->second.callback != nullptr){
															// Получаем функцию обратного вызова
															auto callback = std::bind(i->second.callback, i->second.socks[0], event_type_t::CLOSE);
															// Выполняем поиск события на отключение присутствует в базе событий
															auto j = i->second.mode.find(event_type_t::CLOSE);
															// Если событие найдено и оно активированно
															if((j != i->second.mode.end()) && (j->second == event_mode_t::ENABLED)){
																// Удаляем файловый дескриптор из базы событий
																this->del(i->second.id, i->second.socks[0]);
																// Выполняем функцию обратного вызова
																std::apply(callback, std::make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем файловый дескриптор из базы событий
														this->del(i->second.id, i->second.socks[0]);
													}
												// Если файловый дескриптор не принадлежит соучастнику
												} else if(this->_partners.find(sock) == this->_partners.end())
													// Выполняем удаление фантомного файлового дескриптора
													this->del(sock);
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
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				/**
				 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
					// Если опрос базы событий не заблокирован
					if(!this->_locker){
						// Если в списке достаточно событий для опроса
						if(!this->_change.empty()){
							// Выполняем опрос базы событий
							poll = ::kevent(this->_kq, this->_change.data(), this->_change.size(), this->_events.data(), this->_events.size(), ((this->_baseDelay > -1) || this->_easily ? &baseDelay : nullptr));
							// Если мы получили ошибку
							if(poll == INVALID_SOCKET){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
								#endif
							// Если сработал таймаут
							} else if(poll == 0)
								// Компенсируем условие
								poll = 0;
							// Если опрос прошёл успешно
							else {
								// Идентификатор события
								uint64_t id = 0;
								// Код ошибки полученный от ядра
								int32_t code = 0;
								// Файловый дескриптор события
								SOCKET sock = INVALID_SOCKET;
								// Флаги статусов полученного сокета
								bool isRead = false, isWrite = false,
								     isClose = false, isError = false, isEvent = false;
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
										// Получаем флаг нашего кастомного события
										isEvent = (event.filter & EVFILT_USER);
										// Выполняем поиск файлового дескриптора в базе событий
										auto j = this->_peers.find(event.ident);
										// Если файловый дескриптор есть в базе событий
										if(j != this->_peers.end()){
											// Получаем объект текущего события
											peer_t * item = &j->second;
											// Получаем идентификатор события
											id = item->id;
											// Получаем значение текущего идентификатора
											sock = item->socks[0];
											// Если в сокете появились данные для чтения или пользовательское событие
											if(isRead || isEvent){
												// Определяем тип события к которому принадлежит сокет
												switch(static_cast <uint8_t> (item->type)){
													// Если событие принадлежит к таймеру
													case static_cast <uint8_t> (event_type_t::TIMER): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события таймера присутствует в базе событий
																auto k = item->mode.find(event_type_t::TIMER);
																// Если событие найдено и оно активированно
																if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->socks[0], event_type_t::TIMER));
															}
															// Выполняем поиск файлового дескриптора в базе событий
															j = this->_peers.find(sock);
															// Если файловый дескриптор есть в базе событий
															if((j != this->_peers.end()) && (id == j->second.id)){
																// Если таймер установлен как серийный
																if(j->second.series){
																	// Выполняем поиск события таймера присутствует в базе событий
																	auto k = j->second.mode.find(event_type_t::TIMER);
																	// Если событие найдено и оно активированно
																	if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
																		// Выполняем активацию таймера на указанное время
																		this->_watch.wait(j->second.socks[0], j->second.delay);
																}
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(item->id, item->socks[0]);
													} break;
													// Если событие принадлежит к потоку
													case static_cast <uint8_t> (event_type_t::STREAM): {
														// Выполняем чтение данных
														const uint64_t infelicity = this->_watch.event(sock);
														// Если чтение выполнено удачно
														if(infelicity > 0){
															// Если функция обратного вызова установлена
															if(item->callback != nullptr){
																// Выполняем поиск события межпотоковое присутствует в базе событий
																auto k = item->mode.find(event_type_t::STREAM);
																// Если событие найдено и оно активированно
																if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																	// Выполняем функцию обратного вызова
																	std::apply(item->callback, std::make_tuple(item->socks[0], event_type_t::STREAM));
															}
														// Удаляем файловый дескриптор из базы событий
														} else this->del(item->id, item->socks[0]);
													} break;
													// Если это другое событие
													default: {
														// Если функция обратного вызова установлена
														if(item->callback != nullptr){
															// Выполняем поиск события на получение данных присутствует в базе событий
															auto k = item->mode.find(event_type_t::READ);
															// Если событие найдено и оно активированно
															if((k != item->mode.end()) && (k->second == event_mode_t::ENABLED))
																// Выполняем функцию обратного вызова
																std::apply(item->callback, std::make_tuple(sock, event_type_t::READ));
														}
													}
												}
											}
											// Если сокет доступен для записи
											if(isWrite){
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_peers.find(sock);
												// Если файловый дескриптор есть в базе событий
												if((j != this->_peers.end()) && (id == j->second.id)){
													// Если функция обратного вызова установлена
													if(j->second.callback != nullptr){
														// Выполняем поиск события на запись данных присутствует в базе событий
														auto k = j->second.mode.find(event_type_t::WRITE);
														// Если событие найдено и оно активированно
														if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED))
															// Выполняем функцию обратного вызова
															std::apply(j->second.callback, std::make_tuple(j->second.socks[0], event_type_t::WRITE));
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
													#if DEBUG_MODE
														// Выводим сообщение об ошибке
														this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, ::strerror(code));
													/**
													* Если режим отладки не включён
													*/
													#else
														// Выводим сообщение об ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(code));
													#endif
												}
												// Выполняем поиск файлового дескриптора в базе событий
												j = this->_peers.find(sock);
												// Если файловый дескриптор есть в базе событий
												if(j != this->_peers.end()){
													// Если идентификаторы соответствуют
													if(id == j->second.id){
														// Если функция обратного вызова установлена
														if(j->second.callback != nullptr){
															// Получаем функцию обратного вызова
															auto callback = std::bind(j->second.callback, j->second.socks[0], event_type_t::CLOSE);
															// Выполняем поиск события на отключение присутствует в базе событий
															auto k = j->second.mode.find(event_type_t::CLOSE);
															// Если событие найдено и оно активированно
															if((k != j->second.mode.end()) && (k->second == event_mode_t::ENABLED)){
																// Удаляем файловый дескриптор из базы событий
																this->del(j->second.id, j->second.socks[0]);
																// Выполняем функцию обратного вызова
																std::apply(callback, std::make_tuple());
																// Продолжаем обход дальше
																continue;
															}
														}
														// Удаляем файловый дескриптор из базы событий
														this->del(j->second.id, j->second.socks[0]);
													}
												// Если файловый дескриптор не принадлежит соучастнику
												} else if(this->_partners.find(sock) == this->_partners.end())
													// Выполняем удаление фантомного файлового дескриптора
													this->del(sock);
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
								// Продолжаем опрос дальше
								continue;
							// Если опрос базы событий не заблокирован
							} else if(!this->_locker)
								// Продолжаем опрос дальше
								continue;
						}
					}
					// Замораживаем поток на период времени частоты обновления базы событий
					std::this_thread::sleep_for(100ms);
				#endif
			}
			// Останавливаем работу таймеров скрина
			this->_watch.stop();
			// Снимаем флаг запущенного опроса базы событий
			this->_launched = static_cast <bool> (this->_started);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если не происходит отключение работы базы событий
			if(this->_started){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
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
			} else this->_launched = static_cast <bool> (this->_started);
		}
	}
}
/**
 * rebase Метод пересоздания базы событий
 */
void awh::Base::rebase() noexcept {
	// Если метод запущен в дочернем потоке
	if(this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"rebase\" cannot be called in a child thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если работа базы событий запущена
			if(this->_started){
				// Запоминаем список активных событий
				std::map <SOCKET, peer_t> items = this->_peers;
				// Выполняем остановку работы базы событий
				this->stop();
				// Если список активных событий не пустой
				if(!items.empty()){
					// Выполняем перебор всего списка активных событий
					for(auto & item : items){
						// Выполняем добавление события в базу событий
						if(!this->add(item.second.id, item.second.socks[0], item.second.callback))
							// Выводим сообщение что событие не вышло активировать
							this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, item.second.socks[0]);
						// Если событие добавленно удачно
						else {
							// Выполняем перебор всех разрешений на запуск событий
							for(auto & mode : item.second.mode)
								// Выполняем активацию события на запуск
								this->mode(item.second.id, item.second.socks[0], mode.first, mode.second);
						}
					}
				}
				// Выполняем запуск работы базы событий
				this->start();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
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
	// Выполняем активацию блокировки
	this->_locker = mode;
}
/**
 * easily Метод активации простого режима чтения базы событий
 * @param mode флаг активации
 */
void awh::Base::easily(const bool mode) noexcept {
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
	/*
	// Выполняем поиск указанного верхнеуровневого потока
	auto i = this->_upstreams.find(sid);
	// Если верхнеуровневый поток обнаружен
	if(i != this->_upstreams.end()){
		// Выполняем удаление события сокета из базы событий
		if(!this->del(i->first, i->second.read))
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Failed remove upstream event for SOCKET=%d", log_t::flag_t::WARNING, i->second.read);
	*/
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
	/*
		#if !_WIN32 && !_WIN64
			// Выполняем закрытие открытого сокета на запись
			::close(i->second.write);
		#endif
		// Выполняем удаление верхнеуровневого потока из списка
		this->_upstreams.erase(i);
	}
	*/
}
/**
 * launchUpstream Метод запуска верхнеуровневого потока
 * @param sid идентификатор верхнеуровневого потока
 * @param tid идентификатор трансферной передачи
 */
void awh::Base::launchUpstream(const uint64_t sid, const uint64_t tid) noexcept {
	/*
	// Если метод запущен в основном потоке
	if(!this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"launchUpstream\" cannot be called in a main thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		// Выполняем поиск указанного верхнеуровневого потока
		auto i = this->_upstreams.find(sid);
		// Если верхнеуровневый поток обнаружен
		if(i != this->_upstreams.end()){
	*/
			/**
			 * Для операционной системы OS Windows
			 */
			// #if _WIN32 || _WIN64
				// Выполняем отправку сообщения верхнеуровневому потоку
			// 	i->second.pipe->send(i->second.write, tid);
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			// #else
				// Выполняем отправку сообщения верхнеуровневому потоку
			// 	i->second.pipe->send(i->second.write, tid);
			// #endif
		/*
		}
	}
	*/
}
/**
 * emplaceUpstream Метод создания верхнеуровневого потока
 * @param callback функция обратного вызова
 * @return         идентификатор верхнеуровневого потока
 */
uint64_t awh::Base::emplaceUpstream(function <void (const uint64_t)> callback) noexcept {
	// Результат работы функции
	uint64_t result = 0;
	/*
	// Если метод запущен в дочернем потоке
	if(this->isChildThread())
		// Выводим сообщение об ошибке
		this->_log->print("Method \"emplaceUpstream\" cannot be called in a child thread", log_t::flag_t::WARNING);
	// Если запуск производится в основном потоке
	else {
		// Создаём объект пайпа
		auto pipe = std::make_shared <evpipe_t> (this->_fmk, this->_log);
		// Выполняем создание сокетов
		auto fds = pipe->create();
		// Выполняем инициализацию таймера
		if((fds[0] == INVALID_SOCKET) || (fds[1] == INVALID_SOCKET))
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
		ret.first->second.write = fds[1];
		// Выполняем установку функции обратного вызова
		ret.first->second.callback = callback;
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
	*/
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
 _wid(0), _easily(false), _locker(false),
 _started(false), _launched(false), _baseDelay(-1),
 _maxCount(count), _watch(fmk, log), _fmk(fmk), _log(log) {
	// Получаем идентификатор потока
	this->_wid = this->wid();
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
