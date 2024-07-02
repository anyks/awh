/**
 * @file: timeout.cpp
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
#include <sys/timeout.hpp>

/**
 * trigger Метод обработки событий триггера
 */
void awh::Timeout::trigger() noexcept {
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
			// Получаем файловый дескриптор
			fd = i->second;
			// Определяем сколько прошло времени
			elapsed = (this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS) - this->_date);
			// Если время вышло
			if(elapsed >= i->first){
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
				// Выполняем удаление идентификатора таймера
				this->_ids.erase(fd);
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем запись в сокет данных
					::_write(fd, &fd, sizeof(fd));
				/**
				 * Методя для всех остальных операционных систем
				 */
				#else
					// Выполняем запись в сокет данных
					::write(fd, &fd, sizeof(fd));
				#endif
			// Выходим из цикла
			} else {
				// Если времени больше не усталось
				if((elapsed = (i->first - elapsed)) == 0){
					// Выполняем удаление значение таймера
					i = this->_timers.erase(i);
					// Выполняем удаление идентификатора таймера
					this->_ids.erase(fd);
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем запись в сокет данных
						::_write(fd, &fd, sizeof(fd));
					/**
					 * Методя для всех остальных операционных систем
					 */
					#else
						// Выполняем запись в сокет данных
						::write(fd, &fd, sizeof(fd));
					#endif
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
		// Если список таймеров не пустой и время задержки выше 0
		if(!this->_timers.empty() && (this->_timers.begin()->first > 0))
			// Выполняем смену времени таймера
			this->_screen = static_cast <time_t> (this->_timers.begin()->first);
		// Устанавливаем таймаут по умолчанию
		else this->_screen.timeout();
	// Устанавливаем таймаут по умолчанию
	} else this->_screen.timeout();
}
/**
 * process Метод обработки процесса добавления таймеров
 * @param data данные таймера для добавления
 */
void awh::Timeout::process(const data_t data) noexcept {
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
			// Получаем файловый дескриптор
			fd = i->second;
			// Определяем сколько прошло времени
			elapsed = (this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS) - this->_date);
			// Если время вышло
			if(elapsed >= i->first){
				// Выполняем удаление значение таймера
				i = this->_timers.erase(i);
				// Выполняем удаление идентификатора таймера
				this->_ids.erase(fd);
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем запись в сокет данных
					::_write(fd, &fd, sizeof(fd));
				/**
				 * Методя для всех остальных операционных систем
				 */
				#else
					// Выполняем запись в сокет данных
					::write(fd, &fd, sizeof(fd));
				#endif
			// Выходим из цикла
			} else {
				// Если времени больше не усталось
				if((elapsed = (i->first - elapsed)) == 0){
					// Выполняем удаление значение таймера
					i = this->_timers.erase(i);
					// Выполняем удаление идентификатора таймера
					this->_ids.erase(fd);
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем запись в сокет данных
						::_write(fd, &fd, sizeof(fd));
					/**
					 * Методя для всех остальных операционных систем
					 */
					#else
						// Выполняем запись в сокет данных
						::write(fd, &fd, sizeof(fd));
					#endif
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
	// Если сокет таймера ещё не добавлен
	if(this->_ids.find(data.fd) == this->_ids.end()){
		// Выполняем добавления нового таймера
		auto i = this->_timers.emplace(data.delay, data.fd);
		// Делаем сокет неблокирующим
		this->_socket.blocking(i->second, socket_t::mode_t::DISABLE);
	}
	// Запоминаем текущее значение даты
	this->_date = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
	// Выполняем разблокировку потока
	this->_mtx.unlock();
	// Если список таймеров не пустой и время задержки выше 0
	if(!this->_timers.empty() && (this->_timers.begin()->first > 0))
		// Выполняем смену времени таймера
		this->_screen = static_cast <time_t> (this->_timers.begin()->first);
	// Устанавливаем таймаут по умолчанию
	else this->_screen.timeout();
}
/**
 * stop Метод остановки работы таймера
 */
void awh::Timeout::stop() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
/**
 * start Метод запуска работы таймера
 */
void awh::Timeout::start() noexcept {
	// Выполняем запуск работы экрана
	this->_screen.start();
}
/**
 * del Метод удаления таймера
 * @param fd файловый дескриптор таймера
 */
void awh::Timeout::del(const SOCKET fd) noexcept {
	// Если список таймеров не пустой
	if(!this->_timers.empty()){
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем перебор всего списка таймеров
		for(auto i = this->_timers.begin(); i != this->_timers.end(); ++i){
			// Если таймер найден
			if(i->second == fd){
				// Выполняем удаление идентификатора таймера
				this->_ids.erase(fd);
				// Выполняем удаление таймера
				this->_timers.erase(i);
				// Выходим из цикла
				break;
			}
		}
	}
}
/**
 * set Метод установки таймера
 * @param fd    файловый дескриптор таймера
 * @param delay задержка времени в наносекундах
 */
void awh::Timeout::set(const SOCKET fd, const time_t delay) noexcept {
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
 * Timeout Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Timeout::Timeout(const fmk_t * fmk, const log_t * log) noexcept :
 _date(0), _socket(fmk, log), _screen(screen_t <data_t>::health_t::DEAD), _fmk(fmk), _log(log) {
	// Выполняем добавление функции обратного вызова триггера
	this->_screen = static_cast <function <void (void)>> (std::bind(&timeout_t::trigger, this));
	// Выполняем добавление функции обратного вызова процесса обработки
	this->_screen = static_cast <function <void (const data_t)>> (std::bind(&timeout_t::process, this, _1));
}
/**
 * ~Timeout Деструктор
 */
awh::Timeout::~Timeout() noexcept {
	// Выполняем остановку работы экрана
	this->_screen.stop();
}
