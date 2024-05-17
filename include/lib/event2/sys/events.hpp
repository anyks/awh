/**
 * @file: events.hpp
 * @date: 2022-09-17
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

#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартные модули
 */
#include <mutex>
#include <ctime>
#include <string>
#include <functional>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>

/**
 * Наши модули
 */
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Event Структура события LibEvent2
	 */
	typedef class Event {
		public:
			/**
			 * type_t Тип события
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Событие не установлено
				TIMER  = 0x01, // Событие является таймером
				EVENT  = 0x02, // Событие является сокетом
				SIGNAL = 0x03  // Событие является форком процесса
			};
		private:
			// Режим активации события
			bool _mode;
		private:
			// Мютекс для блокировки основного потока
			mutex _mtx;
		private:
			// Тип события
			type_t _type;
		private:
			// Объект события
			struct event _ev;
			// Параметры интервала времени
			struct timeval _tv;
		private:
			// Задержка времени в миллисекундах
			time_t _delay;
		private:
			// Список ожидаемых событий
			int _events;
			// Файловый дескриптор
			evutil_socket_t _fd;
		private:
			// Функция обратного вызова
			function <void (const evutil_socket_t, const int)> _fn;
		private:
			// База данных событий
			struct event_base * _base;
		private:
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * callback Функция обратного вызова при срабатывании события
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void callback(evutil_socket_t fd, short event, void * ctx) noexcept;
		public:
			/**
			 * set Метод установки времени таймера
			 * @param delay задержка времени в миллисекундах
			 */
			void set(const time_t delay) noexcept;
			/**
			 * set Метод установки базы событий
			 * @param base база событий для установки
			 */
			void set(struct event_base * base) noexcept;
			/**
			 * set Метод установки файлового дескриптора и списка событий
			 * @param fd     файловый дескриптор для установки
			 * @param events список событий файлового дескриптора
			 */
			void set(const evutil_socket_t fd, const int events) noexcept;
			/**
			 * set Метод установки функции обратного вызова
			 * @param callback функция обратного вызова
			 */
			void set(function <void (const evutil_socket_t, const int)> callback) noexcept;
		public:
			/**
			 * stop Метод остановки работы события
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска работы события
			 * @param delay задержка времени в миллисекундах
			 */
			void start(const time_t delay = 0) noexcept;
		public:
			/**
			 * Event Конструктор
			 * @param type тип события
			 * @param log  объект для работы с логами
			 */
			Event(const type_t type, const log_t * log) noexcept :
			 _mode(false), _type(type), _ev({0}), _tv({0}), _delay(0),
			 _events(0), _fd(0), _fn(nullptr), _base(nullptr), _log(log) {}
			/**
			 * ~Event Деструктор
			 */
			~Event() noexcept;
	} event_t;
};

#endif // __AWH_EVENTS__
