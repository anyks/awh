/**
 * @file: events.cpp
 * @date: 2022-09-15
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

// Подключаем заголовочный файл
#include <lib/event2/sys/events.hpp>

/**
 * callback Функция обратного вызова при срабатывании события
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 * @param ctx   передаваемый контекст
 */
void awh::Event::callback(evutil_socket_t fd, short event, void * ctx) noexcept {
	// Получаем объект события
	event_t * ev = reinterpret_cast <event_t *> (ctx);
	// Если функция обратного вызова установлена
	if(ev->_fn != nullptr)
		// Выполняем функцию обратного вызова
		ev->_fn(fd, event);
}
/**
 * set Метод установки времени таймера
 * @param delay задержка времени в миллисекундах
 */
void awh::Event::set(const time_t delay) noexcept {
	// Устанавливаем время задержки в миллисекундах
	this->_delay = delay;
}
/**
 * set Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Event::set(struct event_base * base) noexcept {
	// Устанавливаем базу данных событий
	this->_base = base;
}
/**
 * set Метод установки файлового дескриптора и списка событий
 * @param fd     файловый дескриптор для установки
 * @param events список событий файлового дескриптора
 */
void awh::Event::set(const evutil_socket_t fd, const int events) noexcept {
	// Устанавливаем файловый дескриптор
	this->_fd = fd;
	// Устанавливаем список событий
	this->_events = events;
}
/**
 * set Метод установки функции обратного вызова
 * @param callback функция обратного вызова
 */
void awh::Event::set(function <void (const evutil_socket_t, const int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_fn = callback;
}
/**
 * stop Метод остановки работы события
 */
void awh::Event::stop() noexcept {
	// Если работа события запущена
	if(this->_mode){
		// Очищаем объект времени таймера
		evutil_timerclear(&this->_tv);
		// Если событие является таймаутом
		if(this->_events & EV_TIMEOUT)
			// Удаляем событие таймера
			evtimer_del(&this->_ev);
		// Удаляем событие
		else event_del(&this->_ev);
		// Снимаем флаг запущенной работы
		this->_mode = !this->_mode;
	}
}
/**
 * start Метод запуска работы события
 * @param delay задержка времени в миллисекундах
 */
void awh::Event::start(const time_t delay) noexcept {
	// Если задержка времени передана
	if(delay > 0)
		// Устанавливаем время задержки в миллисекундах
		this->_delay = delay;
	// Если работа события ещё не запущена
	if(!this->_mode && (this->_base != nullptr)){
		// Устанавливаем флаг запущенной работы
		this->_mode = !this->_mode;
		// Определяем тип выполняемого события
		switch(static_cast <uint8_t> (this->_type)){
			// Если событие является сигналом
			case static_cast <uint8_t> (type_t::SIGNAL): {
				// Создаём событие на активацию базы событий
				evsignal_assign(&this->_ev, this->_base, this->_events, &callback, this);
				// Если таймаут установлен
				if(this->_delay > 0){
					// Устанавливаем время в секундах
					this->_tv.tv_sec = (this->_delay / 1000);
					// Устанавливаем время счётчика (микросекунды)
					this->_tv.tv_usec = ((this->_delay % 1000) * 1000);
					// Выполняем отслеживание возникающего сигнала
					evsignal_add(&this->_ev, &this->_tv);
				// Выполняем отслеживание возникающего сигнала
				} else evsignal_add(&this->_ev, nullptr);
			} break;
			// Если событие является таймером
			case static_cast <uint8_t> (type_t::TIMER): {
				// Если событие является таймаутом
				if(this->_events & EV_TIMEOUT){
					// Если задержка времени передана
					if(this->_delay > 0){
						// Устанавливаем время в секундах
						this->_tv.tv_sec = (this->_delay / 1000);
						// Устанавливаем время счётчика (микросекунды)
						this->_tv.tv_usec = ((this->_delay % 1000) * 1000);
						// Создаём событие на активацию базы событий
						evtimer_assign(&this->_ev, this->_base, &callback, this);
						// Создаём событие таймаута на активацию базы событий
						evtimer_add(&this->_ev, &this->_tv);
					}
				// Выводим сообщение об ошибке
				} else this->_log->print("Signal type for the timer is formed incorrectly", log_t::flag_t::WARNING);
			} break;
			// Если событие является сокетом
			case static_cast <uint8_t> (type_t::EVENT): {
				// Если событие является стандартным
				if(this->_fd > 0){
					// Создаём событие на активацию базы событий
					event_assign(&this->_ev, this->_base, this->_fd, this->_events, &callback, this);
					// Если таймаут установлен
					if(this->_delay > 0){
						// Устанавливаем время в секундах
						this->_tv.tv_sec = (this->_delay / 1000);
						// Устанавливаем время счётчика (микросекунды)
						this->_tv.tv_usec = ((this->_delay % 1000) * 1000);
						// Выполняем активацию события
						event_add(&this->_ev, &this->_tv);
					// Выполняем активацию события
					} else event_add(&this->_ev, nullptr);
				// Выводим сообщение об ошибке
				} else this->_log->print("File descriptor is not set", log_t::flag_t::WARNING);
			} break;
			// Если событие не установлено
			default: this->_log->print("Event type is not set", log_t::flag_t::CRITICAL);
		}
	}
}
/**
 * ~Event Деструктор
 */
awh::Event::~Event() noexcept {
	// Выполняем остановку работы события
	this->stop();
}
