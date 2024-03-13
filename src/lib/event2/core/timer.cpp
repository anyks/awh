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
#include <lib/event2/core/timer.hpp>

/**
 * callback Метод обратного вызова
 * @param tid   идентификатор таймера
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::Timer::callback(const uint16_t tid, const evutil_socket_t fd, const short event) noexcept {
	// Выполняем поиск активного брокера
	auto it = this->_brokers.find(tid);
	// Если активный брокер найден
	if(it != this->_brokers.end()){
		// Выполняем остановку активного брокера
		it->second->event.stop();
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
					it->second->event.start();
			// Продолжаем работу дальше
			} else it->second->event.start();
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
			it->second->event.stop();
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
		it->second->event.stop();
		// Если функция обратного вызова существует
		if(this->_callbacks.is(static_cast <uint64_t> (tid)))
			// Выполняем удаление функции обратного вызова
			this->_callbacks.erase(static_cast <uint64_t> (tid));
		// Выполняем удаление таймера
		this->_brokers.erase(it);
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::Timer::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем копирование функций обратного вызова
	this->_callbacks = callbacks;
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
	if((this->_dispatch.base != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(static_cast <uint16_t> (this->_brokers.size() + 1), unique_ptr <broker_t> (new broker_t(this->_log)));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Получаем идентификатор таймера
			result = ret.first->first;
			// Устанавливаем время задрежки таймера
			ret.first->second->delay = delay;
			// Устанавливаем тип таймера
			ret.first->second->event.set(-1, EV_TIMEOUT);
			// Устанавливаем базу данных событий
			ret.first->second->event.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->event.set(std::bind(&timer_t::callback, this, result, _1, _2));
			// Выполняем запуск работы таймера
			ret.first->second->event.start(ret.first->second->delay);
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
	if((this->_dispatch.base != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(static_cast <uint16_t> (this->_brokers.size() + 1), unique_ptr <broker_t> (new broker_t(this->_log)));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Получаем идентификатор таймера
			result = ret.first->first;
			// Устанавливаем время задрежки таймера
			ret.first->second->delay = delay;
			// Устанавливаем флаг персистентной работы
			ret.first->second->persist = true;
			// Устанавливаем тип таймера
			ret.first->second->event.set(-1, EV_TIMEOUT);
			// Устанавливаем базу данных событий
			ret.first->second->event.set(this->_dispatch.base);
			// Устанавливаем функцию обратного вызова
			ret.first->second->event.set(std::bind(&timer_t::callback, this, result, _1, _2));
			// Выполняем запуск работы таймера
			ret.first->second->event.start(ret.first->second->delay);
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
