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
 * Стандартная библиотека
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
	 * Core Прототип класса ядра биндинга TCP/IP
	 */
	class Core;
	/**
	 * Scheme Структура схемы сети
	 */
	typedef struct Scheme {
		private:
			/**
			 * Core Устанавливаем дружбу с классом ядра
			 */
			friend class Core;
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
			 * Timers Структура таймеров
			 */
			typedef struct Timers {
				event_t read;    // Событие таймера таймаута на чтение из сокета
				event_t write;   // Событие таймера таймаута на запись в сокет
				event_t connect; // Событие таймера таймаута на подключение к серверу
				/**
				 * Timers Конструктор
				 * @param log объект для работы с логами
				 */
				Timers(const log_t * log) noexcept :
				 read(event_t::type_t::TIMER, log),
				 write(event_t::type_t::TIMER, log),
				 connect(event_t::type_t::TIMER, log) {}
			} timers_t;
			/**
			 * Events Структура событий
			 */
			typedef struct Events {
				event_t read;    // Событие на чтение
				event_t write;   // Событие на запись
				event_t connect; // Событие на подключение
				/**
				 * Events Конструктор
				 * @param log объект для работы с логами
				 */
				Events(const log_t * log) noexcept :
				 read(event_t::type_t::EVENT, log),
				 write(event_t::type_t::EVENT, log),
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
				Timeouts() noexcept : read(READ_TIMEOUT), write(WRITE_TIMEOUT), connect(CONNECT_TIMEOUT) {}
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
				timers_t timers; // Собатия таймера на чтение/запись
				locked_t locked; // Блокиратор чтения/записи
				/**
				 * BufferEvent Конструктор
				 * @param log объект для работы с логами
				 */
				BufferEvent(const log_t * log) noexcept : events(log), timers(log) {}
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
				Marker() noexcept : read(BUFFER_READ_MIN, BUFFER_READ_MAX), write(BUFFER_WRITE_MIN, BUFFER_WRITE_MAX) {}
			} marker_t;
		private:
			/**
			 * Broker Структура брокера
			 */
			typedef struct Broker {
				private:
					/**
					 * Core Устанавливаем дружбу с классом ядра
					 */
					friend class Core;
					/**
					 * Scheme Устанавливаем дружбу с родительским объектом
					 */
					friend class Scheme;
					/**
					 * Client Core Устанавливаем дружбу с клиентским классом ядра
					 */
					friend class client::Core;
					/**
					 * Server Core Устанавливаем дружбу с серверным классом ядра
					 */
					friend class server::Core;
				private:
					// Идентификатор брокера
					uint64_t _bid;
				private:
					// Адрес интернет-подключения клиента
					string _ip;
					// Мак адрес подключившегося клиента
					string _mac;
					// Порт интернет-подключения клиента
					u_int _port;
				private:
					// Объект буфера событий
					bev_t _bev;
				private:
					// Маркер размера детектируемых байт
					marker_t _marker;
				private:
					// Объект таймаутов
					timeouts_t _timeouts;
				private:
					// Контекст двигателя для работы с передачей данных
					engine_t::ctx_t _ectx;
					// Создаём объект подключения клиента
					engine_t::addr_t _addr;
				public:
					// Создаём объект фреймворка
					const fmk_t * fmk;
					// Создаём объект работы с логами
					const log_t * log;
					// Объект родительской схемы
					const Scheme * parent;
				private:
					/**
					 * read Метод вызова при чтении данных с сокета
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void read(const evutil_socket_t fd, const short event) noexcept;
					/**
					 * connect Метод вызова при подключении к серверу
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void connect(const evutil_socket_t fd, const short event) noexcept;
					/**
					 * timeout Метод вызова при срабатывании таймаута
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void timeout(const evutil_socket_t fd, const short event) noexcept;
				public:
					/**
					 * Broker Конструктор
					 * @param parent объект родительской схемы сети
					 * @param fmk    объект фреймворка
					 * @param log    объект для работы с логами
					 */
					Broker(const Scheme * parent, const fmk_t * fmk, const log_t * log) noexcept :
					 _bid(0), _ip{""}, _mac{""}, _port(0), _bev(log), _ectx(fmk, log), _addr(fmk, log), fmk(fmk), log(log), parent(parent) {}
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept {}
			} broker_t;
		public:
			// Идентификатор родительской схемы
			uint16_t sid;
		public:
			// Флаг ожидания входящих сообщений
			bool wait;
			// Флаг автоматического поддержания подключения
			bool alive;
		public:
			// Хранилище функций обратного вызова
			fn_t callback;
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
			// Создаём объект фреймворка
			const Core * _core;
		private:
			/**
			 * resolving Метод получения IP адреса доменного имени
			 * @param ip     адрес интернет-подключения
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void resolving(const string & ip, const int family) noexcept;
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
			 * ip Метод получения IP адреса брокера
			 * @param bid идентификатор брокера
			 * @return    адрес интернет подключения брокера
			 */
			const string & ip(const uint64_t bid) const noexcept;
			/**
			 * mac Метод получения MAC адреса брокера
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
			 sid(0), wait(false), alive(false), callback(log), _fmk(fmk), _log(log), _core(nullptr) {}
			/**
			 * ~Scheme Деструктор
			 */
			virtual ~Scheme() noexcept {}
	} scheme_t;
};

#endif // __AWH_SCHEME__
