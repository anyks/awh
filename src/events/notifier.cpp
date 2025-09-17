/**
 * @file: notifier.cpp
 * @date: 2025-09-16
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
#include <events/notifier.hpp>

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64

/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	/**
	 * Подключаем системные заголовки
	 */
	#include <port.h>

	/**
	 * Создаём идентификатор события
	 */
	static constexpr uintptr_t USER_EVENT = 1;
/**
 * Для операционной системы Linux
 */
#elif __linux__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/epoll.h>
	#include <sys/eventfd.h>
/**
 * Для операционной системы OpenBSD
 */
#elif __OpenBSD__
	/**
	 * Подключаем системные заголовки
	 */
	#include <fcntl.h>
	#include <unistd.h>
/**
 * Для операционной системы  MacOS X, FreeBSD или NetBSD
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
	/**
	 * Подключаем системные заголовки
	 */
	#include <sys/event.h>

	/**
	 * Создаём идентификатор события
	 */
	static constexpr uintptr_t USER_EVENT = 1;
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * reset Метод сброса уведомителя
 */
void awh::Notifier::reset() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_fds[0] != nullptr){
				// Закрываем сокет на чтение
				::CloseHandle(this->_fds[0]);
				// Сбрасываем значение сокета на чтение
				this->_fds[0] = nullptr;
			}
			// Если сокет ещё не закрыт
			if(this->_fds[1] != nullptr){
				// Закрываем сокет на запись
				::CloseHandle(this->_fds[1]);
				// Сбрасываем значение сокета на запись
				this->_fds[1] = nullptr;
			}
		/**
		 * Для операционной системы Linux или Sun Solaris
		 */
		#elif __linux__ || __sun__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Выполняем закрытие сокета
				::close(this->_fd);
				// Сбрасываем значение сокета
				this->_fd = INVALID_SOCKET;
			}
		/**
		 * Для операционной системы OpenBSD
		 */
		#elif __OpenBSD__
			// Если сокет ещё не закрыт
			if(this->_fds[0] != INVALID_SOCKET){
				// Закрываем сокет на чтение
				::close(this->_fds[0]);
				// Сбрасываем значение сокета на чтение
				this->_fds[0] = INVALID_SOCKET;
			}
			// Если сокет ещё не закрыт
			if(this->_fds[1] != INVALID_SOCKET){
				// Закрываем сокет на запись
				::close(this->_fds[1]);
				// Сбрасываем значение сокета на запись
				this->_fds[1] = INVALID_SOCKET;
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Создаём объект события
				struct kevent event;
				// Выполняем удаление события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
				// Выполняем обновления ядра операционной системы
				::kevent(this->_fd, &event, 1, nullptr, 0, nullptr);
				// Выполняем закрытие сокета
				::close(this->_fd);
				// Сбрасываем значение сокета
				this->_fd = INVALID_SOCKET;
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
 * init Метод инициализации уведомителя
 * @return содержимое сокета для извлечения
 */
std::array <SOCKET, 2> awh::Notifier::init() noexcept {
	// Результат работы функции
	std::array <SOCKET, 2> result = {
		INVALID_SOCKET,
		INVALID_SOCKET
	};
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокеты ещё не инициализированны
			if((this->_fds[0] == nullptr) && (this->_fds[1] == nullptr)){
				// Выполняем инициализацию сокета события
				if(!::CreatePipe(&this->_fds[0], &this->_fds[1], nullptr, 0)){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Сбрасываем значение сокета на чтение
					this->_fds[0] = nullptr;
					// Сбрасываем значение сокета на запись
					this->_fds[1] = nullptr;
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
				}
			}
			// Устанавливаем данные сокета на чтение
			result[0] = (SOCKET) this->_fds[0];
			// Устанавливаем данные сокета на запись
			result[1] = (SOCKET) this->_fds[1];
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если сокет ещё не инициализирован
			if(this->_fd == INVALID_SOCKET){
				// Выполняем инициализацию сокета события
				this->_fd = ::port_create();
				// Если сокет не создан
				if(this->_fd == INVALID_SOCKET){
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
			}
			// Устанавливаем данные сокета на чтение
			result[0] = this->_fd;
			// Устанавливаем данные сокета на запись
			result[1] = this->_fd;
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не инициализирован
			if(this->_fd == INVALID_SOCKET){
				// Выполняем инициализацию сокета события
				this->_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
				// Если сокет не создан
				if(this->_fd == INVALID_SOCKET){
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
			}
			// Устанавливаем данные сокета на чтение
			result[0] = this->_fd;
			// Устанавливаем данные сокета на запись
			result[1] = this->_fd;
		/**
		 * Для операционной системы OpenBSD
		 */
		#elif __OpenBSD__
			// Если сокеты ещё не инициализированны
			if((this->_fds[0] == INVALID_SOCKET) && (this->_fds[1] == INVALID_SOCKET)){
				// Выполняем инициализацию сокета события
				if(::pipe(this->_fds) == INVALID_SOCKET){
					// Сбрасываем значение сокета на чтение
					this->_fds[0] = INVALID_SOCKET;
					// Сбрасываем значение сокета на запись
					this->_fds[1] = INVALID_SOCKET;
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
				// Установим неблокирующий режим на чтение
				} else ::fcntl(this->_fds[0], F_SETFL, O_NONBLOCK);
			}
			// Устанавливаем данные сокета на чтение
			result[0] = this->_fds[0];
			// Устанавливаем данные сокета на запись
			result[1] = this->_fds[1];
		/**
		 * Для операционной системы MacOS X, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не инициализирован
			if(this->_fd == INVALID_SOCKET){
				// Выполняем инициализацию сокета события
				this->_fd = ::kqueue();
				// Если сокет не создан
				if(this->_fd == INVALID_SOCKET){
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
			}
			// Если сокет уже инициализирован
			if(this->_fd != INVALID_SOCKET){
				// Создаём объект события
				struct kevent event;
				// Выполняем удаление события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
				// Выполняем активацию события
				EV_SET(&event, USER_EVENT, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
				// Выполняем активацию нашего события
				if(::kevent(this->_fd, &event, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
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
			}
			// Устанавливаем данные сокета на чтение
			result[0] = this->_fd;
			// Устанавливаем данные сокета на запись
			result[1] = this->_fd;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * event Метод извлечения идентификатора события
 * @return идентификатор события
 */
uint64_t awh::Notifier::event() noexcept {
	// Результат работы функции
	uint64_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_fds[0] != nullptr){
				// Буфер данных для чтения
				char buffer[8];
				// Общий размер прочитанных данных
				DWORD size = 0;
				// Количество прочитанных данных
				DWORD bytes = 0;
				// Выполняем чтение данных пока не прочитаем все
				while(size < 8){
					// Выполняем чтение данных
					::ReadFile(this->_fds[0], buffer + size, 8, &bytes, nullptr);
					// Если данные прочитанны
					if(bytes > 0)
						// Увеличиваем количество прочитанных данных
						size += bytes;
				}
				// Копируем прочитанные данные
				::memcpy(&result, buffer, size);
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Если очередь событий не пустая
			if(!this->_events.empty()){
				// Выполняем извлечение из очереди события
				result = this->_events.front();
				// Удаляем извлечённое событие
				this->_events.pop();
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Буфер данных для чтения
				char buffer[8];
				// Общий размер прочитанных данных
				int8_t size = 0;
				// Количество прочитанных данных
				int8_t bytes = 0;
				// Выполняем чтение данных пока не прочитаем все
				while(size < 8){
					// Выполняем чтение данных
					bytes = static_cast <int8_t> (::read(this->_fd, &buffer + size, 8));
					// Если данные прочитанны
					if(bytes > 0)
						// Увеличиваем количество прочитанных данных
						size += bytes;
				}
				// Копируем прочитанные данные
				::memcpy(&result, buffer, size);
			}
		/**
		 * Для операционной системы OpenBSD
		 */
		#elif __OpenBSD__
			// Если сокет ещё не закрыт
			if(this->_fds[0] != INVALID_SOCKET){
				// Буфер данных для чтения
				char buffer[8];
				// Общий размер прочитанных данных
				int8_t size = 0;
				// Количество прочитанных данных
				int8_t bytes = 0;
				// Выполняем чтение данных пока не прочитаем все
				while(size < 8){
					// Выполняем чтение данных
					bytes = static_cast <int8_t> (::read(this->_fds[0], &buffer + size, 8));
					// Если данные прочитанны
					if(bytes > 0)
						// Увеличиваем количество прочитанных данных
						size += bytes;
				}
				// Копируем прочитанные данные
				::memcpy(&result, buffer, size);
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD или NetBSD 
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Создаём объект события
			struct kevent event;
			// Выполняем удаление события
			EV_SET(&event, USER_EVENT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
			// Выполняем обновления ядра операционной системы
			::kevent(this->_fd, &event, 1, nullptr, 0, nullptr);
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Если очередь событий не пустая
			if(!this->_events.empty()){
				// Выполняем извлечение из очереди события
				result = this->_events.front();
				// Удаляем извлечённое событие
				this->_events.pop();
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
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
 * notify Метод отправки уведомления
 * @param id идентификатор для отправки
 */
void awh::Notifier::notify(const uint64_t id) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Для операционной системы OS Windows
		 */
		#if _WIN32 || _WIN64
			// Если сокет ещё не закрыт
			if(this->_fds[1] != nullptr){
				// Количество отправленных байт
				DWORD bytes = 0;
				// Выполняем отправку сообщения
				::WriteFile(this->_fds[1], &id, sizeof(id), &bytes, nullptr);
				// Если данные отправлены не полностью
				if(bytes < sizeof(id)){
					// Создаём буфер сообщения ошибки
					wchar_t message[256] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 256, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, this->_fmk->convert(message).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, this->_fmk->convert(message).c_str());
					#endif
				}
			}
		/**
		 * Для операционной системы Sun Solaris
		 */
		#elif __sun__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Выполняем блокирование потока
				this->_mtx.lock();
				// Удаляем извлечённое событие
				this->_events.push(id);
				// Выполняем разблокирование потока
				this->_mtx.unlock();
				// Выполняем отправку сообщения в порт
				if(::port_send(this->_fd, USER_EVENT, nullptr) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы Linux
		 */
		#elif __linux__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Выполняем отправку сообщения
				if(::write(this->_fd, &id, sizeof(id)) < sizeof(id)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы OpenBSD
		 */
		#elif __OpenBSD__
			// Если сокет ещё не закрыт
			if(this->_fds[1] != INVALID_SOCKET){
				// Выполняем отправку сообщения
				if(::write(this->_fds[1], &id, sizeof(id)) < sizeof(id)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, ::strerror(errno));
					#endif
				}
			}
		/**
		 * Для операционной системы MacOS X, FreeBSD или NetBSD
		 */
		#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__
			// Если сокет ещё не закрыт
			if(this->_fd != INVALID_SOCKET){
				// Выполняем блокирование потока
				this->_mtx.lock();
				// Удаляем извлечённое событие
				this->_events.push(id);
				// Выполняем разблокирование потока
				this->_mtx.unlock();
				// Создаём событие триггера
				struct kevent trigger;
				// Выполняем установку события триггера
				EV_SET(&trigger, USER_EVENT, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
				// Выполняем отправку события триггера
				if(::kevent(this->_fd, &trigger, 1, nullptr, 0, nullptr) == INVALID_SOCKET){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, ::strerror(errno));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
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
 * Notifier Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Notifier::Notifier(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	/**
	 * Для операционной системы OS Windows или OpenBSD
	 */
	#if _WIN32 || _WIN64 || __OpenBSD__
		// Сбрасываем значение сокета на чтение
		this->_fds[0] = nullptr;
		// Сбрасываем значение сокета на запись
		this->_fds[1] = nullptr;
	/**
	 * Для операционной системы MacOS X, FreeBSD, NetBSD, Linux или Sun Solaris
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __linux__ || __sun__
		// Инициализируем файловый дескриптор
		this->_fd = INVALID_SOCKET;
	#endif
}
/**
 * ~Notifier Деструктор
 */
awh::Notifier::~Notifier() noexcept {
	// Выполняем сброс всех параметров
	this->reset();
}
