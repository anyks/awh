/**
 * @file: events.cpp
 * @date: 2025-10-18
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
#include <events/base.hpp>
#include <events/events.hpp>

#include <iostream>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки находятся ли событии в ожидании
 * 
 * @return результат проверки
 */
bool awh::Events::awaiting() const noexcept {
	// Выводим результат
	return this->_awaiting;
}
/**
 * @brief Метод получения типов установленных соыбтий
 *
 * @return установленные типы событий
 */
std::unordered_set <awh::Events::type_t> awh::Events::types() const noexcept {
	// Результат работы функции
	std::unordered_set <type_t> result;
	// Если активированно событие таймера
	if(this->_events & static_cast <uint16_t> (react_t::AWH_TIMER))
		// Устанавливаем тип активного события как таймер
		result.emplace(type_t::TIMER);
	// Если активированно событие интервала
	else if(this->_events & static_cast <uint16_t> (react_t::AWH_INTERVAL))
		// Устанавливаем тип активного события как интервал
		result.emplace(type_t::INTERVAL);
	// Если активированны другие события
	else {
		// Если активированно событие ожидания готовности на получение данных
		if(this->_events & static_cast <uint16_t> (react_t::AWH_READ))
			// Устанавливаем тип активного события как чтение
			result.emplace(type_t::READ);
		// Если активированно событие ожидания готовности на отправку данных
		if(this->_events & static_cast <uint16_t> (react_t::AWH_WRITE))
			// Устанавливаем тип активного события как запись
			result.emplace(type_t::WRITE);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки базы событий
 *
 * @param base база событий для установки
 * @return     результат выполнения установки
 */
bool awh::Events::set(base_t * base) noexcept {
	// Результат работы функции
	bool result = false;
	// Если сокет уже был добавлен в базу событий для отслеживания событий
	if(!(result = !this->_awaiting)){
		// Удаляем активное событие из базы событий
		if((result = this->_base->erase(this->_id))){
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Устанавливаем базу данных событий
			this->_base = base;
			// Если события уже установлены
			if((this->_events & static_cast <uint16_t> (react_t::AWH_READ)) ||
			   (this->_events & static_cast <uint16_t> (react_t::AWH_WRITE))){
				// Выполняем добавление нового сокета в базу событий
				this->_id = this->_base->emplace(this->_sock, this->_callback);
				// Добавляем события на отслеживание
				return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
			// Если событиями является таймер или интервал
			} else if((this->_events & static_cast <uint16_t> (react_t::AWH_TIMER)) ||
			          (this->_events & static_cast <uint16_t> (react_t::AWH_INTERVAL))) {
				// Выполняем добавление нового сокета в базу событий
				this->_id = this->_base->emplace(INVALID_SOCKET, this->_callback, this->_delay);
				// Добавляем события на отслеживание
				return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
			// Снимаем флаг отслеживания событий
			} else this->_awaiting = false;
		// Если событие не удалось удалить из базы событий
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Unable to connect a new eventbase because the current eventbase is currently active", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Unable to connect a new eventbase because the current eventbase is currently active", log_t::flag_t::WARNING);
			#endif
		}
	// Если сокет можно безопасно заменить
	} else {
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Устанавливаем базу данных событий
		this->_base = base;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки файлового дескриптора
 *
 * @param sock сетевой сокет для установки
 * @return     результат выполнения установки
 */
bool awh::Events::set(const SOCKET sock) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если сокет уже был добавлен в базу событий для отслеживания событий
		if(!(result = !this->_awaiting)){
			// Удаляем активное событие из базы событий
			if((result = this->_base->erase(this->_id))){
				// Выполняем блокировку потока
				const lock_guard lock(this->_mtx);
				// Выполняем удаление задержки времени
				this->_delay = 0;
				// Устанавливаем сетевой сокет
				this->_sock = sock;
				// Выполняем добавление нового сокета в базу событий
				this->_id = this->_base->emplace(this->_sock, this->_callback);
				// Если события уже установлены
				if((this->_events & static_cast <uint16_t> (react_t::AWH_READ)) ||
				   (this->_events & static_cast <uint16_t> (react_t::AWH_WRITE)))
					// Добавляем события на отслеживание
					return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
				// Если события установлены как таймер или интервал
				else {
					// Просто сбрасываем ранее установленные события
					this->_events = static_cast <uint16_t> (react_t::AWH_NONE);
					// Сбрасываем флаг отслеживания событий
					this->_awaiting = !this->_awaiting;
				}
			// Если событие не удалось удалить из базы событий
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to establish new socket [%d], please stop event tracking subscription first", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::WARNING, sock);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unable to establish new socket [%d], please stop event tracking subscription first", log_t::flag_t::WARNING, sock);
				#endif
			}
		// Если сокет можно безопасно заменить
		} else {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Сбрасываем ранее сгенерированный идентификатор
			this->_id = 0;
			// Устанавливаем сетевой сокет
			this->_sock = sock;
			// Если был установлен таймер или интервал
			if((this->_events & static_cast <uint16_t> (react_t::AWH_TIMER)) ||
			   (this->_events & static_cast <uint16_t> (react_t::AWH_INTERVAL))){
				// Выполняем удаление задержки времени
				this->_delay = 0;
				// Сбрасываем ранее установленные события
				this->_events = static_cast <uint16_t> (react_t::AWH_NONE);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки функции обратного вызова
 *
 * @param callback функция обратного вызова
 * @return         результат выполнения установки
 */
bool awh::Events::set(callback_t callback) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если сокет уже был добавлен в базу событий для отслеживания событий
		if(!(result = !this->_awaiting)){
			// Удаляем активное событие из базы событий
			if((result = this->_base->erase(this->_id))){
				// Выполняем блокировку потока
				const lock_guard lock(this->_mtx);
				// Устанавливаем функцию обратного вызова
				this->_callback = ::move(callback);
				// Если события уже установлены
				if((this->_events & static_cast <uint16_t> (react_t::AWH_READ)) ||
				   (this->_events & static_cast <uint16_t> (react_t::AWH_WRITE))){
					// Выполняем добавление нового сокета в базу событий
					this->_id = this->_base->emplace(this->_sock, this->_callback);
					// Добавляем события на отслеживание
					return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
				// Если событиями является таймер или интервал
				} else if((this->_events & static_cast <uint16_t> (react_t::AWH_TIMER)) ||
				          (this->_events & static_cast <uint16_t> (react_t::AWH_INTERVAL))) {
					// Выполняем добавление нового сокета в базу событий
					this->_id = this->_base->emplace(INVALID_SOCKET, this->_callback, this->_delay);
					// Добавляем события на отслеживание
					return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
				// Снимаем флаг отслеживания событий
				} else this->_awaiting = false;
			// Если событие не удалось удалить из базы событий
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to set new callback, please stop event tracking subscription first", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unable to set new callback, please stop event tracking subscription first", log_t::flag_t::WARNING);
				#endif
			}
		// Если сокет можно безопасно заменить
		} else {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Устанавливаем функцию обратного вызова
			this->_callback = ::move(callback);
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод для установки задержки времени таймера
 *
 * @param delay задержка времени в миллисекундах
 * @return      результат выполнения установки
 */
bool awh::Events::set(const uint32_t delay) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если сокет уже был добавлен в базу событий для отслеживания событий
		if(!(result = !this->_awaiting)){
			// Удаляем активное событие из базы событий
			if((result = this->_base->erase(this->_id))){
				// Выполняем блокировку потока
				const lock_guard lock(this->_mtx);
				// Устанавливаем задержку времени в миллисекундах
				this->_delay = delay;
				// Удаляем сетевой сокет
				this->_sock = INVALID_SOCKET;
				// Выполняем добавление нового таймера в базу событий
				this->_id = this->_base->emplace(this->_sock, this->_callback, this->_delay);
				// Если события уже установлены
				if((this->_events & static_cast <uint16_t> (react_t::AWH_TIMER)) ||
				   (this->_events & static_cast <uint16_t> (react_t::AWH_INTERVAL)))
					// Добавляем события на отслеживание
					return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
				// Если события установлены как таймер или интервал
				else {
					// Просто сбрасываем ранее установленные события
					this->_events = static_cast <uint16_t> (react_t::AWH_NONE);
					// Сбрасываем флаг отслеживания событий
					this->_awaiting = !this->_awaiting;
				}
			// Если событие не удалось удалить из базы событий
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unable to set new delay timeout, please stop event tracking subscription first", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unable to set new delay timeout, please stop event tracking subscription first", log_t::flag_t::WARNING);
				#endif
			}
		// Если сокет можно безопасно заменить
		} else {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Устанавливаем задержку времени в миллисекундах
			this->_delay = delay;
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки режима работы модуля
 *
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Events::mode(const type_t type, const mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard lock(this->_mtx);
	/**
	 * Определяем тип события для установки
	 */
	switch(static_cast <uint8_t> (type)){
		// Если тип события является ожиданием готовности на чтение
		case static_cast <uint8_t> (type_t::READ): {
			/**
			 * Определяем флаг режима работы
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать событие
				case static_cast <uint8_t> (mode_t::ENABLED):
					// Активируем событие ожидания готовности на чтение
					this->_events |= static_cast <uint16_t> (react_t::AWH_READ);
				break;
				// Если необходимо деактивировать событие
				case static_cast <uint8_t> (mode_t::DISABLED):
					// Деактивируем событие ожидания готовности на чтение
					this->_events ^= static_cast <uint16_t> (react_t::AWH_READ);
				break;
			}
		} break;
		// Если тип события является ожиданием готовности на запись
		case static_cast <uint8_t> (type_t::WRITE): {
			/**
			 * Определяем флаг режима работы
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать событие
				case static_cast <uint8_t> (mode_t::ENABLED):
					// Активируем событие ожидания готовности на запись
					this->_events |= static_cast <uint16_t> (react_t::AWH_WRITE);
				break;
				// Если необходимо деактивировать событие
				case static_cast <uint8_t> (mode_t::DISABLED):
					// Деактивируем событие ожидания готовности на запись
					this->_events ^= static_cast <uint16_t> (react_t::AWH_WRITE);
				break;
			}
		} break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER): {
			/**
			 * Определяем флаг режима работы
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать событие
				case static_cast <uint8_t> (mode_t::ENABLED):
					// Устанавливаем событие таймера
					this->_events |= static_cast <uint16_t> (react_t::AWH_TIMER);
				break;
				// Если необходимо деактивировать событие
				case static_cast <uint8_t> (mode_t::DISABLED):
					// Снимаем событие таймера
					this->_events ^= static_cast <uint16_t> (react_t::AWH_TIMER);
				break;
			}
		} break;
		// Если тип события является интервалом
		case static_cast <uint8_t> (type_t::INTERVAL): {
			/**
			 * Определяем флаг режима работы
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо активировать событие
				case static_cast <uint8_t> (mode_t::ENABLED):
					// Устанавливаем событие интервала
					this->_events |= static_cast <uint16_t> (react_t::AWH_INTERVAL);
				break;
				// Если необходимо деактивировать событие
				case static_cast <uint8_t> (mode_t::DISABLED):
					// Снимаем событие интервала
					this->_events ^= static_cast <uint16_t> (react_t::AWH_INTERVAL);
				break;
			}
		} break;
	}
	// Если сокет уже был добавлен в базу событий для отслеживания событий
	if(this->_awaiting)
		// Добавляем события на отслеживание
		return this->_base->mode(this->_id, static_cast <uint8_t> (this->_events));
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод остановки работы события
 *
 * @return результат работы функции
 */
bool awh::Events::stop() noexcept {
	// Если сокет уже был добавлен в базу событий для отслеживания событий
	if(this->_awaiting){
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Выполняем остановку работы отслеживания событий
		return this->_base->erase(this->_id);
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод запуска работы события
 *
 * @return результат работы функции
 */
bool awh::Events::start() noexcept {
	// Если сокет ещё не был добавлен в базу событий для отслеживания событий
	if(!this->_awaiting && (this->_callback != nullptr)){
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Если сетевой сокет установлен
		if(this->_sock != INVALID_SOCKET){
			// Выполняем добавление нового сокета в базу событий
			this->_id = this->_base->emplace(this->_sock, this->_callback);
			// Добавляем события на отслеживание
			return (this->_awaiting = this->_base->mode(this->_id, static_cast <uint8_t> (this->_events)));
		// Если событиями является таймер или интервал
		} else if(this->_delay > 0) {
			// Выполняем добавление нового сокета в базу событий
			this->_id = this->_base->emplace(INVALID_SOCKET, this->_callback, this->_delay);
			// Добавляем события на отслеживание
			return (this->_awaiting = this->_base->mode(this->_id, static_cast <uint8_t> (this->_events)));
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод обмена данными между событиями
 * 
 * @param events объект событий для обмена
 */
void awh::Events::swap(Events && events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если текущее отслеживание событий сокета активно
		if(this->_awaiting)
			// Выполняем остановку работы
			this->_base->erase(this->_id);
		// Если отслеживание событий сокета активно у стороннего объекта событий
		if(events._awaiting)
			// Выполняем остановку работы
			events._base->erase(events._id);
		// Выполняем блокировку потока
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока
		const lock_guard lock2(events._mtx);
		// Выполняем обмен идентификаторами
		this->_id += (events._id - (events._id = this->_id));
		// Выполняем обмен сокетами
		this->_sock += (events._sock - (events._sock = this->_sock));
		// Выполняем обменом задержки времени таймера
		this->_delay += (events._delay - (events._delay = this->_delay));
		// Выполняем обмен событиями
		this->_events += (events._events - (events._events = this->_events));
		// Получаем текущее значение функции обратного вызова
		auto callback = ::move(this->_callback);
		// Устанавливаем новое значение функции обратного вызова
		this->_callback = ::move(events._callback);
		// Устанавливаем текущее значение функции обратного вызова
		events._callback = ::move(callback);
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
 * @brief Оператор для установки базы событий
 *
 * @param base база событий для установки
 * @return     текущий объект
 */
awh::Events & awh::Events::operator = (base_t * base) noexcept {
	// Выполняем установку базы событий
	this->set(base);
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор для установки файлового дескриптора
 *
 * @param sock сетевой сокет для установки
 * @return     текущий объект
 */
awh::Events & awh::Events::operator = (const SOCKET sock) noexcept {
	// Выполняем установку сокета
	this->set(sock);
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор для установки задержки времени таймера
 *
 * @param delay задержка времени в миллисекундах
 * @return      текущий объект
 */
awh::Events & awh::Events::operator = (const uint32_t delay) noexcept {
	// Выполняем установку времени задержки таймера
	this->set(delay);
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор для установки функции обратного вызова
 *
 * @param callback функция обратного вызова
 * @return         текущий объект
 */
awh::Events & awh::Events::operator = (callback_t callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->set(::move(callback));
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор перемещения объекта событий
 * 
 * @param events объект событий для перемещения
 * @return       текущий объект
 */
awh::Events & awh::Events::operator = (Events && events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если текущее отслеживание событий сокета активно
		if(this->_awaiting)
			// Выполняем остановку работы
			this->_base->erase(this->_id);
		// Если отслеживание событий сокета активно у стороннего объекта событий
		if(events._awaiting)
			// Выполняем остановку работы
			events._base->erase(events._id);
		// Выполняем блокировку потока
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока
		const lock_guard lock2(events._mtx);
		// Выполняем установку идентификатора
		this->_id = events._id;
		// Очищаем изначальное значение идентификатора
		events._id = 0;
		// Выполняем установку сокета
		this->_sock = events._sock;
		// Очищаем изначальное значение идентификатора
		events._sock = INVALID_SOCKET;
		// Выполняем установку задержки времени таймера
		this->_delay = events._delay;
		// Очищаем изначальное значение времени таймера
		events._delay = 0;
		// Выполняем установку событий
		this->_events = events._events;
		// Очищаем изначальное значение событий
		events._events = 0;
		// Устанавливаем новое значение функции обратного вызова
		this->_callback = ::move(events._callback);
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
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор копирования объекта событий
 * 
 * @param events объект событий для копирования
 * @return       текущий объект
 */
awh::Events & awh::Events::operator = (const Events & events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если текущее отслеживание событий сокета активно
		if(this->_awaiting)
			// Выполняем остановку работы
			this->_base->erase(this->_id);
		// Если отслеживание событий сокета активно у стороннего объекта событий
		if(events._awaiting)
			// Выполняем остановку работы
			events._base->erase(events._id);
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Выполняем установку идентификатора
		this->_id = events._id;
		// Выполняем установку сокета
		this->_sock = events._sock;
		// Выполняем установку задержки времени таймера
		this->_delay = events._delay;
		// Выполняем установку событий
		this->_events = events._events;
		// Устанавливаем новое значение функции обратного вызова
		this->_callback = events._callback;
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
	// Выводим текущее значение объекта
	return (* this);
}
/**
 * @brief Конструктор перемещения
 * 
 * @param events объект событий для перемещения
 */
awh::Events::Events(Events && events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если текущее отслеживание событий сокета активно
		if(this->_awaiting)
			// Выполняем остановку работы
			this->_base->erase(this->_id);
		// Если отслеживание событий сокета активно у стороннего объекта событий
		if(events._awaiting)
			// Выполняем остановку работы
			events._base->erase(events._id);
		// Выполняем блокировку потока
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока
		const lock_guard lock2(events._mtx);
		// Выполняем установку идентификатора
		this->_id = events._id;
		// Очищаем изначальное значение идентификатора
		events._id = 0;
		// Выполняем установку сокета
		this->_sock = events._sock;
		// Очищаем изначальное значение идентификатора
		events._sock = INVALID_SOCKET;
		// Выполняем установку задержки времени таймера
		this->_delay = events._delay;
		// Очищаем изначальное значение времени таймера
		events._delay = 0;
		// Выполняем установку событий
		this->_events = events._events;
		// Очищаем изначальное значение событий
		events._events = 0;
		// Устанавливаем новое значение функции обратного вызова
		this->_callback = ::move(events._callback);
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
 * @brief Конструктор копирования
 * 
 * @param events объект событий для копирования
 */
awh::Events::Events(const Events & events) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Если текущее отслеживание событий сокета активно
		if(this->_awaiting)
			// Выполняем остановку работы
			this->_base->erase(this->_id);
		// Если отслеживание событий сокета активно у стороннего объекта событий
		if(events._awaiting)
			// Выполняем остановку работы
			events._base->erase(events._id);
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Выполняем установку идентификатора
		this->_id = events._id;
		// Выполняем установку сокета
		this->_sock = events._sock;
		// Выполняем установку задержки времени таймера
		this->_delay = events._delay;
		// Выполняем установку событий
		this->_events = events._events;
		// Устанавливаем новое значение функции обратного вызова
		this->_callback = events._callback;
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Events::Events(const fmk_t * fmk, const log_t * log) noexcept :
 _sock(INVALID_SOCKET), _id(0), _delay(0), _events(0),
 _awaiting(false), _callback(nullptr), _base(nullptr), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Events::~Events() noexcept {
	// Выполняем остановку работы событий
	this->stop();
}
