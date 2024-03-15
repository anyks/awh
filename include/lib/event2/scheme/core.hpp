/**
 * @file: core.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_SCHEME__
#define __AWH_SCHEME__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <string>
#include <event2/event.h>
#include <event2/event_struct.h>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/engine.hpp>
#include <lib/event2/sys/events.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
}

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Scheme Структура схемы сети
	 */
	typedef struct Scheme {
		private:
			/**
			 * Node Core Устанавливаем дружбу с нодой сетевого ядра
			 */
			friend class Node;
			/**
			 * Client Core Устанавливаем дружбу с клиентским классом ядра
			 */
			friend class client::Core;
			/**
			 * Server Core Устанавливаем дружбу с серверным классом ядра
			 */
			friend class server::Core;
		public:
			/**
			 * Флаги активации/деактивации
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x01, // Включено
				DISABLED = 0x00  // Отключено
			};
			/**
			 * Семейство протоколов интернета
			 */
			enum class family_t : uint8_t {
				IPV4 = 0x01, // Протокол IPv4
				IPV6 = 0x02, // Протокол IPv6
				NIX  = 0x03  // Протокол unix-сокет
			};
			/**
			 * Тип сокета подключения
			 */
			enum class sonet_t : uint8_t {
				TCP  = 0x01, // Нешифрованное подключение TCP
				UDP  = 0x02, // Нешифрованное подключение UDP
				TLS  = 0x03, // Шифрованное подключение TCP
				DTLS = 0x04, // Шифрованное подключение UDP
				SCTP = 0x05  // Шифрованное подключение SCTP
			};
		public:
			/**
			 * Events Структура событий
			 */
			typedef struct Events {
				event_t read;    // Событие на чтение
				event_t write;   // Событие на запись
				event_t accept;  // Событие на запросы подключений
				event_t connect; // Событие на подключение
				/**
				 * Events Конструктор
				 * @param log объект для работы с логами
				 */
				Events(const log_t * log) noexcept :
				 read(event_t::type_t::EVENT, log),
				 write(event_t::type_t::EVENT, log),
				 accept(event_t::type_t::EVENT, log),
				 connect(event_t::type_t::EVENT, log) {}
			} events_t;
			/**
			 * Timeouts Структура таймаутов
			 */
			typedef struct Timeouts {
				time_t read;    // Таймаут на чтение в секундах
				time_t write;   // Таймаут на запись в секундах
				time_t connect; // Таймаут на подключение в секундах
				/**
				 * Timeouts Конструктор
				 */
				Timeouts() noexcept :
				 read(READ_TIMEOUT),
				 write(WRITE_TIMEOUT),
				 connect(CONNECT_TIMEOUT) {}
			} timeouts_t;
			/**
			 * Locked Структура блокировки на чтение и запись данных в буфере
			 */
			typedef struct Locked {
				bool read;  // Флаг блокировки чтения
				bool write; // Флаг блокировки записи
				/**
				 * Locked Конструктор
				 */
				Locked() noexcept : read(true), write(true) {}
			} locked_t;
			/**
			 * BufferEvent Структура буфера событий
			 */
			typedef struct BufferEvent {
				events_t events; // События чтения/записи
				locked_t locked; // Блокиратор чтения/записи
				/**
				 * BufferEvent Конструктор
				 * @param log объект для работы с логами
				 */
				BufferEvent(const log_t * log) noexcept : events(log) {}
			} bev_t;
			/**
			 * Mark Структура маркера на размер детектируемых байт
			 */
			typedef struct Mark {
				size_t min; // Минимальный размер детектируемых байт
				size_t max; // Максимальный размер детектируемых байт
				/**
				 * Mark Конструктор
				 * @param min минимальный размер детектируемых байт
				 * @param max максимальный размер детектируемых байт
				 */
				Mark(const size_t min = 0, const size_t max = 0) : min(min), max(max) {}
			} __attribute__((packed)) mark_t;
			/**
			 * Marker Структура маркеров
			 */
			typedef struct Marker {
				mark_t read;  // Маркера размера детектируемых байт на чтение
				mark_t write; // Маркера размера детектируемых байт на запись
				/**
				 * Marker Конструктор
				 */
				Marker() noexcept :
				 read(BUFFER_READ_MIN, BUFFER_READ_MAX),
				 write(BUFFER_WRITE_MIN, BUFFER_WRITE_MAX) {}
			} marker_t;
		private:
			/**
			 * Broker Класс брокера подключения
			 */
			typedef class Broker {
				private:
					// Мютекс для блокировки потока
					mutex _mtx;
				private:
					// Идентификатор брокера
					uint64_t _id;
				private:
					// Идентификатор схемы сети
					uint16_t _sid;
				private:
					// Адрес интернет-подключения клиента
					string _ip;
					// Мак адрес подключившегося клиента
					string _mac;
					// Порт интернет-подключения клиента
					u_int _port;
				public:
					// Объект буфера событий
					bev_t _bev;
				private:
					// Тип сокета подключения
					sonet_t _sonet;
				private:
					// Хранилище функций обратного вызова
					fn_t _callbacks;
				public:
					// Маркер размера детектируемых байт
					marker_t _marker;
				public:
					// Объект таймаутов
					timeouts_t _timeouts;
				public:
					// Контекст двигателя для работы с передачей данных
					engine_t::ctx_t _ectx;
					// Создаём объект подключения клиента
					engine_t::addr_t _addr;
				private:
					// Создаём объект фреймворка
					const fmk_t * _fmk;
					// Создаём объект работы с логами
					const log_t * _log;
				private:
					// База данных событий
					struct event_base * _base;
				public:
					/**
					 * id Метод извлечения идентификатора брокера
					 * @return идентификатор брокера
					 */
					uint64_t id() const noexcept;
				public:
					/**
					 * sid Метод извлечения идентификатора схемы сети
					 * @return идентификатор схемы сети
					 */
					uint16_t sid() const noexcept;
				public:
					/**
					 * port Метод извлечения порта подключения
					 * @return установленный порт подключения
					 */
					u_int port() const noexcept;
					/**
					 * port Метод установки порта подключения
					 * @param порт подключения для установки
					 */
					void port(const u_int port) noexcept;
				public:
					/**
					 * ip Метод извлечения IP-адреса
					 * @return установленный IP-адрес
					 */
					const string & ip() const noexcept;
					/**
					 * ip Метод установки IP-адреса
					 * @param ip адрес для установки
					 */
					void ip(const string & ip) noexcept;
				public:
					/**
					 * mac Метод извлечения MAC-адреса
					 * @return установленный MAC-адрес
					 */
					const string & mac() const noexcept;
					/**
					 * mac Метод установки MAC-адреса
					 * @param mac адрес для установки
					 */
					void mac(const string & mac) noexcept;
				public:
					/**
					 * callbacks Метод установки функций обратного вызова
					 * @param callbacks функции обратного вызова
					 */
					void callbacks(const fn_t & callbacks) noexcept;
				public:
					/**
					 * callback Шаблон метода установки финкции обратного вызова
					 * @tparam A тип функции обратного вызова
					 */
					template <typename A>
					/**
					 * callback Метод установки функции обратного вызова
					 * @param idw идентификатор функции обратного вызова
					 * @param fn  функция обратного вызова для установки
					 */
					void callback(const uint64_t idw, function <A> fn) noexcept {
						// Если функция обратного вызова передана
						if((idw > 0) && (fn != nullptr)){
							// Выполняем блокировку потока
							const lock_guard <mutex> lock(this->_mtx);
							// Выполняем установку функции обратного вызова
							this->_callbacks.set <A> (idw, fn);
						}
					}
					/**
					 * callback Шаблон метода установки финкции обратного вызова
					 * @tparam A тип функции обратного вызова
					 */
					template <typename A>
					/**
					 * callback Метод установки функции обратного вызова
					 * @param name название функции обратного вызова
					 * @param fn   функция обратного вызова для установки
					 */
					void callback(const string & name, function <A> fn) noexcept {
						// Если функция обратного вызова передана
						if(!name.empty() && (fn != nullptr)){
							// Выполняем блокировку потока
							const lock_guard <mutex> lock(this->_mtx);
							// Выполняем установку функции обратного вызова
							this->_callbacks.set <A> (name, fn);
						}
					}
				private:
					/**
					 * sonet Метод установки типа сокета подключения
					 * @param sonet тип сокета подключения
					 */
					void sonet(const sonet_t sonet) noexcept;
				private:
					/**
					 * read Метод вызова при чтении данных с сокета
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void read(const evutil_socket_t fd, const short event) noexcept;
					/**
					 * write Метод вызова при активации сокета на запись данных
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void write(const evutil_socket_t fd, const short event) noexcept;
					/**
					 * accept Метод вызова при подключении к серверу
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void accept(const evutil_socket_t fd, const short event) noexcept;
					/**
					 * connect Метод вызова при подключении к серверу
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void connect(const evutil_socket_t fd, const short event) noexcept;
				public:
					/**
					 * lockup Метод блокировки метода режима работы
					 * @param mode   флаг блокировки метода
					 * @param method метод режима работы
					 */
					void lockup(const mode_t mode, const engine_t::method_t method) noexcept;
					/**
					 * events Метод активации/деактивации метода события сокета
					 * @param mode   сигнал активации сокета
					 * @param method метод режима работы
					 */
					void events(const mode_t mode, const engine_t::method_t method) noexcept;
					/**
					 * timeout Метод установки таймаута ожидания появления данных
					 * @param seconds время ожидания в секундах
					 * @param method  метод режима работы
					 */
					void timeout(const time_t seconds, const engine_t::method_t method) noexcept;
					/**
					 * marker Метод установки маркера на размер детектируемых байт
					 * @param min    минимальный размер детектируемых байт
					 * @param min    максимальный размер детектируемых байт
					 * @param method метод режима работы
					 */
					void marker(const size_t min, const size_t max, const engine_t::method_t method) noexcept;
				public:
					/**
					 * base Метод установки базы событий
					 * @param base база событий для установки
					 */
					void base(struct event_base * base) noexcept;
				public:
					/**
					 * Оператор [=] установки базы событий
					 * @param base база событий для установки
					 * @return     текущий объект
					 */
					Broker & operator = (struct event_base * base) noexcept;
				public:
					/**
					 * Broker Конструктор
					 * @param sid идентификатор схемы сети
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const uint16_t sid, const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept {}
			} broker_t;
		public:
			// Идентификатор родительской схемы
			uint16_t id;
		public:
			// Флаг ожидания входящих сообщений
			bool wait;
			// Флаг автоматического поддержания подключения
			bool alive;
		public:
			// Маркер размера детектируемых байт
			marker_t marker;
		public:
			// Объект таймаутов
			timeouts_t timeouts;
		public:
			// Время жизни подключения
			engine_t::alive_t keepAlive;
		protected:
			// Список подключённых брокеров
			map <uint64_t, unique_ptr <broker_t>> _brokers;
		protected:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
		public:
			/**
			 * socket Метод извлечения сокета брокера
			 * @param bid идентификатор брокера
			 * @return    активный сокет брокера
			 */
			SOCKET socket(const uint64_t bid) const noexcept;
		public:
			/**
			 * port Метод получения порта подключения брокера
			 * @param bid идентификатор брокера
			 * @return   порт подключения брокера
			 */
			u_int port(const uint64_t bid) const noexcept;
			/**
			 * ip Метод получения IP-адреса брокера
			 * @param bid идентификатор брокера
			 * @return    адрес интернет подключения брокера
			 */
			const string & ip(const uint64_t bid) const noexcept;
			/**
			 * mac Метод получения MAC-адреса брокера
			 * @param bid идентификатор брокера
			 * @return    адрес устройства брокера
			 */
			const string & mac(const uint64_t bid) const noexcept;
		public:
			/**
			 * Scheme Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Scheme(const fmk_t * fmk, const log_t * log) noexcept :
			 id(0), wait(false), alive(false), _fmk(fmk), _log(log) {}
			/**
			 * ~Scheme Деструктор
			 */
			virtual ~Scheme() noexcept {}
	} scheme_t;
};

#endif // __AWH_SCHEME__
