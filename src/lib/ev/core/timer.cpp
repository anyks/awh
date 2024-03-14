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
	// Выполняем остановку таймера
	timer.stop();
	// Выполняем функцию события таймера
	this->_broker->fn(this->_tid, this->_delay);
}
/**
 * event Метод события таймера
 * @param tid   идентификатор таймера
 * @param delay задержка времени в секундах
 */
void awh::Timer::event(const uint16_t tid, const float delay) noexcept {
	// Выполняем поиск активного брокера
	auto it = this->_brokers.find(tid);
	// Если активный брокер найден
	if(it != this->_brokers.end()){
		// Если персистентная работа не установлена, удаляем таймер
		if(!it->second->persist){
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Выполняем удаление таймера
			this->_brokers.erase(it);
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
				auto it = this->_brokers.find(tid);
				// Если активный брокер найден
				if(it != this->_brokers.end())
					// Продолжаем работу дальше
					it->second->io.start(delay);
			// Продолжаем работу дальше
			} else it->second->io.start(delay);
		}
	}
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
		for(auto it = this->_brokers.begin(); it != this->_brokers.end();){
			// Выполняем остановку активного брокера
			it->second->io.stop();
			// Если функция обратного вызова существует
			if(this->_callbacks.is(static_cast <uint64_t> (it->first)))
				// Выполняем удаление функции обратного вызова
				this->_callbacks.erase(static_cast <uint64_t> (it->first));
			// Выполняем удаление таймера
			it = this->_brokers.erase(it);
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
	auto it = this->_brokers.find(tid);
	// Если активный брокер найден
	if(it != this->_brokers.end()){
		// Выполняем остановку активного брокера
		it->second->io.stop();
		// Если функция обратного вызова существует
		if(this->_callbacks.is(static_cast <uint64_t> (tid)))
			// Выполняем удаление функции обратного вызова
			this->_callbacks.erase(static_cast <uint64_t> (tid));
		// Выполняем удаление таймера
		this->_brokers.erase(it);
	}
}
/**
 * timeout Метод создания таймаута
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::timeout(const time_t delay) noexcept {
	// Результат работы функции
	uint16_t result = 0;
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
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(tid, delay / static_cast <float> (1000))));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Получаем идентификатор таймера
			result = ret.first->first;
			// Устанавливаем приоритет выполнения
			ev_set_priority(&ret.first->second->io, 1);
			// Устанавливаем функцию обратного вызова
			ret.first->second->fn = std::bind(&timer_t::event, this, _1, _2);
			// Устанавливаем базу событий
			ret.first->second->io.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->io.set(&ret.first->second->timeout);
			// Запускаем работу таймера
			ret.first->second->io.start(delay / static_cast <float> (1000));
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	// Выводим результат
	return result;
}
/**
 * interval Метод создания интервала
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::interval(const time_t delay) noexcept {
	// Результат работы функции
	uint16_t result = 0;
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
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(tid, delay / static_cast <float> (1000))));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Получаем идентификатор таймера
			result = ret.first->first;
			// Устанавливаем приоритет выполнения
			ev_set_priority(&ret.first->second->io, 1);
			// Устанавливаем флаг персистентной работы
			ret.first->second->persist = true;
			// Устанавливаем функцию обратного вызова
			ret.first->second->fn = std::bind(&timer_t::event, this, _1, _2);
			// Устанавливаем базу событий
			ret.first->second->io.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->io.set(&ret.first->second->timeout);
			// Запускаем работу таймера
			ret.first->second->io.start(delay / static_cast <float> (1000));
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	// Выводим результат
	return result;
}
/**
 * ~Timer Деструктор
 */
awh::Timer::~Timer() noexcept {
	// Выполняем удаление всех таймеров
	this->clear();
}
