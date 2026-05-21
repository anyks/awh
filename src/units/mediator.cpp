/**
 * @file: mediator.cpp
 * @date: 2026-03-23
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
 * Подключаем заголовочный файл модуля
 */
#include <units/mediator.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * @brief Метод обработки действий посредника
 *
 * @param eid    идентификатор события
 * @param action действие посредника
 */
void awh::unit::Mediator::action(const event::id_t eid, const event::action_t action) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("action"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::action_t)> ("action", eid, action);
}
/**
 * @brief Метод обработки событий изменения статуса посредника
 *
 * @param eid    идентификатор события
 * @param status новый статус посредника
 */
void awh::unit::Mediator::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("state"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки событий получения данных посредником
 *
 * @param eid  идентификатор события
 * @param data данные события получения данных посредником
 * @param size размер данных события получения данных посредником
 */
void awh::unit::Mediator::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("read"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, data, size);
}
/**
 * @brief Метод обработки событий ошибок посредника
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::unit::Mediator::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::Mediator::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков для родительского юнита
	unit_t::threadSafety(mode);
	// Устанавливаем режим безопасности работы потоков для объекта блокировки
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод фиксации настроек посредника
 *
 * @param eid идентификатор события посредника
 * @return    результат выполнения фиксации
 */
bool awh::unit::Mediator::commit(const event::id_t eid) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием посредника
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем фиксацию параметров события посредника
			result = this->_io->commit(eid);
		}
		// Если результат фиксации параметров события посредника не успешный
		if(!result){
			{
				// Выполняем блокировку потока для работы с событием посредника
				const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
				// Удаляем событие посредника
				this->_io->destroy(eid);
			}{
				// Выполняем блокировку потока для работы временным списком событий посредника
				const locker_t <> lock(this->_mtx);
				// Выполняем поиск идентификатора события посредника в списке событий посредника
				auto i = this->_events.find(eid);
				// Если идентификатор события посредника найден в списке событий посредника
				if(i != this->_events.end())
					// Удаляем идентификатор события посредника из списка событий посредника
					this->_events.erase(i);
			}
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Failed to launch client", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Failed to launch client", log_t::flag_t::CRITICAL);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод отправки данных посредником
 *
 * @param eid    идентификатор события посредника
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных посредником
 */
size_t awh::unit::Mediator::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем отправку данных посредником
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Метод перемещения данных между посредником и другим событием
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения перемещения
 */
bool awh::unit::Mediator::splice(const event::id_t eid, const event::id_t dest) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем перемещение данных между событием посредника и другим событием
	return this->_io->splice(eid, dest);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события посредника
 * @return    адрес хоста целевой машины
 */
string awh::unit::Mediator::getTarget(const event::id_t eid) const noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение адреса хоста целевой машины для события посредника
	return this->_io->getTarget(eid);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Mediator::setTarget(const event::id_t eid, string_view target) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку адреса хоста целевой машины для события посредника
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Mediator::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Выполняем установку адреса хоста целевой машины для события посредника
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события посредника
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::unit::Mediator::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение адреса хоста целевой машины для события посредника
	return this->_io->getTarget(eid, target);
}
/**
 * @brief Метод получения адреса посредника
 *
 * @param eid     идентификатор события посредника
 * @param address тип адреса посредника
 * @return        значение адреса посредника
 */
string awh::unit::Mediator::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
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
 */
bool awh::unit::Mediator::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
 */
bool awh::unit::Mediator::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
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
 */
bool awh::unit::Mediator::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Выполняем блокировку потока для работы с событием посредника
	const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::SHARED);
	// Выполняем получение адреса посредника для события посредника
	return this->_io->getAddress(eid, address, value);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
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
 */
void awh::unit::Mediator::destroy(const event::id_t eid) noexcept {
	{
		// Выполняем блокировку потока для уничтожения события посредника
		const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Удаляем событие посредника
		this->_io->destroy(eid);
	}
	// Если в списке событий посредника есть события
	if(!this->_events.empty()){
		// Выполняем блокировку потока для работы временным списком событий посредника
		const locker_t <> lock(this->_mtx);
		// Выполняем поиск идентификатора события посредника в списке событий посредника
		auto i = this->_events.find(eid);
		// Если идентификатор события посредника найден в списке событий посредника
		if(i != this->_events.end())
			// Удаляем идентификатор события посредника из списка событий посредника
			this->_events.erase(i);
	}
}
/**
 * @brief Метод получения идентификатора посредника для перехвата пакетов тоннеля
 *
 * @param family семейство адресов
 * @return       идентификатор созданного посредника
 */
awh::event::id_t awh::unit::Mediator::issue(const event::family_t family) noexcept {
	// Результат работы функции
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		{
			// Выполняем блокировку потока для работы с событием посредника
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Добавляем новое событие посредника для перехвата пакетов тоннеля
			result = this->_io->event(event::node_t::MEDIATOR, family);
			// Устанавливаем функцию обратного вызова на событие изменения действий посредника
			this->_io->on(result, static_cast <engine::callback::event_t> (std::bind(&mediator_t::action, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие чтения данных
			this->_io->on(result, static_cast <engine::callback::read_t> (std::bind(&mediator_t::read, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие изменения статуса посредника
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&mediator_t::status, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&mediator_t::error, this, _1, _2, _3)));
		}
		// Выполняем блокировку потока для работы временным списком событий посредника
		const locker_t <> lock(this->_mtx);
		// Добавляем идентификатор события посредника в список событий посредника
		this->_events.emplace(result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Mediator::Mediator(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {
	// Деактивируем мьютекс на время инициализации
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::unit::Mediator::~Mediator() noexcept {
	// Если в списке событий посредника есть события
	if(!this->_events.empty()){
		// Выполняем блокировку потока для работы временным списком событий посредника
		const locker_t <> lock(this->_mtx);
		// Выполняем удаление всех событий посредника
		for(const auto & eid : this->_events){
			// Выполняем блокировку потока для уничтожения событий
			const locker_t <std::shared_mutex> lock(this->mtx(), locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Удаляем событие посредника
			this->_io->destroy(eid);
		}
	}
}
