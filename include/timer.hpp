/**
 * @file: timer.hpp
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

#ifndef __AWH_TIMER__
#define __AWH_TIMER__

#include <thread>
#include <chrono>
#include <future>

// Устанавливаем пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timer Класс таймера
	 */
	typedef class Timer {
		private:
			// Токен работы таймера
			atomic_bool token;
		public:
			/**
			 * Шаблон функции
			 */
			template <typename Function>
			/**
			 * setTimeout Метод установки таймаута
			 * @param function функция для вызова
			 * @param delay    интервал задержки времени
			 */
			void setTimeout(Function function, size_t delay) noexcept {
				// Создаём новый поток
				thread t([=]{
					// Устанавливаем флаг разрешающий работу
					this->token.store(true);
					// Создаём бесконечный цикл
					while(this->token.load()){
						// Усыпляем поток на указанное количество милисекунд
						this_thread::sleep_for(chrono::milliseconds(delay));
						// Выполняем функцию обратного вызова
						if(this->token.load()) function();
						// Останавливаем работу таймера
						this->token.store(false);
					}
				});
				// Выполняем ожидание завершения работы потока
				t.detach();
			}
			/**
			 * Шаблон функции
			 */
			template <typename Function>
			/**
			 * setInterval Метод установки таймаута
			 * @param function функция для вызова
			 * @param interval интервал времени выполнения функции
			 */
			void setInterval(Function function, size_t interval) noexcept {
				// Устанавливаем флаг разрешающий работу
				this->token.store(true);
				// Создаём новый поток
				thread t([=]{
					// Создаём бесконечный цикл
					while(this->token.load()){
						// Усыпляем поток на указанное количество милисекунд
						this_thread::sleep_for(chrono::milliseconds(interval));
						// Выполняем функцию обратного вызова
						if(this->token.load()) function();
					}
				});
				// Выполняем ожидание завершения работы потока
				t.detach();
			}
			/**
			 * stop Метод остановки работы таймера
			 */
			void stop() noexcept {
				// Останавливаем работу таймера
				this->token.store(false);
			}
			/**
			 * ~Timer Деструктор
			 */
			~Timer() noexcept {
				// Останавливаем работу таймера
				this->stop();
			}
	} timer_t;
};

#endif // __AWH_TIMER__
