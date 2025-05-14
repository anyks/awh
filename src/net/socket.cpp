/**
 * @file: socket.cpp
 * @date: 2021-12-19
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
 * Для операционных систем FreeBSD, NetBSD, OpenBSD или MacOS X
 */
#if __NetBSD__ || __OpenBSD__
	/**
	 * Активируем TCP флаги
	 */
	#define notyet
#endif

/**
 * Подключаем заголовочный файл
 */
#include <net/socket.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * noSigILL Метод блокировки сигнала SIGILL
 * @return результат работы функции
 */
bool awh::Socket::noSigILL() const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Создаем структуру активации сигнала
		struct sigaction act;
		// Зануляем структуру
		::memset(&act, 0, sizeof(act));
		// Устанавливаем макрос игнорирования сигнала
		act.sa_handler = SIG_IGN;
		// Устанавливаем флаги перезагрузки
		act.sa_flags = (SA_ONSTACK | SA_RESTART | SA_SIGINFO);
		// Устанавливаем блокировку сигнала
		if(!(result = !static_cast <bool> (::sigaction(SIGILL, &act, nullptr)))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SIG_IGN on signal SIGILL [%s]", log_t::flag_t::WARNING, this->message().c_str());
			#endif
		}
	#endif
	// Все удачно
	return result;
}
/**
 * events Метод активации получения событий SCTP для сокета
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
bool awh::Socket::events(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Если операционной системой является Linux или FreeBSD
	 */
	#if defined(__linux__) || defined(__FreeBSD__)
		// Создаём объект подписки на события
		struct sctp_event_subscribe event;
		// Зануляем объект события
		::memset(&event, 0, sizeof(event));
		// Активируем получение входящих событий
		event.sctp_data_io_event = 1;
		// Выполняем активацию получения событий SCTP для сокета
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SCTP_EVENTS option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * noSigPIPE Метод игнорирования отключения сигнала записи в убитый сокет
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
bool awh::Socket::noSigPIPE(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы Linux
	 */
	#ifdef __linux__
		// Создаем структуру активации сигнала
		struct sigaction act;
		// Зануляем структуру
		::memset(&act, 0, sizeof(act));
		// Устанавливаем макрос игнорирования сигнала
		act.sa_handler = SIG_IGN;
		// Устанавливаем флаг перезагрузки
		act.sa_flags = SA_RESTART;
		// Устанавливаем блокировку сигнала
		if(!(result = !static_cast <bool> (::sigaction(SIGPIPE, &act, nullptr)))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("%s", log_t::flag_t::WARNING, "Cannot set SIG_IGN on signal SIGPIPE [%s]", this->message().c_str());
			#endif
		}
		/*
		// Отключаем вывод сигнала записи в пустой сокет
		if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			// Выводим результат по умолчанию
			return -1;
		*/
	/**
	 * Для операционных систем FreeBSD, NetBSD, OpenBSD или MacOS X
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Устанавливаем параметр
		const int32_t on = 1;
		// Устанавливаем SO_NOSIGPIPE
		if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SO_NOSIGPIPE option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * reuseable Метод разрешающая повторно использовать сокет после его удаления
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
bool awh::Socket::reuseable(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Устанавливаем параметр
		const int32_t on = 1;
		// Разрешаем повторно использовать тот же host:port после отключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&on), sizeof(on))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SO_REUSEADDR option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Устанавливаем параметр
		const int32_t on = 1;
		// Разрешаем повторно использовать тот же host:port после отключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&on), sizeof(on))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SO_REUSEADDR option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		/**
		 * Если операционная система не является Linux
		 */
		#if !defined(__linux__)
			// Разрешаем повторно использовать тот же host:port после отключения
			if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast <const char *> (&on), sizeof(on))))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Cannot set SO_REUSEPORT option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
			}
		#endif
	#endif
	// Выводим результат
	return result;
}
/**
 * closeOnExec Метод разрешения закрывать сокет, после запуска
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
bool awh::Socket::closeOnExec(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Флаги файлового дескриптора
		int32_t flags = 0;
		// Получаем флаги файлового дескриптора 
		if(!(result = ((flags = ::fcntl(fd, F_GETFD, nullptr)) >= 0))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set CLOSE_ON_EXEC option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		// Если флаг ещё не установлен
		if(!(flags & FD_CLOEXEC)){
			// Устанавливаем флаги для файлового дескриптора
			if(!(result = ((flags = ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC)) >= 0))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Cannot set CLOSE_ON_EXEC option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
			}
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * blocking Метод проверки сокета блокирующий режим
 * @param fd файловый дескриптор (сокет)
 * @return   результат работы функции
 */
