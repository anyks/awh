/**
 * @file: timer.cpp
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

// Подключаем заголовочный файл
#include <core/timer2.hpp>

/**
 * launching Метод вызова при активации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer2::launching(const bool mode, const bool status) noexcept {

}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer2::closedown(const bool mode, const bool status) noexcept {

}
/**
 * event Метод события таймера
 * @param tid  идентификатор таймера
 * @param fd   файловый дескриптор таймера
 * @param type тип отслеживаемого события
 */
void awh::Timer2::event(const uint16_t tid, const SOCKET fd, const base_t::event_type_t type) noexcept {

}
/**
 * clear Метод очистки всех таймеров
 */
void awh::Timer2::clear() noexcept {

}
/**
 * clear Метод очистки таймера
 * @param tid идентификатор таймера для очистки
 */
void awh::Timer2::clear(const uint16_t tid) noexcept {

}
/**
 * timeout Метод создания таймаута
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer2::timeout(const time_t delay) noexcept {
	return 0;
}
/**
 * interval Метод создания интервала
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer2::interval(const time_t delay) noexcept {
	// Если данные переданы
	if((this->_dispatch.base != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {


			
			/*
			SOCKET fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
			if(fd == -1) {
				perror("timerfd_create");
				return false;
			}
			*/

			/*
			
			{
				struct sigevent ev;
				clockid_t tid;
				// struct itimerspec its;

				ev.sigev_notify = SIGEV_SIGNAL;
				// ev.sigev_signo = SIGTOTEST;

				if (timer_create(CLOCK_MONOTONIC, &ev, &tid) != 0) {
					perror("timer_create() did not return success\n");
					return 0;
				}
			}
			*/

			
			
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = (this->_brokers.empty() ? 1 : this->_brokers.rbegin()->first + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(this->_fmk, this->_log)));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем флаг персистентной работы
			ret.first->second->persist = true;
			// Устанавливаем базу данных событий
			// ret.first->second->event.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->event.set(std::bind(&timer2_t::event, this, ret.first->first, _1, _2));
			// Выполняем запуск работы таймера
			ret.first->second->event.start();
			// Выполняем активацию чтение сокета
			ret.first->second->event.mode(base_t::event_type_t::READ, base_t::event_mode_t::ENABLED);
			// Выводим идентификатор таймера
			return ret.first->first;
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * Timer Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Timer2::Timer2(const fmk_t * fmk, const log_t * log) noexcept : awh::core_t(fmk, log), _callbacks(log), _fmk(fmk), _log(log) {
	// Создаём очередь для работы с таймером
	this->_queue = dispatch_queue_create("AWHTimer", 0);
}
/**
 * ~Timer Деструктор
 */
awh::Timer2::~Timer2() noexcept {
	// Выполняем удаление всех таймеров
	this->clear();
}
