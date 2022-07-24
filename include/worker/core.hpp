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
#include <net/ssl.hpp>

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
			// Core Устанавливаем дружбу с классом ядра
			friend class Core;
			// Client Core Устанавливаем дружбу с клиентским классом ядра
			friend class client::Core;
			// Server Core Устанавливаем дружбу с серверным классом ядра
			friend class server::Core;
		public:
			/**
			 * Timer Структура таймаута
			 */
			typedef struct Timer {
				ev::timer read;  // Событие таймера таймаута на чтение из сокета
				ev::timer write; // Событие таймера таймаута на запись в сокет
			} timer_t;
			/**
			 * Event Структура события
			 */
			typedef struct Event {
				ev::io read;  // Событие на чтение
				ev::io write; // Событие на запись
			} event_t;
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
				int socket;      // Активный сокет
				event_t event;   // Событие чтения/записи
				timer_t timer;   // Собатие таймера на чтение/запись
				locked_t locked; // Блокиратор чтения/записи
				/**
				 * BufferEvent Конструктор
				 */
				BufferEvent() noexcept : socket(-1) {}
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
		public:
			/**
			 * Adjutant Структура адъютанта
			 */
			typedef struct Adjutant {
				private:
					// Core Устанавливаем дружбу с классом ядра
					friend class Core;
					// Worker Устанавливаем дружбу с родительским объектом
					friend class Worker;
					// Client Core Устанавливаем дружбу с клиентским классом ядра
					friend class client::Core;
					// Server Core Устанавливаем дружбу с серверным классом ядра
					friend class server::Core;
				private:
					// Идентификатор адъютанта
					size_t aid;
				private:
					// Адрес интернет подключения клиента
					string ip;
					// Мак адрес подключившегося клиента
					string mac;
				private:
					// Объект буфера событий
					bev_t bev;
				private:
					// Маркера размера детектируемых байт на чтение
					mark_t markRead;
					// Маркера размера детектируемых байт на запись
					mark_t markWrite;
				private:
					// Таймер на чтение в секундах
					time_t timeRead;
					// Таймер на запись в секундах
					time_t timeWrite;
				private:
					// Контекст SSL для работы с защищённым подключением
					ssl_t::ctx_t ssl;
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
					 aid(0), ip(""), mac(""),
					 markRead(BUFFER_READ_MIN, BUFFER_READ_MAX),
					 markWrite(BUFFER_WRITE_MIN, BUFFER_WRITE_MAX),
					 timeRead(READ_TIMEOUT), timeWrite(WRITE_TIMEOUT),
					 parent(parent), fmk(fmk), log(log) {}
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
			// Маркера размера детектируемых байт на чтение
			mark_t markRead;
			// Маркера размера детектируемых байт на запись
			mark_t markWrite;
		public:
			// Таймер на чтение в секундах
			time_t timeRead;
			// Таймер на запись в секундах
			time_t timeWrite;
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
		public:
			// Функция обратного вызова при открытии приложения
			function <void (const size_t, Core *)> openFn;
			// Функция обратного вызова для персистентного вызова
			function <void (const size_t, const size_t, Core *)> persistFn;
			// Функция обратного вызова при запуске подключения
			function <void (const size_t, const size_t, Core *)> connectFn;
			// Функция обратного вызова при закрытии подключения
			function <void (const size_t, const size_t, Core *)> disconnectFn;
			// Функция обратного вызова при получении данных
			function <void (const char *, const size_t, const size_t, const size_t, Core *)> readFn;
			// Функция обратного вызова при записи данных
			function <void (const char *, const size_t, const size_t, const size_t, Core *)> writeFn;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
		public:
			/**
			 * ip Метод получения IP адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес интернет подключения адъютанта
			 */
			const string & ip(const size_t aid) const noexcept;
			/**
			 * mac Метод получения MAC адреса адъютанта
			 * @param aid идентификатор адъютанта
			 * @return    адрес устройства адъютанта
			 */
			const string & mac(const size_t aid) const noexcept;
		public:
			/**
			 * Worker Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Worker(const fmk_t * fmk, const log_t * log) noexcept :
			 wid(0), wait(false), alive(false),
			 markRead(BUFFER_READ_MIN, BUFFER_READ_MAX),
			 markWrite(BUFFER_WRITE_MIN, BUFFER_WRITE_MAX),
			 timeRead(READ_TIMEOUT), timeWrite(WRITE_TIMEOUT),
			 fmk(fmk), log(log), core(nullptr),
			 openFn(nullptr), persistFn(nullptr), connectFn(nullptr),
			 disconnectFn(nullptr), readFn(nullptr), writeFn(nullptr) {}
			/**
			 * ~Worker Деструктор
			 */
			virtual ~Worker() noexcept {}
	} worker_t;
};

#endif // __AWH_WORKER__