bool awh::Socket::blocking(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Флаги файлового дескриптора
		int32_t flags = 0;
		// Получаем флаги файлового дескриптора
		if(!(result = ((flags = ::fcntl(fd, F_GETFL, nullptr)) >= 0))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot get BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
		// Определяем в каком статусе установлен флаг сокета
		result = !static_cast <bool> (flags & O_NONBLOCK);
	#endif
	// Выводим результат
	return result;
}
/**
 * blocking Метод установки блокирующего сокета
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Socket::blocking(const SOCKET fd, const mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Определяем режим блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо перевести сокет в блокирующий режим
			case static_cast <uint8_t> (mode_t::ENABLED): {
				// Формируем флаг разблокировки
				u_long flag = 0;
				// Выполняем разблокировку сокета
				if(!(result = !static_cast <bool> (::ioctlsocket(fd, FIONBIO, &flag)))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
			// Если необходимо перевести сокет в неблокирующий режим
			case static_cast <uint8_t> (mode_t::DISABLED): {
				// Формируем флаг разблокировки
				u_long flag = 1;
				// Выполняем разблокировку сокета
				if(!(result = !static_cast <bool> (::ioctlsocket(fd, FIONBIO, &flag)))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set NON_BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
		}
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Флаги файлового дескриптора
		int32_t flags = 0;
		// Получаем флаги файлового дескриптора
		if(!(result = ((flags = ::fcntl(fd, F_GETFL, nullptr)) >= 0))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot get BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		// Определяем режим блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо перевести сокет в блокирующий режим
			case static_cast <uint8_t> (mode_t::ENABLED): {
				// Если флаг уже установлен
				if(flags & O_NONBLOCK){
					// Устанавливаем неблокирующий режим
					if(!(result = (::fcntl(fd, F_SETFL, flags ^ O_NONBLOCK) >= 0))){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим в лог информацию
							this->_log->print("Cannot set BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
						#endif
					}
				}
			} break;
			// Если необходимо перевести сокет в неблокирующий режим
			case static_cast <uint8_t> (mode_t::DISABLED): {
				// Если флаг ещё не установлен
				if(!(flags & O_NONBLOCK)){
					// Устанавливаем неблокирующий режим
					if(!(result = (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0))){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим в лог информацию
							this->_log->print("Cannot set NON_BLOCK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
						#endif
					}
				}
			} break;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * cork Метод активации TCP/CORK
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Socket::cork(const SOCKET fd, const mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы Linux
	 */
	#ifdef __linux__
		// Флаг активации или деактивации алгоритма TCP/CORK
		int32_t flag = -1;
		// Определяем режим установки типа сокета
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо активировать алгоритм TCP/CORK
			case static_cast <uint8_t> (mode_t::ENABLED):
				// Активируем алгоритм TCP/CORK
				flag = 1;
			break;
			// Если необходимо деактивировать алгоритм TCP/CORK
			case static_cast <uint8_t> (mode_t::DISABLED):
				// Деактивируем алгоритм TCP/CORK
				flag = 0;
			break;
		}
		// Устанавливаем TCP_CORK
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set TCP_CORK option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	/**
	 * Для операционных систем FreeBSD, NetBSD, OpenBSD или MacOS X
	 */
	#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		// Флаг активации или деактивации алгоритма TCP/CORK
		int32_t flag = -1;
		// Определяем режим установки типа сокета
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо активировать алгоритм TCP/CORK
			case static_cast <uint8_t> (mode_t::ENABLED):
				// Активируем алгоритм TCP/CORK
				flag = 1;
			break;
			// Если необходимо деактивировать алгоритм TCP/CORK
			case static_cast <uint8_t> (mode_t::DISABLED):
				// Деактивируем алгоритм TCP/CORK
				flag = 0;
			break;
		}
		// Устанавливаем TCP_NOPUSH
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &flag, sizeof(flag))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set TCP_NOPUSH option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	#endif
	// Все удачно
	return result;
}
/**
 * nodelay Метод отключения алгоритма Нейгла
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Socket::nodelay(const SOCKET fd, const mode_t mode) const noexcept {
	// Флаг активации или деактивации алгоритма TCP/NODELAY
	int32_t flag = -1;
	// Результат работы функции
	bool result = false;
	// Определяем режим установки типа сокета
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо активировать алгоритм TCP/NODELAY
		case static_cast <uint8_t> (mode_t::ENABLED):
			// Активируем алгоритм TCP/NODELAY
			flag = 1;
		break;
		// Если необходимо деактивировать алгоритм TCP/NODELAY
		case static_cast <uint8_t> (mode_t::DISABLED):
			// Деактивируем алгоритм TCP/NODELAY
			flag = 0;
		break;
	}
	// Устанавливаем TCP_NODELAY
	if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast <const char *> (&flag), sizeof(flag))))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим в лог информацию
			this->_log->print("Cannot set TCP_NODELAY option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * error Метод получения кода ошибки
 * @param fd файловый дескриптор (сокет)
 * @return   код ошибки на сокете если присутствует
 */
