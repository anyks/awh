/**
 * @file: timer.hpp
 * @date: 2024-06-30
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

#ifndef __AWH_TIMER__
#define __AWH_TIMER__

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)

/**
 * Если это Linux
 */
#elif __linux__
	/**
	 * Стандартные модули
	 */
	#include <sys/timerfd.h>
/**
 * Если это FreeBSD или MacOS X
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__
	/**
	 * Стандартные модули
	 */
	#include <ctime>
	#include <cstdio>
	#include <csignal>
	#include <dispatch/dispatch.h>
#endif

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <sys/events1.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timer Класс таймера ядра биндинга
	 */
	typedef class Timer2 : public awh::core_t {
		private:
			/**
			 * Broker Класс брокера
			 */
			typedef class Broker {
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Объект события таймера
					event1_t event;
				public:
					/**
					 * Broker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const fmk_t * fmk, const log_t * log) noexcept : persist(false), event(fmk, log) {}
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept {}
			} broker_t;
		private:
			// Мютекс для блокировки основного потока
			mutex _mtx;
		private:
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Объект очереди таймеров
			dispatch_queue_t _queue;
		private:
			// Список активных брокеров
			map <uint16_t, unique_ptr <broker_t>> _brokers;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * launching Метод вызова при активации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void launching(const bool mode, const bool status) noexcept;
			/**
			 * closedown Метод вызова при деакцтивации базы событий
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void closedown(const bool mode, const bool status) noexcept;
		private:
			/**
			 * event Метод события таймера
			 * @param tid  идентификатор таймера
			 * @param fd   файловый дескриптор таймера
			 * @param type тип отслеживаемого события
			 */
			void event(const uint16_t tid, const SOCKET fd, const base_t::event_type_t type) noexcept;
		public:
			/**
			 * clear Метод очистки всех таймеров
			 */
			void clear() noexcept;
			/**
			 * clear Метод очистки таймера
			 * @param tid идентификатор таймера для очистки
			 */
			void clear(const uint16_t tid) noexcept;
		public:
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * set Метод установки функции обратного вызова
			 * @param tid идентификатор таймера для установки
			 * @param fn  функция обратного вызова для установки
			 */
			void set(const uint16_t tid, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if((tid > 0) && (fn != nullptr)){
					// Выполняем блокировку потока
					const lock_guard <mutex> lock(this->_mtx);
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (static_cast <uint64_t> (tid), fn);
				}
			}
		public:
			/**
			 * timeout Метод создания таймаута
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t timeout(const time_t delay) noexcept;
			/**
			 * interval Метод создания интервала
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t interval(const time_t delay) noexcept;
		public:
			/**
			 * Timer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Timer2(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Timer Деструктор
			 */
			~Timer2() noexcept;
	} timer2_t;
};

#endif // __AWH_TIMER__
