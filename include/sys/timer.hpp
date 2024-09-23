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

#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <functional>
#include <condition_variable>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

// Устанавливаем пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timer Класс таймера
	 */
	typedef class AWHSHARED_EXPORT Timer {
		private:
			/**
			 * Worker Класс воркера
			 */
			typedef struct Worker {
				// Флаг остановки работы таймера
				bool stop;
				// Мютекс для блокировки потока
				mutex mtx;
				// Задержка времени таймера
				uint32_t delay;
				// Условная переменная, ожидания получения сигналов
				condition_variable cv;
				// Функция обратного вызова
				function <void (void)> callback;
				/**
				 * Worker Конструктор
				 */
				Worker() noexcept : stop(false), delay(0), callback(nullptr) {}
			} wrk_t;
		private:
			// Мютекс для блокировки потока
			mutable mutex _mtx;
		private:
			// Список активных воркеров таймера
			map <uint32_t, unique_ptr <wrk_t>> _timers;
		private:
			/**
			 * checkInputData Метод проверки на существование данных
			 * @param tid идентификатор таймера
			 * @return    результат проверки
			 */
			bool checkInputData(const uint32_t tid) const noexcept {
				// Результат работы функции
				bool result = true;
				// Выполняем блокировку потока
				const lock_guard <mutex> lock(this->_mtx);
				// Выполняем поиск идентификатор таймера
				auto i = this->_timers.find(tid);
				// Если идентификатор таймера в списке существует
				if(i != this->_timers.end())
					// Выполняем установку флага остановки таймера
					result = i->second->stop;
				// Выводим результат
				return result;
			}
		public:
			/**
			 * setTimeout Метод установки таймаута
			 * @param callback функция обратного вызова
			 * @param delay    интервал задержки времени
			 * @return         идентификатор таймаута
			 */
			uint32_t setTimeout(function <void (void)> callback, const uint32_t delay) noexcept {
				// Выполняем получение идентификатора таймаута
				const uint32_t tid = (this->_timers.size() + 1);
				// Выполняем блокировку потока
				this->_mtx.lock();
				// Выполняем добавление идентификатора таймера в список
				auto ret = this->_timers.emplace(tid, unique_ptr <wrk_t> (new wrk_t));
				// Выполняем разблокировку потока
				this->_mtx.unlock();
				// Выполняем установку задержки времени
				ret.first->second->delay = delay;
				// Выполняем установку функции обратного вызова
				ret.first->second->callback = callback;
				// Выполняем создание нового активного потока
				std::thread([=]{
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Выполняем поиск нашего таймера
						auto i = this->_timers.find(tid);
						// Если таймер найден
						if(i != this->_timers.end()){
							// Выполняем блокировку уникальным мютексом
							unique_lock <mutex> lock(i->second->mtx);
							// Выполняем ожидание на поступление новых заданий
							i->second->cv.wait_for(lock, chrono::milliseconds(i->second->delay), std::bind(&Timer::checkInputData, this, tid));
						}{
							// Выполняем поиск нашего таймера
							auto i = this->_timers.find(tid);
							// Если идентификатор таймера в списке существует и остановка не подтверждена
							if((i != this->_timers.end()) && !i->second->stop){
								// Выполняем функцию обратного вызова
								i->second->callback();
								{
									// Выполняем блокировку потока
									const lock_guard <mutex> lock(this->_mtx);
									// Выполняем поиск нашего таймера
									auto i = this->_timers.find(tid);
									// Если идентификатор таймера в списке существует и остановка не подтверждена
									if(i != this->_timers.end())
										// Выполняем удаление идентификатора таймера
										this->_timers.erase(i);
								}
							// Если была выполнена остановка
							} else if(i != this->_timers.end()) {
								// Выполняем блокировку потока
								const lock_guard <mutex> lock(this->_mtx);
								// Выполняем удаление идентификатора таймера
								this->_timers.erase(i);
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						// Выполняем блокировку потока
						const lock_guard <mutex> lock(this->_mtx);
						// Выполняем поиск идентификатор таймера
						auto i = this->_timers.find(tid);
						// Если идентификатор таймера в списке существует
						if(i != this->_timers.end())
							// Выполняем удаление идентификатора таймера
							this->_timers.erase(i);
					}
				}).detach();
				// Выводим результат
				return tid;
			}
			/**
			 * setInterval Метод установки таймаута
			 * @param callback функция для вызова
			 * @param interval интервал времени выполнения функции
			 * @return         идентификатор таймаута
			 */
			uint32_t setInterval(function <void (void)> callback, const uint32_t interval) noexcept {
				// Выполняем получение идентификатора таймаута
				const uint32_t tid = (this->_timers.size() + 1);
				// Выполняем блокировку потока
				this->_mtx.lock();
				// Выполняем добавление идентификатора таймера в список
				auto ret = this->_timers.emplace(tid, unique_ptr <wrk_t> (new wrk_t));
				// Выполняем разблокировку потока
				this->_mtx.unlock();
				// Выполняем установку задержки времени
				ret.first->second->delay = interval;
				// Выполняем установку функции обратного вызова
				ret.first->second->callback = callback;
				// Выполняем создание нового активного потока
				std::thread([=]{
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Создаём бесконечный цикл
						for(;;){
							// Выполняем поиск нашего таймера
							auto i = this->_timers.find(tid);
							// Если таймер найден
							if(i != this->_timers.end()){
								// Выполняем блокировку уникальным мютексом
								unique_lock <mutex> lock(i->second->mtx);
								// Выполняем ожидание на поступление новых заданий
								i->second->cv.wait_for(lock, chrono::milliseconds(i->second->delay), std::bind(&Timer::checkInputData, this, tid));
							}{
								// Выполняем поиск идентификатор таймера
								auto i = this->_timers.find(tid);
								// Если идентификатор таймера в списке существует и остановка не подтверждена
								if((i != this->_timers.end()) && !i->second->stop)
									// Выполняем функцию обратного вызова
									i->second->callback();
								// Если была выполнена остановка
								else if(i != this->_timers.end()) {
									// Выполняем блокировку потока
									const lock_guard <mutex> lock(this->_mtx);
									// Выполняем удаление идентификатора таймера
									this->_timers.erase(i);
									// Выходим из цикла
									break;
								}
							}
						}
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						// Выполняем блокировку потока
						const lock_guard <mutex> lock(this->_mtx);
						// Выполняем поиск идентификатор таймера
						auto i = this->_timers.find(tid);
						// Если идентификатор таймера в списке существует
						if(i != this->_timers.end())
							// Выполняем удаление идентификатора таймера
							this->_timers.erase(i);
					}
				}).detach();
				// Выводим результат
				return tid;
			}
			/**
			 * stop Метод остановки работы таймера
			 * @param tid идентификатор таймера
			 */
			void stop(const uint32_t tid) noexcept {
				// Выполняем блокировку потока
				const lock_guard <mutex> lock(this->_mtx);
				// Выполняем поиск идентификатор таймера
				auto i = this->_timers.find(tid);
				// Если идентификатор таймера в списке существует
				if(i != this->_timers.end()){
					// Выполняем установку флага остановки таймера
					i->second->stop = true;
					// Отправляем сообщение, что данные записаны
					i->second->cv.notify_one();
				}
			}
			/**
			 * Timer Конструктор
			 */
			Timer() noexcept {}
			/**
			 * ~Timer Деструктор
			 */
			~Timer() noexcept {
				// Если список активных таймеров не пустой
				if(!this->_timers.empty()){
					// Выполняем перебор всех активных таймеров
					for(auto & timer : this->_timers){
						// Выполняем установку флага остановки таймера
						timer.second->stop = true;
						// Отправляем сообщение, что данные записаны
						timer.second->cv.notify_one();
					}
				}
				// Дожидаемся пока все таймеры будут удалены
				while(!this->_timers.empty());
			}
	} timer_t;
};

#endif // __AWH_TIMER__
