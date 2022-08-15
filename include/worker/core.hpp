/**
 * @file: core.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_WORKER__
#define __AWH_WORKER__

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
	 * Worker Структура воркера
	 */
	typedef struct Worker {
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
		public:
			/**
			 * Callback Структура функций обратного вызова
			 */
			typedef struct Callback {
				// Функция обратного вызова при открытии приложения
				function <void (const size_t, Core *)> open;
				// Функция обратного вызова для персистентного вызова
				function <void (const size_t, const size_t, Core *)> persist;
				// Функция обратного вызова при запуске подключения
				function <void (const size_t, const size_t, Core *)> connect;
				// Функция обратного вызова при закрытии подключения
				function <void (const size_t, const size_t, Core *)> disconnect;
				// Функция обратного вызова при получении данных
				function <void (const char *, const size_t, const size_t, const size_t, Core *)> read;
				// Функция обратного вызова при записи данных
				function <void (const char *, const size_t, const size_t, const size_t, Core *)> write;
				// Функция обратного вызова при открытии подключения к прокси-серверу
				function <void (const size_t, const size_t, awh::Core *)> connectProxy;
				// Функция обратного вызова при получении данных с прокси-сервера
				function <void (const char *, const size_t, const size_t, const size_t, awh::Core *)> readProxy;
				// Функция обратного вызова при подключении нового клиента
				function <bool (const string &, const string &, const u_int, const size_t, awh::Core *)> accept;
				// Функция обратного вызова при получении сообщения от сервера
				function <void (const char *, const size_t, const size_t, const size_t, const pid_t, Core *)> mess;
				/**
				 * Callback Конструктор
				 */
				Callback() noexcept :
				 open(nullptr), persist(nullptr),
				 connect(nullptr), disconnect(nullptr),
				 read(nullptr), write(nullptr),
				 connectProxy(nullptr), readProxy(nullptr),
				 accept(nullptr), mess(nullptr) {}
			} fn_t;
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
					 * Worker Устанавливаем дружбу с родительским объектом
					 */
					friend class Worker;
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
					size_t aid;
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
					// Бинарный буфер для записи данных в сокет
					vector <char> buffer;
				public:
					// Создаём объект фреймворка
					const fmk_t * fmk;
					// Создаём объект работы с логами
					const log_t * log;
					// Объект родительского воркера
					const Worker * parent;
				private:
					/**
					 * read Функция обратного вызова при чтении данных с сокета
					 * @param watcher объект события чтения
					 * @param revents идентификатор события
					 */
					void read(ev::io & watcher, int revents) noexcept;
					/**
					 * write Функция обратного вызова при записи данных в сокет
					 * @param watcher объект события записи
					 * @param revents идентификатор события
					 */
					void write(ev::io & watcher, int revents) noexcept;
					/**
					 * connect Функция обратного вызова при подключении к серверу
					 * @param watcher объект события подключения
					 * @param revents идентификатор события
					 */
					void connect(ev::io & watcher, int revents) noexcept;
					/**
					 * timeout Функция обратного вызова при срабатывании таймаута
					 * @param timer   объект события таймаута
					 * @param revents идентификатор события
					 */
					void timeout(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Adjutant Конструктор
					 * @param parent объект родительского воркера
					 * @param fmk    объект фреймворка
					 * @param log    объект для работы с логами
					 */
					Adjutant(const Worker * parent, const fmk_t * fmk, const log_t * log) noexcept :
					 aid(0), ip(""), mac(""), port(0), ectx(fmk, log),
					 addr(fmk, log), fmk(fmk), log(log), parent(parent) {}
					/**
					 * ~Adjutant Деструктор
					 */
					~Adjutant() noexcept {}
			} adj_t;
		public:
			// Идентификатор воркера
			size_t wid;
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
			map <size_t, unique_ptr <adj_t>> adjutants;
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
			 * @param did    идентификатор DNS запроса
			 */
			void resolving(const string & ip, const int family, const size_t did) noexcept;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
		public:
			/**
			 * getPort Метод получения порта подключения адъютанта
			 * @param aid идентификатор адъютанта
			 * @return   порт подключения адъютанта
			 */
			u_int getPort(const size_t aid) const noexcept;
			/**
			 * getIp Метод получения IP адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес интернет подключения адъютанта
			 */
			const string & getIp(const size_t aid) const noexcept;
			/**
			 * getMac Метод получения MAC адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес устройства адъютанта
			 */
			const string & getMac(const size_t aid) const noexcept;
		public:
			/**
			 * Worker Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Worker(const fmk_t * fmk, const log_t * log) noexcept : wid(0), wait(false), alive(false), fmk(fmk), log(log), core(nullptr) {}
			/**
			 * ~Worker Деструктор
			 */
			virtual ~Worker() noexcept {}
	} worker_t;
};

#endif // __AWH_WORKER__