int32_t awh::Socket::error(const SOCKET fd) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Размер кода ошибки
	socklen_t size = sizeof(result);
	// Если мы получили ошибку, выходим сообщение
	if(static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast <char *> (&result), &size))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выполняем извлечение кода ошибки
			result = AWH_ERROR();
			// Если код ошибки получен
			if(result > 0)
				// Выводим в лог информацию
				this->_log->print("Getsockopt for SO_ERROR failed option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message(result).c_str());
		#endif
		// Выходим из функции
		return -1;
	}
	// Выводим результат
	return result;
}
/**
 * message Метод получения текста описания ошибки
 * @param code код ошибки для получения сообщения
 * @return     текст сообщения описания кода ошибки
 */
string awh::Socket::message(const int32_t code) const noexcept {
	// Если код ошибки не передан
	if(code == 0)
		// Выполняем получение кода ошибки
		const_cast <int32_t &> (code) = AWH_ERROR();
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Создаём буфер сообщения ошибки
		wchar_t message[256] = {0};
		// Выполняем формирование текста ошибки
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code, 0, message, 256, 0);
		// Выводим текст полученной ошибки
		return this->_fmk->convert(message);
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Выводим текст полученной ошибки
		return ::strerror(code);
	#endif
}
/**
 * onlyIPv6 Метод включающая или отключающая режим отображения IPv4 на IPv6
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим активации или деактивации
 * @return     результат работы функции
 */
