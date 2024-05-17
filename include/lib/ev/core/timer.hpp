/**
 * @file: timer.hpp
 * @date: 2024-03-10
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

#ifndef __AWH_CORE_TIMER__
#define __AWH_CORE_TIMER__

/**
 * Наши модули
 */
#include <core/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timer Класс таймера ядра биндинга
	 */
	typedef class Timer : public awh::core_t {
		private:
			/**
			 * Broker Прототип класса брокера подключения
			 */
			class Broker;
			/**
			 * Timeout Класс таймаута
			 */
			typedef class Timeout {
				private:
					// Задержка времени в секундах
					float _delay;
				private:
					// Идентификатор таймера
					uint16_t _tid;
				private:
					// Объект брокера подключения
					Broker * _broker;
				public:
					/**
					 * operator Оператор [()] Получения события таймера
					 * @param timer   объект события таймаута
					 * @param revents идентификатор события
					 */
					void operator()(ev::timer & timer, int revents) noexcept;
				public:
					/**
					 * Timeout Конструктор
					 * @param tid    идентификатор таймера
					 * @param delay  задержка времени в секундах
					 * @param broker брокер подключения
					 */
					Timeout(const uint16_t tid, const float delay, Broker * broker) noexcept :
					 _delay(delay), _tid(tid), _broker(broker) {}
			} timeout_t;
			/**
			 * Broker Класс брокера
			 */
			typedef class Broker {
				public:
					// Флаг персистентной работы
					bool persist;
				public:
					// Флаг запущенного брокера
					bool launched;
				public:
					// Объект события таймера
					ev::timer io;
				public:
					// Объект таймаута
					timeout_t timeout;
				public:
					// Функция обратного вызова
					function <void (const uint16_t, const float)> fn;
				public:
					/**
					 * Broker Конструктор
					 * @param tid   идентификатор таймера
					 * @param delay задержка времени в секундах
					 */
					Broker(const uint16_t tid, const float delay) noexcept :
					 persist(false), launched(false), timeout(tid, delay, this), fn(nullptr) {}
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
			// Список активных брокеров
			map <uint16_t, unique_ptr <broker_t>> _brokers;
		private:
			/**
			 * event Метод события таймера
			 * @param tid   идентификатор таймера
			 * @param delay задержка времени в секундах
			 */
			void event(const uint16_t tid, const float delay) noexcept;
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
			Timer(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), _callbacks(log) {}
			/**
			 * ~Timer Деструктор
			 */
			~Timer() noexcept;
	} timer_t;
};

#endif // __AWH_CORE_TIMER__
