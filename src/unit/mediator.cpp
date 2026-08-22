/**
 * @file mediator.cpp
 * @date 2026-03-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация модуля посредника — связывание двух независимых узлов в двунаправленный канал и проксирование
 *        трафика между ними с контролем встречного давления
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/mediator.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод обработки действий посредника
 *
 * @param eid    идентификатор события
 * @param action действие посредника
 *
 */
void awh::unit::Mediator::action(const event::id_t eid, const event::action_t action) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
}
/**
 * @brief Метод обработки событий изменения статуса посредника
 *
 * @param eid    идентификатор события
 * @param status новый статус посредника
 *
 */
void awh::unit::Mediator::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если статус посредника представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Если в списке событий посредника есть события
		if(!this->_events.empty()){
			// Выполняем поиск идентификатора события посредника в списке событий посредника
			auto i = this->_events.find(eid);
			// Если идентификатор события посредника найден в списке событий посредника
			if(i != this->_events.end())
				// Удаляем идентификатор события посредника из списка событий посредника
				this->_events.erase(i);
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки событий получения данных посредником
 *
 * @param eid  идентификатор события
 * @param data данные события получения данных посредником
 * @param size размер данных события получения данных посредником
 *
 */
void awh::unit::Mediator::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, data, size);
}
/**
 * @brief Метод обработки событий ошибок посредника
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 *
 */
void awh::unit::Mediator::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод фиксации настроек посредника
 *
 * @param eid идентификатор события посредника
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::Mediator::commit(const event::id_t eid) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем фиксацию параметров события посредника
		if(!(result = this->_io->commit(eid))){
			// Удаляем событие посредника
			this->_io->destroy(eid);
			// Выполняем поиск идентификатора события посредника в списке событий посредника
			auto i = this->_events.find(eid);
			// Если идентификатор события посредника найден в списке событий посредника
			if(i != this->_events.end())
				// Удаляем идентификатор события посредника из списка событий посредника
				this->_events.erase(i);
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Failed to commit mediator", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Failed to commit mediator", log_t::flag_t::CRITICAL);
				#endif
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод отправки данных посредником
 *
 * @param eid    идентификатор события посредника
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных посредником
 *
 */
size_t awh::unit::Mediator::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Выполняем отправку данных посредником
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Метод перемещения данных между посредником и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения перемещения
 *
 */
bool awh::unit::Mediator::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Выполняем перемещение данных между событием посредника и другим событием
	return this->_io->splice(eid, dest);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события посредника
 * @return    адрес хоста целевой машины
 *
 */
string awh::unit::Mediator::getTarget(const event::id_t eid) const noexcept {
	// Выполняем получение адреса хоста целевой машины для события посредника
	return this->_io->getTarget(eid);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Mediator::setTarget(const event::id_t eid, string_view target) noexcept {
	// Выполняем установку адреса хоста целевой машины для события посредника
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Mediator::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Выполняем установку адреса хоста целевой машины для события посредника
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 *
 */
bool awh::unit::Mediator::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Выполняем получение адреса хоста целевой машины для события посредника
	return this->_io->getTarget(eid, target);
}
/**
 * @brief Метод получения адреса посредника
 *
 * @param eid     идентификатор события посредника
 * @param address тип адреса посредника
 * @return        значение адреса посредника
 *
 */
string awh::unit::Mediator::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Выполняем получение адреса посредника для события посредника
	return this->_io->getAddress(eid, address);
}
/**
 * @brief Метод установки адреса посредника
 *
 * @param eid     идентификатор события посредника
 * @param address тип адреса посредника
 * @param value   значение адреса посредника
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Mediator::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Выполняем установку адреса посредника для события посредника
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод установки адреса посредника
 *
 * @param eid     идентификатор события посредника
 * @param address тип адреса посредника
 * @param value   значение адреса посредника
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Mediator::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Выполняем установку адреса посредника для события посредника
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод получения адреса посредника
 *
 * @param eid     идентификатор события посредника
 * @param address тип адреса посредника
 * @param value   объект для извлечения адреса посредника
 * @return        результат выполнения извлечения адреса посредника
 *
 */
bool awh::unit::Mediator::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Выполняем получение адреса посредника для события посредника
	return this->_io->getAddress(eid, address, value);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::Mediator::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении данных из тоннеля
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при получении состояния посредника
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при обработке действий посредника
	this->_callback.set("action", callback);
}
/**
 * @brief Метод уничтожения события посредника
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::Mediator::destroy(const event::id_t eid) noexcept {
	// Удаляем событие посредника
	this->_io->destroy(eid);
}
/**
 * @brief Метод получения идентификатора посредника для перехвата пакетов тоннеля
 *
 * @param family семейство адресов
 * @return       идентификатор созданного посредника
 *
 */
awh::event::id_t awh::unit::Mediator::issue(const event::family_t family) noexcept {
	// Переменная результата
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Добавляем новое событие посредника для перехвата пакетов тоннеля
		result = this->_io->event(event::node_t::MEDIATOR, family);
		// Если событие посредника успешно создано
		if(result > 0){
			// Устанавливаем функцию обратного вызова на событие изменения действий посредника
			this->_io->on(result, static_cast <engine::callback::event_t> (std::bind(&mediator_t::action, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие чтения данных
			this->_io->on(result, static_cast <engine::callback::read_t> (std::bind(&mediator_t::read, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие изменения статуса посредника
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&mediator_t::status, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&mediator_t::error, this, _1, _2, _3)));
			// Добавляем идентификатор события посредника в список событий посредника
			this->_events.emplace(result);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Mediator::Mediator(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Mediator::~Mediator() noexcept {
	// Если в списке событий посредника есть события
	if(!this->_events.empty()){
		// Копируем список событий посредника для безопасного удаления событий посредника во время итерации
		auto events = this->_events;
		/**
		 * Выполняем удаление всех событий посредника
		 */
		for(const auto & eid : events){
			// Снимаем функцию обратного вызова на событие чтения данных
			this->_io->on(eid, static_cast <engine::callback::read_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(eid, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения действий посредника
			this->_io->on(eid, static_cast <engine::callback::event_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения статуса посредника
			this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
			// Удаляем событие посредника
			this->_io->destroy(eid);
		}
	}
}
