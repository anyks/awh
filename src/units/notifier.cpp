/**
 * @file: notifier.cpp
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
#include <units/notifier.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод обработки событий записи сообщений уведомителя
 *
 * @param eid  идентификатор события
 * @param size размер сообщения
 */
void awh::unit::Notifier::write(const event::id_t eid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const size_t)> ("trigger", eid, size);
}
/**
 * @brief Метод обработки событий чтения сообщений уведомителя
 *
 * @param eid  идентификатор события
 * @param data данные сообщения
 * @param size размер сообщения
 */
void awh::unit::Notifier::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("notify", eid, data, size);
}
/**
 * @brief Метод обработки состояния уведомителя
 *
 * @param eid    идентификатор события
 * @param status статус события
 */
void awh::unit::Notifier::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если статус уведомителя представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Если в списке событий уведомителя есть события
		if(!this->_events.empty()){
			// Выполняем поиск идентификатора события уведомителя в списке событий уведомителя
			auto i = this->_events.find(eid);
			// Если идентификатор события уведомителя найден в списке событий уведомителя
			if(i != this->_events.end())
				// Удаляем идентификатор события уведомителя из списка событий уведомителя
				this->_events.erase(i);
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки исключений событий уведомителя
 *
 * @param eid     идентификатор события
 * @param error   тип ошибки
 * @param message сообщение об ошибке
 */
void awh::unit::Notifier::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, message);
}
/**
 * @brief Метод обработки событий доступного размера очереди события уведомителя
 *
 * @param eid    идентификатор события
 * @param status статус события
 * @param size   доступный размер очереди в байтах
 */
void awh::unit::Notifier::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t, const size_t)> ("available", eid, status, size);
}
/**
 * @brief Метод создания события уведомителя
 *
 * @return идентификатор события уведомителя
 */
awh::event::id_t awh::unit::Notifier::create() noexcept {
	// Выполняем создание события уведомителя
	event::id_t result = this->_io->event(event::node_t::NOTIFY, event::family_t::USER);
	// Если событие уведомителя успешно создано
	if(result > 0){
		// Выполняем фиксацию настроек события сервера
		if(this->_io->commit(result)){
			// Устанавливаем функцию обратного вызова на событие записи сообщений
			this->_io->on(result, static_cast <engine::callback::write_t> (std::bind(&notifier_t::write, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие чтения сообщений
			this->_io->on(result, static_cast <engine::callback::read_t> (std::bind(&notifier_t::read, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие изменения состояния
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&notifier_t::state, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&notifier_t::error, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие доступности очереди сообщений
			this->_io->on(result, static_cast <engine::callback::available_t> (std::bind(&notifier_t::available, this, _1, _2, _3)));
			// Запускаем работу события уведомителя
			if(!this->_io->launch(result)){
				// Удаляем событие уведомителя
				this->_io->destroy(result);
				// Обнуляем результат
				result = 0;
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог запуска события
					this->_log->debug("Notifier event could not be launched", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог запуска события
					this->_log->print("Notifier event could not be launched", log_t::flag_t::WARNING);
				#endif
			// Добавляем идентификатор события уведомителя в список событий уведомителя
			} else this->_events.emplace(result);
		// Если событие уведомителя не может быть создано
		} else {
			// Удаляем событие уведомителя
			this->_io->destroy(result);
			// Обнуляем результат
			result = 0;
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог создания события
				this->_log->debug("Notifier event could not be created", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог создания события
				this->_log->print("Notifier event could not be created", log_t::flag_t::WARNING);
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод уничтожения события уведомителя
 *
 * @param eid идентификатор события уведомителя
 */
void awh::unit::Notifier::destroy(const event::id_t eid) noexcept {
	// Удаляем событие уведомителя
	this->_io->destroy(eid);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Notifier::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при изменении состояния события уведомителя
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при получении сообщения события уведомителя
	this->_callback.set("notify", callback);
	// Выполняем установку функции обратного вызова при триггере события уведомителя
	this->_callback.set("trigger", callback);
	// Выполняем установку функции обратного вызова при доступности очереди сообщений события уведомителя
	this->_callback.set("available", callback);
}
/**
 * @brief Метод триггера события уведомителя
 *
 * @param eid    идентификатор события уведомителя
 * @param buffer буфер данных для отправки
 * @param size   размер буфера данных
 * @return       количество отправленных байт
 */
size_t awh::unit::Notifier::trigger(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Триггерим событие уведомителя
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Notifier::Notifier(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Notifier::~Notifier() noexcept {
	// Если в списке событий уведомителя есть события
	if(!this->_events.empty()){
		// Копируем список событий уведомителя для безопасного удаления событий уведомителя во время итерации
		auto events = this->_events;
		/**
		 * Выполняем удаление всех событий уведомителя
		 */
		for(const auto & eid : events){
			// Снимаем функцию обратного вызова на событие чтения сообщений
			this->_io->on(eid, static_cast <engine::callback::read_t> (nullptr));
			// Снимаем функцию обратного вызова на событие записи сообщений
			this->_io->on(eid, static_cast <engine::callback::write_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(eid, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения состояния
			this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
			// Снимаем функцию обратного вызова на событие доступности очереди сообщений
			this->_io->on(eid, static_cast <engine::callback::available_t> (nullptr));
			// Удаляем событие уведомителя
			this->_io->destroy(eid);
		}
	}
}