bool awh::Socket::onlyIPv6(const SOCKET fd, const mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	// Устанавливаем параметр
	const uint32_t on = static_cast <uint32_t> (mode == mode_t::ENABLED);
	// Разрешаем повторно использовать тот же host:port после отключения
	if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast <const char *> (&on), sizeof(on))))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим в лог информацию
			this->_log->print("Cannot set IPV6_V6ONLY option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * timeout Метод установки таймаута на чтение из сокета
 * @param fd   файловый дескриптор (сокет)
 * @param msec время таймаута в миллисекундах
 * @param mode режим установки типа сокета
 * @return     результат работы функции
 */
bool awh::Socket::timeout(const SOCKET fd, const uint32_t msec, const mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Определяем флаг блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо установить таймаут на чтение
			case static_cast <uint8_t> (mode_t::READ): {
				// Выполняем установку таймаута на чтение данных из сокета
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast <const char *> (&msec), sizeof(msec))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_RCVTIMEO option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
			// Если необходимо установить таймаут на запись
			case static_cast <uint8_t> (mode_t::WRITE): {
				// Выполняем установку таймаута на чтение данных из сокета
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast <const char *> (&msec), sizeof(msec))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_SNDTIMEO option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
		}
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Создаём объект таймаута
		struct timeval timeout;
		// Устанавливаем время в секундах
		timeout.tv_sec = (msec > 0 ? (msec / 1000) : 0);
		// Устанавливаем время счётчика (микросекунды)
		timeout.tv_usec = (msec > 0 ? ((msec % 1000) * 1000) : 0);
		// Определяем флаг блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо установить таймаут на чтение
			case static_cast <uint8_t> (mode_t::READ): {
				// Выполняем установку таймаута на чтение данных из сокета
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast <void *> (&timeout), sizeof(timeout))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_RCVTIMEO option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
			// Если необходимо установить таймаут на запись
			case static_cast <uint8_t> (mode_t::WRITE): {
				// Выполняем установку таймаута на запись данных в сокет
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast <void *> (&timeout), sizeof(timeout))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_SNDTIMEO option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * timeToLive Метод установки времени жизни сокета
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @param fd     файловый дескриптор (сокет)
 * @param ttl    время жизни файлового дескриптора в секундах (сокета)
 * @return       результат установки времени жизни
 */
bool awh::Socket::timeToLive(const int32_t family, const SOCKET fd, const int32_t ttl) const noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип протокола интернета
	switch(family){
		// Если тип протокола подключения IPv4
		case static_cast <int32_t> (AF_INET): {
			/**
			 * Если мы работаем в MacOS X
			 */
			#ifdef __APPLE__
				// Выполняем получение размер TTL по умолчанию
				const socklen_t mode = (ttl <= 0 ? 128 : static_cast <socklen_t> (ttl));
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IP, IP_TTL, &mode, sizeof(&mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IP_TTL option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			/**
			 * Для операционной системы Windows
			 */
			#elif defined(_WIN32) || defined(_WIN64)
				// Выполняем получение размер TTL по умолчанию
				const int32_t mode = (ttl <= 0 ? 128 : ttl);
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast <const char *> (&mode), sizeof(mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IP_TTL option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Выполняем получение размер TTL по умолчанию
				const socklen_t mode = (ttl <= 0 ? 128 : static_cast <socklen_t> (ttl));
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast <const char *> (&mode), sizeof(mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IP_TTL option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			#endif
		} break;
		// Если тип протокола подключения IPv6
		case static_cast <int32_t> (AF_INET6): {
			/**
			 * Если мы работаем в MacOS X
			 */
			#ifdef __APPLE__
				// Выполняем получение размер TTL по умолчанию
				const socklen_t mode = (ttl <= 0 ? 128 : static_cast <socklen_t> (ttl));
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &mode, sizeof(&mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IPV6_UNICAST_HOPS option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			/**
			 * Для операционной системы Windows
			 */
			#elif defined(_WIN32) || defined(_WIN64)
				// Выполняем получение размер TTL по умолчанию
				const int32_t mode = (ttl <= 0 ? 128 : ttl);
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, reinterpret_cast <const char *> (&mode), sizeof(mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IPV6_UNICAST_HOPS option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Выполняем получение размер TTL по умолчанию
				const socklen_t mode = (ttl <= 0 ? 128 : static_cast <socklen_t> (ttl));
				// Выполняем установку параметров времени жизни файлового дескриптора (сокета)
				if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, reinterpret_cast <const char *> (&mode), sizeof(mode))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set IPV6_UNICAST_HOPS option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			#endif
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * isBind Метод проверки на занятость порта
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @param type   тип сокета SOCK_DGRAM или SOCK_STREAM
 * @param port   номер порта для проверки
 * @return       результат проверки
 */
bool awh::Socket::isBind(const int32_t family, const int32_t type, const uint32_t port) const noexcept {
	// Резульатат работы функции
	bool result = false;
	// Если порт установлен
	if(port > 0){
		// Создаём объект подключения
		sockaddr_in sd;
		// Устанавливаем семейство интернет-подключения
		sd.sin_family = family;
		// Присваиваем петлевой адрес подключения
		sd.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		// Выполняем создание файлового дескриптора (сокета)
		const SOCKET fd = ::socket(family, type, 0);
		// Если сокет создан продолжаем работу
		if(fd != INVALID_SOCKET){
			// Устанавливаем порт
			sd.sin_port = htons(port);
			// Выполняем биндинг порта
			result = (::bind(fd, reinterpret_cast <struct sockaddr *> (&sd), sizeof(sd)) != INVALID_SOCKET);
		// Выводим сообщение об ошибке
		} else this->_log->print("PIPE: %s", log_t::flag_t::CRITICAL, this->message(AWH_ERROR()).c_str());
		/**
		 * Для операционной системы OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Закрываем файловый дескриптор
			::closesocket(fd);
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#else
			// Закрываем файловый дескриптор
			::close(fd);
		#endif
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * keepAlive Метод устанавливает постоянное подключение на сокет
 * @param fd    файловый дескриптор (сокет)
 * @param cnt   максимальное количество попыток
 * @param idle  время через которое происходит проверка подключения
 * @param intvl время между попытками
 * @return      результат работы функции
 */
bool awh::Socket::keepAlive(const SOCKET fd, const int32_t cnt, const int32_t idle, const int32_t intvl) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		{
			// Флаг устанавливаемой опции KeepAlive
			bool option = false;
			// Если мы получили ошибку, выходим сообщение
			if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast <char *> (&option), sizeof(option))))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Setsockopt for SO_KEEPALIVE failed option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
				// Выходим из функции
				return result;
			}
		}{
			// Флаг получения устанавливаемой опции KeepAlive
			int32_t option = 0;
			// Устанавливаем размер опции для чтения
			socklen_t size = sizeof(option);
			// Если мы получили ошибку, выходим сообщение
			if(!(result = !static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast <char *> (&option), &size)))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Getsockopt for SO_KEEPALIVE failed option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
				// Выходим из функции
				return result;
			}
		}/*{
			// Количество возвращаемых байт
			DWORD numBytesReturned = 0;
			// Структура данных времени для установки
			tcp_keepalive ka {1, idle * 1000, intvl * 1000};
			// Устанавливаем оставшиеся параметры (время через которое происходит проверка подключения и время между попытками)
			if(!(result = !static_cast <bool> (WSAIoctl(fd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr)))){
		*/
				/**
				 * Если включён режим отладки
				 */
		/*
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Getsockopt for TCP_KEEPIDLE and TCP_KEEPINTVL failed option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
			}
		}*/
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Устанавливаем параметр
		int32_t keepAlive = 1;
		// Активация постоянного подключения
		if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set SO_KEEPALIVE option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		// Максимальное количество попыток
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set TCP_KEEPCNT option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		/**
		 * Если мы работаем в MacOS X
		 */
		#ifdef __APPLE__
			// Время через которое происходит проверка подключения
			if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle))))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Cannot set TCP_KEEPALIVE option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
				// Выходим из функции
				return result;
			}
		/**
		 * Если мы работаем в FreeBSD, NetBSD, OpenBSD или Linux
		 */
		#elif __linux__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
			// Время через которое происходит проверка подключения
			if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Cannot set TCP_KEEPIDLE option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
				// Выходим из функции
				return result;
			}
		#endif
		// Время между попытками
		if(!(result = !static_cast <bool> (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Cannot set TCP_KEEPINTVL option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
		}
	#endif
	// Выводим результат
	return result;
}
/**
 * listen Метод проверки сокета на прослушиваемость
 * @param fd файловый дескриптор (сокет)
 * @return   результат проверки сокета
 */
bool awh::Socket::listen(const SOCKET fd) const noexcept {
	// Результат работы функции
	bool result = false;
	{
		// Флаг получения устанавливаемой опции KeepAlive
		int32_t option = 0;
		// Устанавливаем размер опции для чтения
		socklen_t size = sizeof(option);
		// Если мы получили ошибку, выходим сообщение
		if(!(result = !static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, reinterpret_cast <char *> (&option), &size)))){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим в лог информацию
				this->_log->print("Getsockopt for SO_ACCEPTCONN failed option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
			#endif
			// Выходим из функции
			return result;
		}
		// Устанавливаем результат
		result = (option > 0);
	}
	// Выводим результат
	return result;
}
/**
 * availability Метод проверки количества находящихся байт в сокете
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим проверки типа сокета
 * @return     запрашиваемый размер буфера
 */
