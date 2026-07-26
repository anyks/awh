/**
 * @file: timer.cpp
 * @date: 2026-02-22
 * @license: LicenseRef-AWH-1.0
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
 * Подключаем заголовочный файл проекта
 */
#include <units/timer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод обновления состояния таймера
 *
 * @param eid    идентификатор таймера
 * @param status новый статус таймера
 */
void awh::unit::Timer::state(const event::id_t eid, const event::status_t status) noexcept {
	/**
	 * Определяем статус события таймера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если таймер сработал успешно
		case static_cast <uint8_t> (event::status_t::SUCCESS): {
			// Если функция обратного вызова установлена
			if(this->_callback.is(eid)){
				// Получаем функцию обратного вызова для события таймера
				auto callback = ::move(this->_callback.get <void (const event::id_t)> (eid));
				// Если функция обратного вызова получена успешно
				if(callback != nullptr)
					// Выполняем функцию обратного вызова
					callback(eid);
			}
			// Если узел таймера является таймаутом
			if(this->_io->node(eid) == event::node_t::TIMEOUT)
				// Удаляем таймер из списка активных таймеров
				this->clear(eid);
		} break;
		// Если таймер завершился аварийно, был отменён или уничтожен движком
		case static_cast <uint8_t> (event::status_t::FAILURE):
		case static_cast <uint8_t> (event::status_t::CANCELLED):
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Удаляем функцию обратного вызова события таймера
			this->_callback.erase(eid);
			// Снимаем идентификатор таймера с учёта активных таймеров
			this->_timers.erase(eid);
		} break;
	}
}
/**
 * @brief Метод очистки всех таймеров
 *
 */
void awh::unit::Timer::clear() noexcept {
	/**
	 * Переходим по всему списку активных таймеров
	 */
	for(auto & eid : this->_timers){
		// Удаляем функцию обратного вызова для события таймера
		this->_callback.erase(eid);
		// Снимаем функцию обратного вызова на событие таймера
		this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
		// Удаляем событие таймера
		this->_io->destroy(eid);
	}
	// Очищаем список активных таймеров
	this->_timers.clear();
}
/**
 * @brief Метод очистки таймера
 *
 * @param eid идентификатор таймера для очистки
 */
void awh::unit::Timer::clear(const event::id_t eid) noexcept {
	// Удаляем функцию обратного вызова для события таймера
	this->_callback.erase(eid);
	// Снимаем функцию обратного вызова на событие таймера
	this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
	// Удаляем событие таймера
	this->_io->destroy(eid);
	// Удаляем идентификатор таймера из списка активных таймеров
	this->_timers.erase(eid);
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
	// Если событие таймера успешно создано
	if(result > 0){
		// Добавляем новое событие таймера
		this->_io->setTimeout(result, event::action_t::NONE, delay);
		// Выполняем фиксацию настроек события таймера
		if(this->_io->commit(result)){
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&unit::timer_t::state, this, _1, _2)));
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
					// Записываем ошибку в лог запуска события
					this->_log->debug("Timer event could not be launched", __PRETTY_FUNCTION__, make_tuple(delay), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог запуска события
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
				// Записываем ошибку в лог создания события
				this->_log->debug("Timer event could not be created", __PRETTY_FUNCTION__, make_tuple(delay), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог создания события
				this->_log->print("Timer event could not be created", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
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
	// Если событие таймера успешно создано
	if(result > 0){
		// Добавляем новое событие таймера
		this->_io->setTimeout(result, event::action_t::NONE, delay);
		// Выполняем фиксацию настроек события таймера
		if(this->_io->commit(result)){
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&unit::timer_t::state, this, _1, _2)));
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
					// Записываем ошибку в лог запуска события
					this->_log->debug("Timer event could not be launched", __PRETTY_FUNCTION__, make_tuple(delay), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог запуска события
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
				// Записываем ошибку в лог создания события
				this->_log->debug("Timer event could not be created", __PRETTY_FUNCTION__, make_tuple(delay), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог создания события
				this->_log->print("Timer event could not be created", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Timer::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	/**
	 * Переходим по всему списку активных таймеров
	 */
	for(auto & eid : this->_timers)
		// Устанавливаем функцию обратного вызова для события таймера
		this->_callback.set(eid, callback);
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
