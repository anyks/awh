/**
 * @file: timer.cpp
 * @date: 2024-07-15
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
#include <core/timer.hpp>

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
 * event Метод события таймера
 * @param tid   идентификатор таймера
 * @param fd    файловый дескриптор (сокет)
 * @param event произошедшее событие
 */
void awh::Timer::event(const uint16_t tid, const SOCKET fd, const base_t::event_type_t event) noexcept {
	// Блокируем неиспользуемые переменные
	(void) fd;
	// Определяем тип события
	switch(static_cast <uint8_t> (event)){
		// Если выполняется событие таймера
		case static_cast <uint8_t> (base_t::event_type_t::TIMER): {
			// Выполняем поиск активного брокера
			auto i = this->_brokers.find(tid);
			// Если активный брокер найден
			if(i != this->_brokers.end()){
				// Если персистентная работа не установлена, удаляем таймер
				if(!i->second->persist){
					// Выполняем остановку активного брокера
					i->second->event.stop();
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
				// Если функция обратного вызова существует
				} else if(this->_callbacks.is(static_cast <uint64_t> (tid))) {
					// Объект работы с функциями обратного вызова
					fn_t callback(this->_log);
					// Копируем функции обратного вызова
					callback = this->_callbacks;
					// Выполняем функцию обратного вызова
					callback.bind(static_cast <uint64_t> (tid));
				}
			}
		} break;
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
		for(auto i = this->_brokers.begin(); i != this->_brokers.end();){
			// Выполняем остановку активного брокера
			i->second->event.stop();
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
		// Выполняем остановку активного брокера
		i->second->event.stop();
		// Если функция обратного вызова существует
		if(this->_callbacks.is(static_cast <uint64_t> (tid)))
			// Выполняем удаление функции обратного вызова
			this->_callbacks.erase(static_cast <uint64_t> (tid));
		{
			// Выполняем поиск активного брокера
			auto i = this->_brokers.find(tid);
			// Если активный брокер найден
			if(i != this->_brokers.end())
				// Выполняем удаление таймера
				this->_brokers.erase(i);
		}
	}
}
/**
 * timeout Метод создания таймаута
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::timeout(const time_t delay) noexcept {
	// Если данные переданы
	if((this->_dispatch.base != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = (this->_brokers.empty() ? 1 : this->_brokers.rbegin()->first + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, unique_ptr <broker_t> (new broker_t(this->_fmk, this->_log)));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем время задержки таймера
			ret.first->second->event.timeout(delay);
			// Устанавливаем базу данных событий
			ret.first->second->event = this->_dispatch.base;
			// Устанавливаем функцию обратного вызова
			ret.first->second->event = std::bind(&timer_t::event, this, ret.first->first, _1, _2);
			// Выполняем запуск работы таймера
			ret.first->second->event.start();
			// Выполняем запуск работы таймера
			ret.first->second->event.mode(base_t::event_type_t::TIMER, base_t::event_mode_t::ENABLED);
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
	if((this->_dispatch.base != nullptr) && (delay > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
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
			// Устанавливаем время задержки таймера
			ret.first->second->event.timeout(delay, true);
			// Устанавливаем базу данных событий
			ret.first->second->event = this->_dispatch.base;
			// Устанавливаем функцию обратного вызова
			ret.first->second->event = std::bind(&timer_t::event, this, ret.first->first, _1, _2);
			// Выполняем запуск работы таймера
			ret.first->second->event.start();
			// Выполняем запуск работы таймера
			ret.first->second->event.mode(base_t::event_type_t::TIMER, base_t::event_mode_t::ENABLED);
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
