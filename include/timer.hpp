/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_TIMER__
#define __AWH_TIMER__

#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>

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
			// Мютекс для блокировки потока
			mutex mtx;
			// Переменная ожидания
			condition_variable cnd;
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
					// Создаем уникальный локер потока
					unique_lock <mutex> lck(this->mtx);
					// Усыпляем поток на указанное количество милисекунд
					auto b = this->cnd.wait_for(lck, chrono::milliseconds(delay));
					// Если время вышло, выполняем функцию обратного вызова
					if(b == cv_status::timeout) function();
					// Иначе выходим из функции
					else return;
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
				// Создаём новый поток
				thread t([=]{
					// Создаём бесконечный цикл
					while(true){
						// Создаем уникальный локер потока
						unique_lock <mutex> lck(this->mtx);
						// Усыпляем поток на указанное количество милисекунд
						auto b = this->cnd.wait_for(lck, chrono::milliseconds(interval));
						// Если время вышло, выполняем функцию обратного вызова
						if(b == cv_status::timeout) function();
						// Иначе выходим из функции
						else return;
					}
				});
				// Выполняем ожидание завершения работы потока
				t.detach();
			}
			/**
			 * stop Метод остановки работы таймера
			 */
			void stop() noexcept {
				// Отсылаем сообщение потоку, на остановку таймера
				this->cnd.notify_one();
			}
	} timer_t;
};

#endif // __AWH_TIMER__
