/**
 * @file: tmos.cpp
 * @date: 2024-07-02
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

/**
 * Подключаем заголовочный файл
 */
#include <sys/tmos.hpp>

/**
 * trigger Метод обработки событий триггера
 */
void awh::TimerOfScreen::trigger() noexcept {
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		this->_mtx.lock();
		// Идентификатор файлового дескриптора (сокета)
		SOCKET fd = 0;
		// Количество прошедшего времени
		time_t elapsed = 0;
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Определяем сколько прошло времени
			elapsed = (this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS) - this->_date);
			// Если время вышло
			if(elapsed >= i->first){
				// Выполняем запись в сокет данных
				::write(i->second, &i->second, sizeof(i->second));
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
			// Выходим из цикла
			} else {
				// Получаем файловый дескриптор
				fd = i->second;
				// Если времени больше не усталось
				if((elapsed = (i->first - elapsed)) == 0){
					// Выполняем запись в сокет данных
					::write(i->second, &i->second, sizeof(i->second));
					// Выполняем удаление значение таймера
					i = this->_timers.erase(i);
				// Выполняем замену таймера в списке
				} else {
					// Выполняем удаление значение таймера
					this->_timers.erase(i);
					// Добавляем новый элемент в список
					i = this->_timers.emplace(elapsed, fd);
					// Выполняем обход дальше
					++i;
				}
			}
		}
		// Запоминаем текущее значение даты
		this->_date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
		// Выполняем разблокировку потока
		this->_mtx.unlock();
		// Выполняем смену времени таймера
		this->_screen = static_cast <time_t> (this->_timers.begin()->first);
	// Устанавливаем таймаут по умолчанию
	} else this->_screen.timeout();
}
/**
 * process Метод обработки процесса добавления таймеров
 * @param data данные таймера для добавления
 */
void awh::TimerOfScreen::process(const data_t data) noexcept {
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Идентификатор файлового дескриптора (сокета)
		SOCKET fd = 0;
		// Количество прошедшего времени
		time_t elapsed = 0;
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end();){
			// Определяем сколько прошло времени
			elapsed = (this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS) - this->_date);
			// Если время вышло
			if(elapsed >= i->first){
				// Выполняем запись в сокет данных
				::write(i->second, &i->second, sizeof(i->second));
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
			// Выходим из цикла
			} else {
				// Получаем файловый дескриптор
				fd = i->second;
				// Если времени больше не усталось
				if((elapsed = (i->first - elapsed)) == 0){
					// Выполняем запись в сокет данных
					::write(i->second, &i->second, sizeof(i->second));
					// Выполняем удаление значение таймера
					i = this->_timers.erase(i);
				// Выполняем замену таймера в списке
				} else {
					// Выполняем удаление значение таймера
					this->_timers.erase(i);
					// Добавляем новый элемент в список
					i = this->_timers.emplace(elapsed, fd);
					// Выполняем обход дальше
					++i;
				}
			}
		}
	}
	// Выполняем добавления нового таймера
	auto i = this->_timers.emplace(data.delay, data.fd);
	// Делаем сокет неблокирующим
	this->_socket.blocking(i->second, socket_t::mode_t::DISABLE);
	// Запоминаем текущее значение даты
	this->_date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
	// Выполняем разблокировку потока
	this->_mtx.unlock();
	// Если время задержки выше 0
	if(this->_timers.begin()->first > 0)
		// Выполняем смену времени таймера
		this->_screen = static_cast <time_t> (this->_timers.begin()->first);
	// Устанавливаем таймаут по умолчанию
	else this->_screen.timeout();
}
/**
 * stop Метод остановки работы таймера
 */
void awh::TimerOfScreen::stop() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
/**
 * start Метод запуска работы таймера
 */
void awh::TimerOfScreen::start() noexcept {
	// Выполняем запуск работы экрана
	this->_screen.start();
}
/**
 * del Метод удаления таймера
 * @param fd файловый дескриптор таймера
 */
void awh::TimerOfScreen::del(const SOCKET fd) noexcept {
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end(); ++i){
			// Если таймер найден
			if(i->second == fd){
				// Выполняем удаление таймера
				this->_timers.erase(i);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * add Метод добавления таймера
 * @param fd    файловый дескриптор таймера
 * @param delay задержка времени в наносекундах
 */
void awh::TimerOfScreen::add(const SOCKET fd, const time_t delay) noexcept {
	// Создаём объект даты для передачи
	data_t data;
	// Устанавливаем файловый дескриптор
	data.fd = fd;
	// Устанавливаем задержку времени в наносекундах
	data.delay = delay;
	// Выполняем отправку события экрану
	this->_screen = data;
}
/**
 * TimerOfScreen Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::TimerOfScreen::TimerOfScreen(const fmk_t * fmk, const log_t * log) noexcept :
 _date(0), _socket(fmk, log), _screen(screen_t <data_t>::health_t::DEAD), _fmk(fmk), _log(log) {
	// Выполняем добавление функции обратного вызова триггера
	this->_screen = static_cast <function <void (void)>> (std::bind(&tmos_t::trigger, this));
	// Выполняем добавление функции обратного вызова процесса обработки
	this->_screen = static_cast <function <void (const data_t)>> (std::bind(&tmos_t::process, this, _1));
}
/**
 * ~TimerOfScreen Деструктор
 */
awh::TimerOfScreen::~TimerOfScreen() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
