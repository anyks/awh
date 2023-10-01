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
#include <libev/ev++.h>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/engine.hpp>

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
			 * Timer Структура таймаута
			 */
			typedef struct Timer {
				ev::timer read;    // Событие таймера таймаута на чтение из сокета
				ev::timer write;   // Событие таймера таймаута на запись в сокет
				ev::timer connect; // Событие таймера таймаута на подключение к серверу
			} timer_t;
			/**
			 * Event Структура события
			 */
			typedef struct Event {
				ev::io read;    // Событие на чтение
				ev::io write;   // Событие на запись
				ev::io connect; // Событие на подключение
			} event_t;
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
				event_t event;   // Событие чтения/записи
				timer_t timer;   // Собатие таймера на чтение/запись
				locked_t locked; // Блокиратор чтения/записи
				/**
				 * BufferEvent Конструктор
				 */
				BufferEvent() noexcept {}
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
			 * Adjutant Структура адъютанта
			 */
			typedef struct Adjutant {
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
					// Идентификатор адъютанта
					uint64_t aid;
				private:
					// Адрес интернет-подключения клиента
					string ip;
					// Мак адрес подключившегося клиента
					string mac;
					// Порт интернет-подключения клиента
					u_int port;
				private:
					// Объект буфера событий
					bev_t bev;
				private:
					// Маркер размера детектируемых байт
					marker_t marker;
				private:
					// Объект таймаутов
					timeouts_t timeouts;
				private:
					// Контекст двигателя для работы с передачей данных
					engine_t::ctx_t ectx;
					// Создаём объект подключения клиента
					engine_t::addr_t addr;
				private:
					// Метод выполняемой операции
					engine_t::method_t method;
				private:
					// Бинарный буфер для записи данных в сокет
					vector <char> buffer;
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
					 * @param watcher объект события чтения
					 * @param revents идентификатор события
					 */
					void read(ev::io & watcher, int revents) noexcept;
					/**
					 * write Метод вызова при записи данных в сокет
					 * @param watcher объект события записи
					 * @param revents идентификатор события
					 */
					void write(ev::io & watcher, int revents) noexcept;
					/**
					 * connect Метод вызова при подключении к серверу
					 * @param watcher объект события подключения
					 * @param revents идентификатор события
					 */
					void connect(ev::io & watcher, int revents) noexcept;
					/**
					 * timeout Метод вызова при срабатывании таймаута
					 * @param timer   объект события таймаута
					 * @param revents идентификатор события
					 */
					void timeout(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Adjutant Конструктор
					 * @param parent объект родительской схемы сети
					 * @param fmk    объект фреймворка
					 * @param log    объект для работы с логами
					 */
					Adjutant(const Scheme * parent, const fmk_t * fmk, const log_t * log) noexcept :
					 aid(0), ip(""), mac(""), port(0), ectx(fmk, log), addr(fmk, log),
					 method(engine_t::method_t::DISCONNECT), fmk(fmk), log(log), parent(parent) {}
					/**
					 * ~Adjutant Деструктор
					 */
					~Adjutant() noexcept {}
			} adj_t;
		public:
			// Идентификатор родительской схемы
			uint16_t sid;
		public:
			// Флаг ожидания входящих сообщений
			bool wait;
			// Флаг автоматического поддержания подключения
			bool alive;
		public:
			// Объявляем функции обратного вызова
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
			// Список подключённых адъютантов
			map <uint64_t, unique_ptr <adj_t>> adjutants;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk;
			// Создаём объект работы с логами
			const log_t * log;
			// Создаём объект фреймворка
			const Core * core;
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
			 * getSocket Метод извлечения сокета адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    активный сокет адъютанта
			 */
			SOCKET getSocket(const uint64_t aid) const noexcept;
		public:
			/**
			 * getPort Метод получения порта подключения адъютанта
			 * @param aid идентификатор адъютанта
			 * @return   порт подключения адъютанта
			 */
			u_int getPort(const uint64_t aid) const noexcept;
			/**
			 * getIp Метод получения IP адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес интернет подключения адъютанта
			 */
			const string & getIp(const uint64_t aid) const noexcept;
			/**
			 * getMac Метод получения MAC адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес устройства адъютанта
			 */
			const string & getMac(const uint64_t aid) const noexcept;
		public:
			/**
			 * Scheme Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Scheme(const fmk_t * fmk, const log_t * log) noexcept : sid(0), wait(false), alive(false), callback(log), fmk(fmk), log(log), core(nullptr) {}
			/**
			 * ~Scheme Деструктор
			 */
			virtual ~Scheme() noexcept {}
	} scheme_t;
};

#endif // __AWH_SCHEME__
