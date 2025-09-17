/**
 * @file: event.cpp
 * @date: 2025-09-16
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
#include <events/event.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * type Метод получения типа события
 * @return установленный тип события
 */
awh::Event::type_t awh::Event::type() const noexcept {
	// Выводим тип установленного события
	return this->_type;
}
/**
 * set Метод установки базы событий
 * @param base база событий для установки
 */
void awh::Event::set(base_t * base) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Устанавливаем базу данных событий
	this->_base = base;
}
/**
 * set Метод установки файлового дескриптора
 * @param fd файловый дескриптор для установки
 */
void awh::Event::set(const SOCKET fd) noexcept {
	// Получаем флаг перезапуска работы события
	const bool restart = (this->_mode && (fd != INVALID_SOCKET) && (this->_fd != INVALID_SOCKET) && (fd != this->_fd));
	// Если необходимо выполнить перезапуск события
	if(restart)
		// Выполняем остановку работы события
		this->stop();
	// Выполняем блокировку потока
	this->_mtx.lock();
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Устанавливаем файловый дескриптор
			this->_fd = fd;
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Socket for a event timeout cannot be set", log_t::flag_t::WARNING);
		break;
	}
	// Выполняем разблокировку потока
	this->_mtx.unlock();
	// Если необходимо выполнить перезапуск события
	if(restart)
		// Выполняем запуск работы события
		this->start();
}
/**
 * set Метод установки функции обратного вызова
 * @param callback функция обратного вызова
 */
void awh::Event::set(base_t::callback_t callback) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Устанавливаем функцию обратного вызова
	this->_callback = callback;
}
/**
 * Метод удаления типа события
 * @param type тип события для удаления
 */
void awh::Event::del(const base_t::event_type_t type) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0))
			// Выполняем удаление события
			this->_base->del(this->_id, this->_fd, type);
		// Выводим сообщение об ошибке
		else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * timeout Метод установки задержки времени таймера
 * @param delay  задержка времени в миллисекундах
 * @param series флаг серийного таймаута
 */
void awh::Event::timeout(const uint32_t delay, const bool series) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER): {
			// Устанавливаем задержки времени в миллисекундах
			this->_delay = delay;
			// Выполняем установку флага серийности таймера
			this->_series = series;
		} break;
	}
}
/**
 * mode Метод установки режима работы модуля
 * @param type тип событий модуля для которого требуется сменить режим работы
 * @param mode флаг режима работы модуля
 * @return     результат работы функции
 */
bool awh::Event::mode(const base_t::event_type_t type, const base_t::event_mode_t mode) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если работа базы событий активированна
	if((this->_base != nullptr) && ((this->_fd != INVALID_SOCKET) || (this->_delay > 0)))
		// Выполняем установку режима работы модуля
		return this->_base->mode(this->_id, this->_fd, type, mode);
	// Сообщаем, что режим работы не установлен
	return false;
}
/**
 * stop Метод остановки работы события
 */
void awh::Event::stop() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Снимаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем удаление всех событий
			this->_base->del(this->_id, this->_fd);
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * start Метод запуска работы события
 */
void awh::Event::start() noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Если работа события ещё не запущена
	if(!this->_mode && (this->_base != nullptr)){
		// Если событие является стандартным
		if((this->_fd != INVALID_SOCKET) || (this->_delay > 0)){
			// Устанавливаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем генерацию идентификатора записи
			this->_id = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
			// Определяем тип установленного события
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип является обычным событием
				case static_cast <uint8_t> (type_t::EVENT): {
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_fd, this->_callback))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, this->_fd);
				} break;
				// Если тип события является таймером
				case static_cast <uint8_t> (type_t::TIMER): {
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_fd, this->_callback, this->_delay, this->_series))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate timer", log_t::flag_t::WARNING);
				} break;
			}
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * Оператор [=] для установки базы событий
 * @param base база событий для установки
 * @return     текущий объект
 */
awh::Event & awh::Event::operator = (base_t * base) noexcept {
	// Выполняем установку базы событий
	this->set(base);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки файлового дескриптора
 * @param fd файловый дескриптор для установки
 * @return   текущий объект
 */
awh::Event & awh::Event::operator = (const SOCKET fd) noexcept {
	// Выполняем установку файлового дескриптора
	this->set(fd);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки задержки времени таймера
 * @param delay задержка времени в миллисекундах
 * @return      текущий объект
 */
awh::Event & awh::Event::operator = (const uint32_t delay) noexcept {
	// Выполняем блокировку потока
	const lock_guard <std::recursive_mutex> lock(this->_mtx);
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER):
			// Устанавливаем задержки времени в миллисекундах
			this->_delay = delay;
		break;
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки функции обратного вызова
 * @param callback функция обратного вызова
 * @return         текущий объект
 */
awh::Event & awh::Event::operator = (base_t::callback_t callback) noexcept {
	// Выполняем установку функции обратного вызова
	this->set(callback);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * ~Event Деструктор
 */
awh::Event::~Event() noexcept {
	// Выполняем остановку работы события
	this->stop();
}
