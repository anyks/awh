/**
 * @file: notifier.cpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация уведомителя событий — межпоточное пробуждение цикла событий через eventfd на Linux,
 *        канал на OpenBSD и Solaris и нативный триггер на macOS, FreeBSD и NetBSD
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Для операционной системы Linux
 */
#if __linux__
	/**
	 * Системные заголовочные файлы
	 */
	#include <sys/epoll.h>
	#include <sys/eventfd.h>
/**
 * Для операционной системы OpenBSD или Sun Solaris
 */
#elif __OpenBSD__ || __sun__
	/**
	 * Системные заголовочные файлы
 	 */
	#include <fcntl.h>
	#include <unistd.h>
/**
 * Для операционной системы  macOS, FreeBSD или NetBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
	/**
	 * Системный заголовочный файл
	 */
	#include <sys/event.h>

	/**
	 * Создаём идентификатор события
	 */
	static constexpr uintptr_t USER_EVENT = 0x01;
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <engine/notifier.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * @brief Функция установки неблокирующего режима сокета
		 *
		 * @param sock сокет для установки режима
		 * @param log  объект для работы с логами
		 *
		 */
		void noblocking(const SOCKET sock, const awh::log_t * log) noexcept {
			// Формируем флаг разблокировки
			u_long flag = 1;
			// Выполняем разблокировку сокета
			if(static_cast <bool> (::ioctlsocket(sock, FIONBIO, &flag))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					// Записываем в лог информацию
					log->print(L"Cannot set NON_BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, sock, message);
				#endif
			}
		}
		/**
		 * @brief Метод создания пары сокетов
		 *
		 * @param socks      список сокетов которые будут инициализированы
		 * @param overlapped флаг установки использования перекрывающихся операций ввода-вывода
		 * @return           результат выполнения операции
		 *
		 */
		int64_t socketpair(SOCKET socks[2], const bool overlapped = true) noexcept {
			/**
			 * Объединение сетевых интерфейсов
			 */
			union {
				struct sockaddr_in inaddr; // Объект слушателя
				struct sockaddr addr;      // Объект подключения
			} a;
			// Получаем размер структуры слушателя
			socklen_t addrlen = sizeof(a.inaddr);
			// Получаем флаги инициализации сокета
			DWORD flags = (overlapped ? WSA_FLAG_OVERLAPPED : 0);
			// Если сокеты пустые
			if(socks == 0){
				// Выполняем формирование ошибки
				::WSASetLastError(WSAEINVAL);
				// Записываем ошибку в лог
				return INVALID_SOCKET;
			}
			// Выполняем изначальную инициализацию структуры сокетов
			socks[0] = socks[1] = INVALID_SOCKET;
			// Создаём сокет слушателя
			const SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			// Если сокет слушателя не создан
			if(listener == INVALID_SOCKET)
				// Записываем ошибку в лог
				return INVALID_SOCKET;
			// Выполняем инициализацию всех сетевых интерфейсов
			::memset(&a, 0, sizeof(a));
			// Устанавливаем нулевой порт так-как он нам не нужен
			a.inaddr.sin_port = 0;
			// Устанавливаем семейство сокетов
			a.inaddr.sin_family = AF_INET;
			// Устанавливаем петлевой сетевой интерфейс (127.0.0.1)
			a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			// Формируем флаг разрешающий переиспользовать данные сокеты
			const int32_t reuse = 1;
			/**
			 * Выполняем инициализацию сокетов на чтение и запись
			 */
			for(;;){
				// Устанавливаем флаг разрешающий переиспользование сокетов
				if(::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&reuse), (socklen_t) sizeof(reuse)) == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Выполняем биндинг полученного сокета
				if(::bind(listener, &a.addr, sizeof(a.inaddr)) == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Обнуляем все сетевые интерфейсы
				::memset(&a, 0, sizeof(a));
				// Извлекаем имя указанного слушателя сокета
				if(::getsockname(listener, &a.addr, &addrlen) == INVALID_SOCKET)
					// Выходим из цикла
					break;
				/**
				 * Win32 GetockName может установить только номер порта, p = 0,0005.
				 * ( http://msdn.microsoft.com/library/ms738543.aspx )
				 */
				// Устанавливаем семейство IPv4-адресов
				a.inaddr.sin_family = AF_INET;
				// Устанавливаем петлевой сетевой интерфейс (127.0.0.1)
				a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				// Запускаем прослушивание порта
				if(::listen(listener, 1) == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Создаём сокет для чтения данных
				socks[0] = ::WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, flags);
				// Если сокет не создан
				if(socks[0] == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Выполняем подключение к сокету на чтение данных
				if(::connect(socks[0], &a.addr, sizeof(a.inaddr)) == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Выполняем разрешение подключения к сокету и это у нас будет сокет на запись
				socks[1] = ::accept(listener, nullptr, nullptr);
				// Если сокет не создан
				if(socks[1] == INVALID_SOCKET)
					// Выходим из цикла
					break;
				// Закрываем сокет слушателя
				::closesocket(listener);
				// Выходим из функции и сообщаем, что все сокеты созданы удачно
				return 0;
			}
			// Получаем ошибки сгенерированные системой
			const int32_t error = ::WSAGetLastError();
			// Закрываем сокет слушателя
			::closesocket(listener);
			// Закрываем сокет чтение данных
			::closesocket(socks[0]);
			// Закрываем сокет для записи данных
			::closesocket(socks[1]);
			// Выполняем регистрацию ошибки
			::WSASetLastError(error);
			// Выполняем сброс значения сокетов
			socks[0] = socks[1] = INVALID_SOCKET;
			// Записываем ошибку в лог
			return INVALID_SOCKET;
		}
	/**
	 * Для операционной системы OpenBSD или Sun Solaris
	 */
	#elif __OpenBSD__ || __sun__
		/**
		 * @brief Функция установки неблокирующего режима сокета
		 *
		 * @param sock сокет для установки режима
		 * @param log  объект для работы с логами
		 *
		 */
		void noblocking(const SOCKET sock, const awh::log_t * log) noexcept {
			// Флаги сетевого сокета
			int32_t flags = 0;
			// Получаем флаги сетевого сокета
			if(!((flags = ::fcntl(sock, F_GETFL, nullptr)) >= 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем в лог информацию
					log->print("Cannot get \"BLOCK\" option on SOCKET=%d [%s]", log_t::flag_t::WARNING, sock, ::strerror(errno));
				#endif
				// Выходим из функции
				return;
			}
			// Если флаг ещё не установлен
			if(!(flags & O_NONBLOCK)){
				// Устанавливаем неблокирующий режим
				if(!(::fcntl(sock, F_SETFL, flags | O_NONBLOCK) >= 0)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем в лог информацию
						log->print("Cannot set \"NON_BLOCK\" option on SOCKET=%d [%s]", log_t::flag_t::WARNING, sock, ::strerror(errno));
					#endif
				}
			}
		}
	#endif
}

/**
 * @brief Метод сброса уведомителя
 *
 */
void awh::Notifier::reset() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_socks[0] != INVALID_SOCKET){
				// Закрываем сокет на чтение
				::closesocket(this->_socks[0]);
				// Сбрасываем значение сокета на чтение
				this->_socks[0] = INVALID_SOCKET;
			}
			// Если сокет ещё не закрыт
			if(this->_socks[1] != INVALID_SOCKET){
				// Закрываем сокет на запись
				::closesocket(this->_socks[1]);
				// Сбрасываем значение сокета на запись
				this->_socks[1] = INVALID_SOCKET;
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не закрыт
			if(this->_sock != INVALID_SOCKET){
				// Выполняем закрытие сокета
				::close(this->_sock);
				// Сбрасываем значение сокета
				this->_sock = INVALID_SOCKET;
			}
		/**
		 * Для операционной системы OpenBSD или Sun Solaris
		 */
		#elif __OpenBSD__ || __sun__
			// Если сокет ещё не закрыт
			if(this->_socks[0] != INVALID_SOCKET){
				// Закрываем сокет на чтение
				::close(this->_socks[0]);
				// Сбрасываем значение сокета на чтение
				this->_socks[0] = INVALID_SOCKET;
			}
			// Если сокет ещё не закрыт
			if(this->_socks[1] != INVALID_SOCKET){
				// Закрываем сокет на запись
				::close(this->_socks[1]);
				// Сбрасываем значение сокета на запись
				this->_socks[1] = INVALID_SOCKET;
			}
		/**
		 * Для операционной системы macOS, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не закрыт
			if(this->_sock != INVALID_SOCKET){
				// Создаём объект события
				struct kevent event;
				// Выполняем удаление события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
				// Выполняем обновления ядра операционной системы
				::kevent(this->_sock, &event, 1, nullptr, 0, nullptr);
				// Выполняем закрытие сокета
				::close(this->_sock);
				// Сбрасываем значение сокета
				this->_sock = INVALID_SOCKET;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод инициализации уведомителя
 *
 * @return содержимое сокета для извлечения
 *
 */
SOCKET awh::Notifier::init() noexcept {
	// Переменная результата
	SOCKET result = INVALID_SOCKET;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокеты ещё не инициализированны
			if((this->_socks[0] == INVALID_SOCKET) || (this->_socks[1] == INVALID_SOCKET)){
				// Выполняем инициализацию сокета события
				if(::socketpair(this->_socks) == INVALID_SOCKET){
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
						// Записываем ошибку в лог
						this->_log->debug(L"%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, message);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print(L"%s", log_t::flag_t::CRITICAL, message);
					#endif
				// Делаем сокет неблокирующим
				} else ::noblocking(this->_socks[0], this->_log);
			}
			// Устанавливаем данные сокета на чтение
			result = this->_socks[0];
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не инициализирован
			if(this->_sock == INVALID_SOCKET){
				// Выполняем инициализацию сокета события
				this->_sock = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
				// Если сокет не создан
				if(this->_sock == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			}
			// Устанавливаем данные сокета на чтение
			result = this->_sock;
		/**
		 * Для операционной системы OpenBSD или Sun Solaris
		 */
		#elif __OpenBSD__ || __sun__
			// Если сокеты ещё не инициализированны
			if((this->_socks[0] == INVALID_SOCKET) || (this->_socks[1] == INVALID_SOCKET)){
				// Выполняем инициализацию сокета события
				if(::pipe(this->_socks) == INVALID_SOCKET){
					// Сбрасываем значение сокета на чтение
					this->_socks[0] = INVALID_SOCKET;
					// Сбрасываем значение сокета на запись
					this->_socks[1] = INVALID_SOCKET;
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				// Делаем сокет неблокирующим
				} else ::noblocking(this->_socks[0], this->_log);
			}
			// Устанавливаем данные сокета на чтение
			result = this->_socks[0];
		/**
		 * Для операционной системы macOS, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не инициализирован
			if(this->_sock == INVALID_SOCKET){
				// Выполняем инициализацию сокета события
				this->_sock = ::kqueue();
				// Если сокет не создан
				if(this->_sock == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				// Если сокет инициализирован удачно
				} else {
					// Создаём объект события
					struct kevent event;
					// Выполняем удаление события
					EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
					// Выполняем обновления ядра операционной системы
					::kevent(this->_sock, &event, 1, nullptr, 0, nullptr);
					// Выполняем зануление объекта события
					::memset(&event, 0, sizeof(event));
					// Выполняем активацию события
					EV_SET(&event, USER_EVENT, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
					// Выполняем активацию нашего события
					if(::kevent(this->_sock, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Записываем ошибку в лог
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
						#endif
					}
				}
			}
			// Устанавливаем данные сокета на чтение
			result = this->_sock;
		#endif
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения идентификатора события
 *
 * @return идентификатор события
 *
 */
uint32_t awh::Notifier::event() noexcept {
	// Переменная результата
	uint32_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_socks[0] != INVALID_SOCKET){
				// Буфер данных для чтения
				char buffer[4];
				// Общий размер прочитанных данных
				int8_t size = 0;
				// Количество прочитанных данных
				int8_t bytes = 0;
				/**
				 * Выполняем чтение данных пока не прочитаем все
				 */
				while(size < 4){
					// Выполняем чтение данных
					bytes = static_cast <int8_t> (::recv(this->_socks[0], buffer + size, 4 - size, 0));
					// Если данные прочитанны
					if(bytes > 0)
						// Увеличиваем количество прочитанных данных
						size += bytes;
				}
				// Копируем прочитанные данные
				::memcpy(&result, buffer, size);
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не закрыт
			if(this->_sock != INVALID_SOCKET){
				// Выполняем блокировку потока
				const locker_t lock(this->_mtx);
				// Если очередь событий не пустая
				if(!this->_events.empty()){
					// Выполняем извлечение из очереди события
					result = this->_events.front();
					// Удаляем извлечённое событие
					this->_events.pop();
				}
				// Текущее значение счётчика
				uint64_t counter = 0;
				// Выполняем чтение данных
				if(::read(this->_sock, &counter, sizeof(counter)) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы OpenBSD или Sun Solaris
		 */
		#elif __OpenBSD__ || __sun__
			// Если сокет ещё не закрыт
			if(this->_socks[0] != INVALID_SOCKET){
				// Буфер данных для чтения
				char buffer[4];
				// Общий размер прочитанных данных
				int8_t size = 0;
				// Количество прочитанных данных
				int8_t bytes = 0;
				/**
				 * Выполняем чтение данных пока не прочитаем все
				 */
				while(size < 4){
					// Выполняем чтение данных
					bytes = static_cast <int8_t> (::read(this->_socks[0], buffer + size, 4));
					// Если данные прочитанны
					if(bytes > 0)
						// Увеличиваем количество прочитанных данных
						size += bytes;
				}
				// Копируем прочитанные данные
				::memcpy(&result, buffer, size);
			}
		/**
		 * Для операционной системы macOS, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Выполняем блокировку потока
			const locker_t lock(this->_mtx);
			// Если очередь событий не пустая
			if(!this->_events.empty()){
				// Выполняем извлечение из очереди события
				result = this->_events.front();
				// Удаляем извлечённое событие
				this->_events.pop();
			}
			// Если сообщений больше нет, удаляем событие
			if(this->_events.empty()){
				// Создаём объект события
				struct kevent event;
				// Выполняем удаление события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
				// Выполняем обновления ядра операционной системы
				::kevent(this->_sock, &event, 1, nullptr, 0, nullptr);
				// Выполняем зануление объекта события
				::memset(&event, 0, sizeof(event));
				// Выполняем активацию события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
				// Выполняем активацию нашего события
				if(::kevent(this->_sock, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::strerror(errno));
					#endif
				}
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод отправки уведомления
 *
 * @param id идентификатор для отправки
 *
 */
void awh::Notifier::notify(const uint32_t id) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_socks[1] != INVALID_SOCKET){
				// Выполняем отправку сообщения
				if(::send(this->_socks[1], reinterpret_cast <const char *> (&id), sizeof(id), 0) < sizeof(id)){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug(L"%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::WARNING, message);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print(L"%s", log_t::flag_t::WARNING, message);
					#endif
				}
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не закрыт
			if(this->_sock != INVALID_SOCKET){
				// Выполняем блокировку потока
				const locker_t lock(this->_mtx);
				// Удаляем извлечённое событие
				this->_events.push(id);
				// Значение счетчика события
				const uint64_t value = 1;
				// Выполняем отправку сообщения
				if(::write(this->_sock, reinterpret_cast <const char *> (&value), sizeof(value)) < sizeof(value)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы OpenBSD или Sun Solaris
		 */
		#elif __OpenBSD__ || __sun__
			// Если сокет ещё не закрыт
			if(this->_socks[1] != INVALID_SOCKET){
				// Выполняем отправку сообщения
				if(::write(this->_socks[1], reinterpret_cast <const char *> (&id), sizeof(id)) < sizeof(id)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы macOS, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не закрыт
			if(this->_sock != INVALID_SOCKET){
				// Выполняем блокировку потока
				const locker_t lock(this->_mtx);
				// Удаляем извлечённое событие
				this->_events.push(id);
				// Создаём событие триггера
				struct kevent trigger;
				// Выполняем установку события триггера
				EV_SET(&trigger, USER_EVENT, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
				// Выполняем отправку события триггера
				if(::kevent(this->_sock, &trigger, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Записываем ошибку в лог
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Notifier::Notifier(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
		/**
		 * Для операционной системы MS Windows или OpenBSD или Sun Solaris
		 */
		#if _WIN32 || _WIN64 || __OpenBSD__ || __sun__
			// Сбрасываем значение сокета на чтение
			this->_socks[0] = INVALID_SOCKET;
			// Сбрасываем значение сокета на запись
			this->_socks[1] = INVALID_SOCKET;
		/**
		 * Для операционной системы macOS, FreeBSD, NetBSD или Linux
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__
			// Инициализируем файловый дескриптор
			this->_sock = INVALID_SOCKET;
		#endif
	}
/**
 * @brief Деструктор
 *
 */
awh::Notifier::~Notifier() noexcept {
	// Выполняем сброс всех параметров
	this->reset();
}
