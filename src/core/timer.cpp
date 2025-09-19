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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <core/timer.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод вызова при активации базы событий
 *
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer::launching(const bool mode, const bool status) noexcept {
	// Выполняем функцию в базовом модуле
	core_t::launching(mode, status);
}
/**
 * @brief Метод вызова при деакцтивации базы событий
 *
 * @param mode   флаг работы с сетевым протоколом
 * @param status флаг вывода события статуса
 */
void awh::Timer::closedown(const bool mode, const bool status) noexcept {
	// Если требуется закрыть подключение
	if(mode)
		// Выполняем остановку всех таймеров
		this->clear();
	// Выполняем функцию в базовом модуле
	core_t::closedown(mode, status);
}
/**
 * @brief Метод события таймера
 *
 * @param tid   идентификатор таймера
 * @param sock  сетевой сокет
 * @param event произошедшее событие
 */
void awh::Timer::event(const uint16_t tid, [[maybe_unused]] const SOCKET sock, const base_t::event_type_t event) noexcept {
	/**
	 * Определяем тип события
	 */
	switch(static_cast <uint8_t> (event)){
		// Если выполняется событие таймера
		case static_cast <uint8_t> (base_t::event_type_t::TIMER): {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
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
						// Выполняем поиск функции обратного вызова
						auto j = this->_callback.find(tid);
						// Если функция обратного вызова найдена
						if(j != this->_callback.end()){
							// Выполняем извлечение функции обратного вызова
							auto fn = j->second;
							// Выполняем блокировку потока
							this->_mtx.lock();
							// Выполняем удаление функции обратного вызова
							this->_callback.erase(j);
							// Выполняем разблокировку потока
							this->_mtx.unlock();
							// Выполняем функцию обратного вызова
							std::apply(fn, std::make_tuple());
						}
					// Если мы работаем не с таймером а с интервалом
					} else {
						// Выполняем поиск функции обратного вызова
						auto j = this->_callback.find(i->first);
						// Если функция обратного вызова найдена
						if(j != this->_callback.end()){
							// Выполняем извлечение функции обратного вызова
							auto fn = j->second;
							// Выполняем функцию обратного вызова
							std::apply(fn, std::make_tuple());
						}
					}
				}
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(tid, sock, static_cast <uint16_t> (event)), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		} break;
	}
}
/**
 * @brief Метод очистки всех таймеров
 *
 */
void awh::Timer::clear() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Если список брокеров не пустой
		if(!this->_brokers.empty()){
			// Выполняем перебор всех активных брокеров
			for(auto i = this->_brokers.begin(); i != this->_brokers.end();){
				// Выполняем остановку активного брокера
				i->second->event.stop();
				// Если функция обратного вызова существует
				auto j = this->_callback.find(i->first);
				// Если функция обратного вызова найдена
				if(j != this->_callback.end())
					// Выполняем удаление функции обратного вызова
					this->_callback.erase(j);
				// Выполняем удаление таймера
				i = this->_brokers.erase(i);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки таймера
 *
 * @param tid идентификатор таймера для очистки
 */
void awh::Timer::clear(const uint16_t tid) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::recursive_mutex> lock(this->_mtx);
		// Выполняем поиск активного брокера
		auto i = this->_brokers.find(tid);
		// Если активный брокер найден
		if(i != this->_brokers.end()){
			// Выполняем остановку активного брокера
			i->second->event.stop();
			// Если функция обратного вызова существует
			auto j = this->_callback.find(i->first);
			// Если функция обратного вызова найдена
			if(j != this->_callback.end())
				// Выполняем удаление функции обратного вызова
				this->_callback.erase(j);
			// Выполняем удаление таймера
			this->_brokers.erase(i);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(tid), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод создания таймаута
 *
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::timeout(const uint32_t delay) noexcept {
	// Если данные переданы
	if(delay > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = (this->_brokers.empty() ? 1 : this->_brokers.rbegin()->first + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, std::make_unique <broker_t> (this->_fmk, this->_log));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем время задержки таймера
			ret.first->second->event.timeout(delay);
			// Устанавливаем базу данных событий
			ret.first->second->event = this->base();
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
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return 0;
}
/**
 * @brief Метод создания интервала
 *
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
uint16_t awh::Timer::interval(const uint32_t delay) noexcept {
	// Если данные переданы
	if(delay > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			this->_mtx.lock();
			// Получаем идентификатор таймера
			const uint16_t tid = (this->_brokers.empty() ? 1 : this->_brokers.rbegin()->first + 1);
			// Создаём объект таймера
			auto ret = this->_brokers.emplace(tid, std::make_unique <broker_t> (this->_fmk, this->_log));
			// Выполняем разблокировку потока
			this->_mtx.unlock();
			// Устанавливаем флаг персистентной работы
			ret.first->second->persist = true;
			// Устанавливаем время задержки таймера
			ret.first->second->event.timeout(delay, true);
			// Устанавливаем базу данных событий
			ret.first->second->event = this->base();
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
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return 0;
}
/**
 * @brief Деструктор
 *
 */
awh::Timer::~Timer() noexcept {
	// Выполняем удаление всех таймеров
	this->clear();
}
