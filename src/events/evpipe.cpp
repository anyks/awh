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
 * port Метод получения активного порта
 * @return номер порта сервера
 */
uint32_t awh::EventPipe::port() const noexcept {
	// Выводим номер активного порта сервера
	return this->_port;
}
/**
 * type Метод установки типа пайпа
 * @param type тип пайпа для установки
 */
void awh::EventPipe::type(const type_t type) noexcept {
	// Выполняем установку типа пайпа
	this->_type = type;
}
/**
 * create Метод создания файловых дескрипторов
 * @return файловые дескрипторы для обмена данными
 */
array <SOCKET, 2> awh::EventPipe::create() noexcept {
	// Результат работы функции
	array <SOCKET, 2> result = {
		INVALID_SOCKET,
		INVALID_SOCKET
	};
	// Определяем тип пайпа
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип пайпа соответствует нативному
		case static_cast <uint8_t> (type_t::NATIVE): {
			/**
			 * Для операционной системы OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Создаём объект файловых дескрипторов
				int32_t fds[2];
				// Выполняем инициализацию таймера
				if(::_pipe(fds, sizeof(uint64_t), O_BINARY) == INVALID_SOCKET){
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
				// Выполняем установку сокета на чтение
				result[0] = static_cast <SOCKET> (fds[0]);
				// Выполняем установку сокета на запись
				result[1] = static_cast <SOCKET> (fds[1]);
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
		} break;
		// Если тип пайпа соответствует UDP-серверу
		case static_cast <uint8_t> (type_t::NETWORK): {
			/**
			 * Выполняем генерацию порта
			 */
			do {
				// Подключаем устройство генератора
				mt19937 generator(this->_randev());
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0xC000, 0xFFFF);
				// Выполняем получение порта
				this->_port = dist6(generator);
			// Если такой порт уже был ранее сгенерирован, пробуем ещё раз
			} while(!this->_socket.isBind(AF_INET, SOCK_DGRAM, this->_port));
			// Если сокет не создан выводим сообщение об ошибке
			if((result[0] = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET){
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
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(result[0]);
			// Переводим сокет в не блокирующий режим
			this->_socket.blocking(result[0], socket_t::mode_t::DISABLED);
			// Зануляем объект сервера
			::memset(&this->_peer.server, 0, sizeof(this->_peer.server));
			// Устанавливаем протокол интернета
			this->_peer.server.sin_family = AF_INET;
			// Устанавливаем порт для локального подключения
			this->_peer.server.sin_port = htons(this->_port);
			// Устанавливаем адрес для удаленного подключения
			this->_peer.server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			// Выполняем бинд на сокет
			if(::bind(result[0], reinterpret_cast <struct sockaddr *> (&this->_peer.server), sizeof(this->_peer.server)) == INVALID_SOCKET){
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
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * read Метод чтения из файлового дескриптора в буфер данных
 * @param fd     файловый дескриптор (сокет) для чтения
 * @param buffer бинарный буфер данных куда производится чтение
 * @param size   размер бинарного буфера для чтения
 * @return       размер прочитанных байт
 */
int64_t awh::EventPipe::read(const SOCKET fd, void * buffer, const size_t size) noexcept {
	// Если буфер данных не нулевой
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип пайпа
		switch(static_cast <uint8_t> (this->_type)){
			// Если тип пайпа соответствует нативному
			case static_cast <uint8_t> (type_t::NATIVE): {
				/**
				 * Для операционной системы OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем чтение из сокета данных
					return static_cast <int64_t> (::_read(fd, buffer, size));
				/**
				 * Для операционной системы не являющейся OS Windows
				 */
				#else
					// Выполняем чтение из сокета данных
					return static_cast <int64_t> (::read(fd, buffer, size));
				#endif
			}
			// Если тип пайпа соответствует UDP-серверу
			case static_cast <uint8_t> (type_t::NETWORK): {
				// Получаем размер структуры клиента
				socklen_t length = sizeof(struct sockaddr);
				// Очищаем всю структуру для клиента
				::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
				// Выполняем чтение из сокета данных
				return static_cast <int64_t> (::recvfrom(fd, reinterpret_cast <char *> (buffer), size, 0, reinterpret_cast <struct sockaddr *> (&this->_peer.client), &length));
			}
		}
	}
	// Выводим результат
	return INVALID_SOCKET;
}
/**
 * write Метод записи в файловый дескриптор из буфера данных
 * @param fd     файловый дескриптор (сокет) для чтения
 * @param buffer бинарный буфер данных откуда производится запись
 * @param size   размер бинарного буфера для записи
 * @param port   порт сервера на который нужно отправить ответ
 * @return       размер записанных байт
 */
int64_t awh::EventPipe::write(const SOCKET fd, const void * buffer, const size_t size, const uint32_t port) noexcept {
	// Если буфер данных передан не пустой
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип пайпа
		switch(static_cast <uint8_t> (this->_type)){
			// Если тип пайпа соответствует нативному
			case static_cast <uint8_t> (type_t::NATIVE): {
				/**
				 * Для операционной системы OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем запись в сокет данных
					return static_cast <int64_t> (::_write(fd, buffer, size));
				/**
				 * Для операционной системы не являющейся OS Windows
				 */
				#else
					// Выполняем запись в сокет данных
					return static_cast <int64_t> (::write(fd, buffer, size));
				#endif
			}
			// Если тип пайпа соответствует UDP-серверу
			case static_cast <uint8_t> (type_t::NETWORK): {
				// Получаем размер структуры сервера
				const socklen_t length = sizeof(struct sockaddr);
				// Очищаем всю структуру для сервера
				::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
				// Устанавливаем протокол интернета
				this->_peer.client.sin_family = AF_INET;
				// Устанавливаем порт для локального подключения
				this->_peer.client.sin_port = htons(port);
				// Устанавливаем адрес для удаленного подключения
				this->_peer.client.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				// Выполняем отправку буфера данных клиенту
				return static_cast <int64_t> (::sendto(fd, reinterpret_cast <const char *> (buffer), size, 0, reinterpret_cast <struct sockaddr *> (&this->_peer.client), length));
			}
		}
	}
	// Выводим результат
	return INVALID_SOCKET;
}
