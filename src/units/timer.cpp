/**
 * @file: timer.cpp
 * @date: 2026-02-22
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <units/timer.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * @brief Метод обновления статуса таймера
 *
 * @param eid    идентификатор таймера
 * @param status новый статус таймера
 */
void awh::unit::Timer::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если таймер сработал успешно
	if(status == event::status_t::SUCCESS){
		// Если функция обратного вызова установлена
		if(this->_callback.is(eid)){
			// Получаем функцию обратного вызова для события таймера
			auto callback = ::move(this->_callback.get <void (const event::id_t)> (eid));
			// Выполняем функцию обратного вызова
			callback(eid);
		}
		// Если узел таймера является таймаутом
		if(this->_io->node(eid) == event::node_t::TIMEOUT)
			// Удаляем таймер из списка активных таймеров
			this->clear(eid);
	}
}
/**
 * @brief Метод очистки всех таймеров
 *
 */
void awh::unit::Timer::clear() noexcept {
	// Если список активных таймеров не пустой
	if(!this->_timers.empty()){
		// Переходим по всему списку активных таймеров
		for(auto & eid : this->_timers){
			// Если функция обратного вызова установлена
			if(this->_callback.is(eid))
				// Удаляем функцию обратного вызова для события таймера
				this->_callback.erase(eid);
			// Удаляем событие таймера
			this->_io->destroy(eid);
		}
		// Очищаем список активных таймеров
		this->_timers.clear();
	}
}
/**
 * @brief Метод очистки таймера
 *
 * @param eid идентификатор таймера для очистки
 */
void awh::unit::Timer::clear(const event::id_t eid) noexcept {
	// Выполняем поиск идентификатора таймера в списке активных таймеров
	auto i = this->_timers.find(eid);
	// Если указанный идентификатор таймера найден
	if(i != this->_timers.end()){
		// Если функция обратного вызова установлена
		if(this->_callback.is(eid))
			// Удаляем функцию обратного вызова для события таймера
			this->_callback.erase(eid);
		// Удаляем событие таймера
		this->_io->destroy(eid);
		// Удаляем идентификатор таймера из списка активных таймеров
		this->_timers.erase(i);
	}
}
/**
 * @brief Метод создания таймаута
 *
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
awh::event::id_t awh::unit::Timer::timeout(const uint32_t delay) noexcept {
	// Создаём новое событие таймера
	event::id_t result = this->_io->event(event::node_t::TIMEOUT, event::family_t::TIMER);
	// Добавляем новое событие таймера
	this->_io->setTimeout(result, event::action_t::NONE, delay);
	// Выполняем фиксацию настроек события таймера
	if(this->_io->commit(result)){
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(result, static_cast <event::callback::status_t> (std::bind(&unit::timer_t::status, this, _1, _2)));
		// Запускаем работу события таймера
		if(!this->_io->launch(result)){
			// Удаляем событие таймера
			this->_io->destroy(result);
			// Обнуляем результат
			result = 0;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке запуска события
				this->_log->debug("Timer event could not be launched", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке запуска события
				this->_log->print("Timer event could not be launched", log_t::flag_t::WARNING);
			#endif
		// Добавляем идентификатор таймера в список активных таймеров
		} else this->_timers.emplace(result);
	// Если событие таймера не может быть запущено
	} else {
		// Удаляем событие таймера
		this->_io->destroy(result);
		// Обнуляем результат
		result = 0;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке создания события
			this->_log->debug("Timer event could not be created", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке создания события
			this->_log->print("Timer event could not be created", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод создания интервала
 *
 * @param delay задержка времени в миллисекундах
 * @return      идентификатор таймера
 */
awh::event::id_t awh::unit::Timer::interval(const uint32_t delay) noexcept {
	// Создаём новое событие таймера
	event::id_t result = this->_io->event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Добавляем новое событие таймера
	this->_io->setTimeout(result, event::action_t::NONE, delay);
	// Выполняем фиксацию настроек события таймера
	if(this->_io->commit(result)){
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(result, static_cast <event::callback::status_t> (std::bind(&unit::timer_t::status, this, _1, _2)));
		// Запускаем работу события таймера
		if(!this->_io->launch(result)){
			// Удаляем событие таймера
			this->_io->destroy(result);
			// Обнуляем результат
			result = 0;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке запуска события
				this->_log->debug("Timer event could not be launched", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке запуска события
				this->_log->print("Timer event could not be launched", log_t::flag_t::WARNING);
			#endif
		// Добавляем идентификатор таймера в список активных таймеров
		} else this->_timers.emplace(result);
	// Если событие таймера не может быть запущено
	} else {
		// Удаляем событие таймера
		this->_io->destroy(result);
		// Обнуляем результат
		result = 0;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке создания события
			this->_log->debug("Timer event could not be created", __PRETTY_FUNCTION__, std::make_tuple(delay), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке создания события
			this->_log->print("Timer event could not be created", log_t::flag_t::WARNING);
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Timer::Timer(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Timer::~Timer() noexcept {
	// Выполняем очистку всех таймеров
	this->clear();
}
