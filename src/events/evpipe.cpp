/**
 * @file: evpipe.cpp
 * @date: 2024-07-03
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
#include <events/evpipe.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Для операционной системы OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * socketpair Метод создания пары сокетов
	 * @param socks      список сокетов которые будут инициализированы
	 * @param overlapped флаг установки использования перекрывающихся операций ввода-вывода
	 * @return           результат выполнения операции
	 */
	static int32_t socketpair(SOCKET & socks[2], const bool overlapped = true) noexcept {
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
			WSASetLastError(WSAEINVAL);
			// Выводим ошибку
			return SOCKET_ERROR;
		}
		// Выполняем изначальную инициализацию структуры сокетов
		socks[0] = socks[1] = -1;
		// Создаём сокет слушателя
		const SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		// Если сокет слушателя не создан
		if(listener == -1)
			// Выводим ошибку
			return SOCKET_ERROR;
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
			if(::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&reuse), (socklen_t) sizeof(reuse)) == -1)
				// Выходим из цикла
				break;
			// Выполняем биндинг полученного сокета
			if(::bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
				// Выходим из цикла
				break;
			// Обнуляем все сетевые интерфейсы
			::memset(&a, 0, sizeof(a));
			// Извлекаем имя указанного слушателя сокета
			if(::getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
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
			if(::listen(listener, 1) == SOCKET_ERROR)
				// Выходим из цикла
				break;
			// Создаём сокет для чтения данных
			socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, flags);
			// Если сокет не создан
			if(socks[0] == -1)
				// Выходим из цикла
				break;
			// Выполняем подключение к сокету на чтение данных
			if(::connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
				// Выходим из цикла
				break;
			// Выполняем разрешение подключения к сокету и это у нас будет сокет на запись
			socks[1] = ::accept(listener, nullptr, nullptr);
			// Если сокет не создан
			if(socks[1] == -1)
				// Выходим из цикла
				break;
			// Закрываем сокет слушателя
			::closesocket(listener);
			// Выходим из функции и сообщаем, что все сокеты созданы удачно
			return 0;
		}
		// Получаем ошибки сгенерированные системой
		const int32_t error = WSAGetLastError();
		// Закрываем сокет слушателя
		::closesocket(listener);
		// Закрываем сокет чтение данных
		::closesocket(socks[0]);
		// Закрываем сокет для записи данных
		::closesocket(socks[1]);
		// Выполняем регистрацию ошибки
		WSASetLastError(error);
		// Выполняем сброс значения сокетов
		socks[0] = socks[1] = -1;
		// Выводим ошибку
		return SOCKET_ERROR;
	}
#endif

/**
 * create Метод создания файловых дескрипторов
 * @return файловые дескрипторы для обмена данными
 */
array <SOCKET, 2> awh::EventPIPE::create() noexcept {
	// Результат работы функции
	array <SOCKET, 2> result = {
		INVALID_SOCKET,
		INVALID_SOCKET
	};
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем инициализацию таймера
		if(::dumb_socketpair(result.data()) == INVALID_SOCKET){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Выполняем инициализацию таймера
		if(::socketpair(AF_UNIX, SOCK_DGRAM, 0, result.data()) == INVALID_SOCKET){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}	
	#endif
	// Делаем сокет неблокирующим
	this->_socket.blocking(result[0], socket_t::mode_t::DISABLED);
	// Делаем блокирующим сокет на запись
	this->_socket.blocking(result[1], socket_t::mode_t::ENABLED);
	// Устанавливаем размер буфера на чтение
	this->_socket.bufferSize(result[0], 32, socket_t::mode_t::READ);
	// Устанавливаем размер буфера на запись
	this->_socket.bufferSize(result[1], 32, socket_t::mode_t::WRITE);
	// Выводим результат
	return result;
}
/**
 * read Метод чтения из файлового дескриптора в буфер данных
 * @param fd        файловый дескриптор (сокет) для чтения
 * @param timestamp время прошедшее с момента запуска таймера
 * @return          размер прочитанных байт
 */
int8_t awh::EventPIPE::read(const SOCKET fd, uint64_t & timestamp) noexcept {
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем чтение из сокета данных
		return static_cast <int8_t> (::recv(fd, reinterpret_cast <char *> (&timestamp), sizeof(timestamp), 0));
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Выполняем чтение из сокета данных
		return static_cast <int8_t> (::read(fd, reinterpret_cast <char *> (&timestamp), sizeof(timestamp)));
	#endif
}
/**
 * send Метод отправки файловому дескриптору сообщения
 * @param fd        файловый дескриптор (сокет) для чтения
 * @param timestamp время прошедшее с момента запуска таймера
 * @return          размер записанных байт
 */
int8_t awh::EventPIPE::send(const SOCKET fd, const uint64_t timestamp) noexcept {
	/**
	 * Для операционной системы OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем запись в сокет данных
		return static_cast <int8_t> (::send(fd, reinterpret_cast <const char *> (&timestamp), sizeof(timestamp), 0));
	/**
	 * Для операционной системы не являющейся OS Windows
	 */
	#else
		// Выполняем запись в сокет данных
		return static_cast <int8_t> (::write(fd, reinterpret_cast <const char *> (&timestamp), sizeof(timestamp)));
	#endif
}