u_long awh::Socket::availability(const SOCKET fd, const mode_t mode) const noexcept {
	// Результат работы функции
	u_long result = 0;
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Определяем флаг блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо получить размер буфера на чтение
			case static_cast <uint8_t> (mode_t::READ): {
				// Выполняем получения количества байт в сокете
				if(static_cast <bool> (::ioctlsocket(fd, FIONREAD, &result))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set FIONREAD option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
		}
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Получаем количество байт находящихся в сокете
		int32_t bytes = 0;
		// Определяем флаг блокировки
		switch(static_cast <uint8_t> (mode)){
			// Если необходимо получить размер буфера на чтение
			case static_cast <uint8_t> (mode_t::READ): {
				// Выполняем получения количества байт в сокете
				if(static_cast <bool> (::ioctl(fd, FIONREAD, &bytes))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set FIONREAD option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
			// Если необходимо получить размер буфера на запись
			case static_cast <uint8_t> (mode_t::WRITE): {
				// Выполняем получения количества байт в сокете
				if(static_cast <bool> (::ioctl(fd, TIOCOUTQ, &bytes))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set TIOCOUTQ option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			} break;
		}
		// Если количество полученных байт больше нуля
		if(bytes > 0)
			// Устанавливаем полученный результат
			result = static_cast <u_long> (bytes);
	#endif
	// Выводим результат
	return result;
}
/**
 * bufferSize Метод получения размера буфера
 * @param fd   файловый дескриптор (сокет)
 * @param mode режим проверки типа сокета
 * @return     запрашиваемый размер буфера
 */
int32_t awh::Socket::bufferSize(const SOCKET fd, const mode_t mode) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Размер результата
	socklen_t size = sizeof(result);
	// Определяем флаг блокировки
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо получить размер буфера на чтение
		case static_cast <uint8_t> (mode_t::READ): {
			// Считываем установленный размер буфера
			if(static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast <char *> (&result), &size))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Get buffer read size wrong on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
			}
		} break;
		// Если необходимо получить размер буфера на запись
		case static_cast <uint8_t> (mode_t::WRITE): {
			// Считываем установленный размер буфера
			if(static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast <char *> (&result), &size))){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим в лог информацию
					this->_log->print("Get buffer write size wrong on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
				#endif
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * bufferSize Метод установки размеров буфера
 * @param fd   файловый дескриптор (сокет)
 * @param size устанавливаемый размер буфера
 * @param mode режим проверки типа сокета
 * @return     результат работы функции
 */
bool awh::Socket::bufferSize(const SOCKET fd, const int32_t size, const mode_t mode) const noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем размер массива опции
	socklen_t length = sizeof(size);
	// Определяем флаг блокировки
	switch(static_cast <uint8_t> (mode)){
		// Если необходимо установить размер буфера на чтение
		case static_cast <uint8_t> (mode_t::READ): {
			// Получаем количество байт для установки
			uint32_t bytes = (size > 0 ? size : AWH_BUFFER_SIZE_RCV);
			// Если размер буфера установлен
			if(bytes > 0){
				// Время между попытками
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast <char *> (&bytes), sizeof(bytes))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_RCVBUF option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
					// Выводим результат
					return result;
				}
				// Считываем установленный размер буфера
				if(!(result = !static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast <char *> (&bytes), &length)))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Get buffer wrong on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			}
		} break;
		// Если необходимо установить размер буфера на запись
		case static_cast <uint8_t> (mode_t::WRITE): {
			// Получаем количество байт для установки
			uint32_t bytes = (size > 0 ? size : AWH_BUFFER_SIZE_SND);
			// Если размер буфера установлен
			if(bytes > 0){
				// Время между попытками
				if(!(result = !static_cast <bool> (::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast <char *> (&bytes), sizeof(bytes))))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Cannot set SO_SNDBUF option on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
					// Выводим результат
					return result;
				}
				// Считываем установленный размер буфера
				if(!(result = !static_cast <bool> (::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast <char *> (&bytes), &length)))){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим в лог информацию
						this->_log->print("Get buffer wrong on SOCKET=%d [%s]", log_t::flag_t::WARNING, fd, this->message().c_str());
					#endif
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
