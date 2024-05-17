/**
 * @file: timer.cpp
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

// Подключаем заголовочный файл
#include <lib/ev/core/timer.hpp>

/**
 * operator Оператор [()] Получения события таймера
 * @param timer   объект события таймаута
 * @param revents идентификатор события
 */
void awh::Timer::Timeout::operator()(ev::timer & timer, int revents) noexcept {
	// Зануляем неиспользуемые переменные
	(void) revents;
	// Если таймер был запущен
	if(timer.is_active())
		// Выполняем остановку таймера
		timer.stop();
	// Выполняем функцию обратного вызова
	this->_callback();
}
/**
 * event Метод события таймера
 * @param tid     идентификатор таймера
 * @param persist флаг персистентной работы
 */
void awh::Timer::event(const uint16_t tid, const bool persist) noexcept {
	// Выполняем поиск активного брокера
	auto i = this->_brokers.find(tid);
	// Если активный брокер найден
	if(i != this->_brokers.end()){
		// Если персистентная работа не установлена
		if(!persist){
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Выполняем удаление таймера
			this->_brokers.erase(i);
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Если функция обратного вызова существует
			if(this->_callbacks.is(static_cast <uint64_t> (tid))){
				// Объект работы с функциями обратного вызова
				fn_t callback(this->_log);
				// Копируем функции обратного вызова
				callback = this->_callbacks;
				// Выполняем блокировку потока
				this->_mtx.lock();
				// Выполняем удаление функции обратного вызова
				this->_callbacks.erase(static_cast <uint64_t> (tid));
				// Выполняем разблокировку потока
				this->_mtx.unlock();
				// Выполняем функцию обратного вызова
				callback.bind(static_cast <uint64_t> (tid));
			}
		// Если нужно продолжить работу таймера
		} else {
			// Если функция обратного вызова существует
			if(this->_callbacks.is(static_cast <uint64_t> (tid))){
				// Объект работы с функциями обратного вызова
				fn_t callback(this->_log);
				// Копируем функции обратного вызова
				callback = this->_callbacks;
				// Выполняем функцию обратного вызова
				callback.bind(static_cast <uint64_t> (tid));
				// Выполняем поиск активного брокера
				auto i = this->_brokers.find(tid);
				// Если активный брокер найден
				if(i != this->_brokers.end()){
					// Если таймер не был запущен ранее
					if(!i->second->io.is_active())
						// Продолжаем работу таймера
						i->second->io.again();
				}
			// Если таймер не был запущен ранее
			} else if(!i->second->io.is_active())
				// Продолжаем работу таймера
				i->second->io.again();
		}
	}
}
/**
 * launching Метод вызова при активации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer::launching(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	core_t::launching(mode, status);
}
/**
 * closedown Метод вызова при деакцтивации базы событий
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer::closedown(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	core_t::closedown(mode, status);
	// Если требуется закрыть подключение
	if(mode)
		// Выполняем остановку всех таймеров
		this->clear();
}
/**
 * clear Метод очистки всех таймеров
 */
void awh::Timer::clear() noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если список брокеров не пустой
	if(!this->_brokers.empty()){
		// Выполняем перебор всех активных брокеров
		for(auto i = this->_brokers.begin(); i != this->_brokers.end();){
			// Если таймер был запущен
			if(i->second->io.is_active())
				// Выполняем остановку активного брокера
				i->second->io.stop();
			// Если функция обратного вызова существует
			if(this->_callbacks.is(static_cast <uint64_t> (i->first)))
				// Выполняем удаление функции обратного вызова
				this->_callbacks.erase(static_cast <uint64_t> (i->first));
			// Выполняем удаление таймера
			i = this->_brokers.erase(i);
		}
	}
}
/**
 * clear Метод очистки таймера
 * @param tid идентификатор таймера для очистки
 */
void awh::Timer::clear(const uint16_t tid) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем поиск активного брокера
	auto i = this->_brokers.find(tid);
	// Если активный брокер найден
	if(i != this->_brokers.end()){
		// Если таймер был запущен
		if(i->second->io.is_active())
			// Выполняем остановку активного брокера
			i->second->io.stop();
		// Если функция обратного вызова существует
		if(this->_callbacks.is(static_cast <uint64_t> (tid)))
			// Выполняем удаление функции обратного вызова
			this->_callbacks.erase(static_cast <uint64_t> (tid));
		// Выполняем удаление таймера
		this->_brokers.erase(i);
	}
}
/**
 * timeout Метод создания таймаута
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::timeout(const time_t delay) noexcept {
	// Если данные переданы
	if((static_cast <struct ev_loop *> (this->_dispatch.base) != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = static_cast <uint16_t> (this->_brokers.size() + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(std::bind(&timer_t::event, this, tid, false))));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем приоритет выполнения
			ev_set_priority(&ret.first->second->io, 1);
			// Устанавливаем базу событий
			ret.first->second->io.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->io.set(&ret.first->second->timeout);
			// Запускаем работу таймера
			ret.first->second->io.start(delay / 1000.f);
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
 * interval Метод создания интервала
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::interval(const time_t delay) noexcept {
	// Если данные переданы
	if((static_cast <struct ev_loop *> (this->_dispatch.base) != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = static_cast <uint16_t> (this->_brokers.size() + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(std::bind(&timer_t::event, this, tid, true))));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем приоритет выполнения
			ev_set_priority(&ret.first->second->io, 1);
			// Получаем количество секунд
			const float seconds = (delay / 1000.f);
			// Устанавливаем базу событий
			ret.first->second->io.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->io.set(&ret.first->second->timeout);
			// Запускаем работу таймера
			ret.first->second->io.start(seconds, seconds);
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
 * ~Timer Деструктор
 */
awh::Timer::~Timer() noexcept {
	// Выполняем удаление всех таймеров
	this->clear();
}
