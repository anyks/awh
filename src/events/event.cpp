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
	const lock_guard <std::mutex> lock(this->_mtx);
	// Устанавливаем базу данных событий
	this->_base = base;
}
/**
 * set Метод установки файлового дескриптора
 * @param sock файловый дескриптор для установки
 */
void awh::Event::set(const SOCKET sock) noexcept {
	// Получаем флаг перезапуска работы события
	const bool restart = (this->_mode && (sock != INVALID_SOCKET) && (this->_sock != INVALID_SOCKET) && (sock != this->_sock));
	// Если необходимо выполнить перезапуск события
	if(restart)
		// Выполняем остановку работы события
		this->stop();
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT): {
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Устанавливаем файловый дескриптор
			this->_sock = sock;
		} break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Socket for a event timeout cannot be set", log_t::flag_t::WARNING);
		break;
	}
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
	const lock_guard <std::mutex> lock(this->_mtx);
	// Устанавливаем функцию обратного вызова
	this->_callback = callback;
}
/**
 * Метод удаления типа события
 * @param type тип события для удаления
 */
void awh::Event::del(const base_t::event_type_t type) noexcept {
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_sock != INVALID_SOCKET) || (this->_delay > 0)){
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выполняем удаление события
			this->_base->del(this->_id, this->_sock, type);
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * timeout Метод установки задержки времени таймера
 * @param delay  задержка времени в миллисекундах
 * @param series флаг серийного таймаута
 */
void awh::Event::timeout(const uint32_t delay, const bool series) noexcept {
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER): {
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
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
	// Если работа базы событий активированна
	if((this->_base != nullptr) && ((this->_sock != INVALID_SOCKET) || (this->_delay > 0))){
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем установку режима работы модуля
		return this->_base->mode(this->_id, this->_sock, type, mode);
	}
	// Сообщаем, что режим работы не установлен
	return false;
}
/**
 * stop Метод остановки работы события
 */
void awh::Event::stop() noexcept {
	// Если работа события запущена
	if((this->_base != nullptr) && this->_mode){
		// Если событие является стандартным
		if((this->_sock != INVALID_SOCKET) || (this->_delay > 0)){
			// Снимаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выполняем удаление всех событий
			this->_base->del(this->_id, this->_sock);
		// Выводим сообщение об ошибке
		} else this->_log->print("File descriptor is not init", log_t::flag_t::WARNING);
	}
}
/**
 * start Метод запуска работы события
 */
void awh::Event::start() noexcept {
	// Если работа события ещё не запущена
	if(!this->_mode && (this->_base != nullptr)){
		// Если событие является стандартным
		if((this->_sock != INVALID_SOCKET) || (this->_delay > 0)){
			// Устанавливаем флаг запущенной работы
			this->_mode = !this->_mode;
			// Выполняем генерацию идентификатора записи
			this->_id = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
			// Определяем тип установленного события
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип является обычным событием
				case static_cast <uint8_t> (type_t::EVENT): {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_sock, this->_callback))
						// Выводим сообщение что событие не вышло активировать
						this->_log->print("Failed activate event for SOCKET=%d", log_t::flag_t::WARNING, this->_sock);
				} break;
				// Если тип события является таймером
				case static_cast <uint8_t> (type_t::TIMER): {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем добавление события в базу событий
					if(!this->_base->add(this->_id, this->_sock, this->_callback, this->_delay, this->_series))
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
 * @param sock файловый дескриптор для установки
 * @return     текущий объект
 */
awh::Event & awh::Event::operator = (const SOCKET sock) noexcept {
	// Выполняем установку файлового дескриптора
	this->set(sock);
	// Возвращаем текущий объект
	return (* this);
}
/**
 * Оператор [=] для установки задержки времени таймера
 * @param delay задержка времени в миллисекундах
 * @return      текущий объект
 */
awh::Event & awh::Event::operator = (const uint32_t delay) noexcept {
	// Определяем тип установленного события
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип является обычным событием
		case static_cast <uint8_t> (type_t::EVENT):
			// Выводим сообщение что событие не вышло активировать
			this->_log->print("Timeout for event type cannot be set", log_t::flag_t::WARNING);
		break;
		// Если тип события является таймером
		case static_cast <uint8_t> (type_t::TIMER): {
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Устанавливаем задержки времени в миллисекундах
			this->_delay = delay;
		} break;
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
 * Event Конструктор
 * @param type тип события
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::Event::Event(const type_t type, const fmk_t * fmk, const log_t * log) noexcept :
 _series(false), _sock(INVALID_SOCKET), _id(0), _type(type), _delay(0),
 _mode(false), _callback(nullptr), _base(nullptr), _fmk(fmk), _log(log) {}
/**
 * ~Event Деструктор
 */
awh::Event::~Event() noexcept {
	// Выполняем остановку работы события
	this->stop();
}
