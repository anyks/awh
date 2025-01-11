/**
 * @file: pipe.hpp
 * @date: 2024-07-03
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

#ifndef __AWH_PIPE__
#define __AWH_PIPE__

/**
 * Стандартные модули
 */
#include <ctime>
#include <array>
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <algorithm>

/**
 * Наши модули
 */
#include <net/socket.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * PIPE Класс передачи данных между процессами
	 */
	typedef class AWHSHARED_EXPORT PIPE {
		public:
			/**
			 * Тип пайпа с которым производится работа
			 */
			enum class type_t : uint8_t {
				NONE    = 0x00, // Тип пайпа не установлен
				NATIVE  = 0x01, // Тип пайпа нативный
				NETWORK = 0x02  // Тип пайпа сетевой
			};
		private:
			/**
			 * Peer Структура подключения
			 */
			typedef struct Peer {
				struct sockaddr_in client; // Параметры подключения клиента
				struct sockaddr_in server; // Параметры подключения сервера
				/**
				 * Peer Конструктор
				 */
				Peer() noexcept : client{}, server{} {}
			} peer_t;
		private:
			// Объект пира
			peer_t _peer;
		private:
			// Тип пайпа
			type_t _type;
		private:
			// Порт прослушивающего сервера
			uint32_t _port;
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Выполняем инициализацию генератора
			random_device _randev;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * port Метод получения активного порта
			 * @return номер порта сервера
			 */
			uint32_t port() const noexcept;
		public:
			/**
			 * type Метод установки типа пайпа
			 * @param type тип пайпа для установки
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * create Метод создания файловых дескрипторов
			 * @return файловые дескрипторы для обмена данными
			 */
			array <SOCKET, 2> create() noexcept;
		public:
			/**
			 * read Метод чтения из файлового дескриптора в буфер данных
			 * @param fd     файловый дескриптор (сокет) для чтения
			 * @param buffer бинарный буфер данных куда производится чтение
			 * @param size   размер бинарного буфера для чтения
			 * @return       размер прочитанных байт
			 */
			int64_t read(const SOCKET fd, void * buffer, const size_t size) noexcept;
			/**
			 * write Метод записи в файловый дескриптор из буфера данных
			 * @param fd     файловый дескриптор (сокет) для чтения
			 * @param buffer бинарный буфер данных откуда производится запись
			 * @param size   размер бинарного буфера для записи
			 * @param port   порт сервера на который нужно отправить ответ
			 * @return       размер записанных байт
			 */
			int64_t write(const SOCKET fd, const void * buffer, const size_t size, const uint32_t port = 0) noexcept;
		public:
			/**
			 * PIPE Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			PIPE(const fmk_t * fmk, const log_t * log) noexcept :
			 _type(type_t::NONE), _port(0), _socket(fmk, log), _fmk(fmk), _log(log) {}
			/**
			 * ~PIPE Деструктор
			 */
			~PIPE() noexcept {}
	} pipe_t;
};

#endif // __AWH_PIPE__
