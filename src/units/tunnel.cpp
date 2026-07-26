/**
 * @file: tunnel.cpp
 * @date: 2026-03-23
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
#include <units/tunnel.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод обработки событий изменения статуса туннеля
 *
 * @param eid    идентификатор события
 * @param status новый статус туннеля
 */
void awh::unit::Tunnel::status(const event::id_t eid, const event::status_t status) noexcept {
	// Если статус туннеля представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Если в списке событий туннеля есть события
		if(!this->_events.empty()){
			// Выполняем поиск идентификатора события туннеля в списке событий туннеля
			auto i = this->_events.find(eid);
			// Если идентификатор события туннеля найден в списке событий туннеля
			if(i != this->_events.end())
				// Удаляем идентификатор события туннеля из списка событий туннеля
				this->_events.erase(i);
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих данных туннеля
 *
 * @param eid    идентификатор события
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 */
void awh::unit::Tunnel::available(const event::id_t eid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t, const size_t)> ("available", eid, status, size);
}
/**
 * @brief Метод обработки событий ошибок туннеля
 *
 * @param eid         идентификатор события
 * @param error       тип ошибки
 * @param description описание ошибки
 */
void awh::unit::Tunnel::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки событий получения информации о пакетах в туннеле
 *
 * @param eid    идентификатор события туннеля
 * @param mid    идентификатор события посредника
 * @param action действие туннеля
 * @param info   информация о пакетах в туннеле
 */
void awh::unit::Tunnel::info(const event::id_t eid, const event::id_t mid, const event::action_t action, const net::tun_info_t & info) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::id_t, const event::action_t, const net::tun_info_t &)> ("info", eid, mid, action, info);
}
/**
 * @brief Метод фиксации настроек туннеля
 *
 * @param eid идентификатор события туннеля
 * @return    результат выполнения фиксации
 */
bool awh::unit::Tunnel::commit(const event::id_t eid) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем фиксацию параметров события и его запуск
		if(!(result = (this->_io->commit(eid) && this->_io->launch(eid)))){
			// Удаляем событие туннеля
			this->_io->destroy(eid);
			// Выполняем поиск идентификатора события туннеля в списке событий туннеля
			auto i = this->_events.find(eid);
			// Если идентификатор события туннеля найден в списке событий туннеля
			if(i != this->_events.end())
				// Удаляем идентификатор события туннеля из списка событий туннеля
				this->_events.erase(i);
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Failed to launch tunnel", __PRETTY_FUNCTION__, make_tuple(eid), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Failed to launch tunnel", log_t::flag_t::CRITICAL);
				#endif
			}
		// Если функция обратного вызова установлена
		} else if(this->_callback.is("info"))
			// Устанавливаем функцию обратного вызова на событие получения информации о пакетах в туннеле
			this->_io->on(eid, static_cast <engine::callback::tuninfo_t> (std::bind(&tunnel_t::info, this, _1, _2, _3, _4)));
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
 * @brief Метод отправки данных через туннель
 *
 * @param eid    идентификатор события туннеля
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных через туннель
 */
size_t awh::unit::Tunnel::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Выполняем отправку данных через туннель
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Метод получения опций туннеля
 *
 * @param eid идентификатор события туннеля
 * @return    опции туннеля
 */
uint16_t awh::unit::Tunnel::getOptions(const event::id_t eid) const noexcept {
	// Выполняем получение опций для события туннеля
	return this->_io->getOptions(eid);
}
/**
 * @brief Метод установки опций туннеля
 *
 * @param eid     идентификатор события туннеля
 * @param options опции туннеля для установки
 * @return        результат выполнения установки
 */
bool awh::unit::Tunnel::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Выполняем установку опций для события туннеля
	return this->_io->setOptions(eid, options);
}
/**
 * @brief Метод установки опции туннеля
 *
 * @param eid    идентификатор события туннеля
 * @param option опция туннеля для установки
 * @param mode   режим установки опции туннеля
 * @return       результат выполнения установки
 */
bool awh::unit::Tunnel::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Выполняем установку опции для события туннеля
	return this->_io->setOption(eid, option, mode);
}
/**
 * @brief Метод получения сетевого интерфейса туннеля
 *
 * @param eid идентификатор события туннеля
 * @return    сетевой интерфейс туннеля
 */
string awh::unit::Tunnel::getIface(const event::id_t eid) const noexcept {
	// Выполняем получение сетевого интерфейса для события туннеля
	return this->_io->getIface(eid);
}
/**
 * @brief Метод установки сетевого интерфейса туннеля
 *
 * @param eid  идентификатор события туннеля
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 */
bool awh::unit::Tunnel::setIface(const event::id_t eid, string_view name) noexcept {
	// Выполняем установку сетевого интерфейса для события туннеля
	return this->_io->setIface(eid, name);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события туннеля
 * @return    адрес хоста целевой машины
 */
string awh::unit::Tunnel::getTarget(const event::id_t eid) const noexcept {
	// Выполняем получение адреса хоста целевой машины для события туннеля
	return this->_io->getTarget(eid);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события туннеля
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Tunnel::setTarget(const event::id_t eid, string_view target) noexcept {
	// Выполняем установку адреса хоста целевой машины для события туннеля
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события туннеля
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 */
bool awh::unit::Tunnel::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Выполняем установку адреса хоста целевой машины для события туннеля
	return this->_io->setTarget(eid, target);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события туннеля
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 */
bool awh::unit::Tunnel::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Выполняем получение адреса хоста целевой машины для события туннеля
	return this->_io->getTarget(eid, target);
}
/**
 * @brief Метод получения адреса туннеля
 *
 * @param eid     идентификатор события туннеля
 * @param address тип адреса туннеля
 * @return        значение адреса туннеля
 */
string awh::unit::Tunnel::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Выполняем получение адреса туннеля для события туннеля
	return this->_io->getAddress(eid, address);
}
/**
 * @brief Метод установки адреса туннеля
 *
 * @param eid     идентификатор события туннеля
 * @param address тип адреса туннеля
 * @param value   значение адреса туннеля
 * @return        результат выполнения установки
 */
bool awh::unit::Tunnel::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Выполняем установку адреса туннеля для события туннеля
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод установки адреса туннеля
 *
 * @param eid     идентификатор события туннеля
 * @param address тип адреса туннеля
 * @param value   значение адреса туннеля
 * @return        результат выполнения установки
 */
bool awh::unit::Tunnel::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Выполняем установку адреса туннеля для события туннеля
	return this->_io->setAddress(eid, address, value);
}
/**
 * @brief Метод получения адреса туннеля
 *
 * @param eid     идентификатор события туннеля
 * @param address тип адреса туннеля
 * @param value   объект для извлечения адреса туннеля
 * @return        результат выполнения извлечения адреса туннеля
 */
bool awh::unit::Tunnel::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Выполняем получение адреса туннеля для события туннеля
	return this->_io->getAddress(eid, address, value);
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события туннеля
 * @return    MTU сетевого интерфейса
 */
uint16_t awh::unit::Tunnel::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Выполняем получение MTU сетевого интерфейса для события туннеля
	return this->_io->getMaximumTransmissionUnit(eid);
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события туннеля
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 */
bool awh::unit::Tunnel::setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept {
	// Выполняем установку MTU сетевого интерфейса для события туннеля
	return this->_io->setMaximumTransmissionUnit(eid, mtu);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Tunnel::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении информации о пакетах в туннеле
	this->_callback.set("info", callback);
	// Выполняем установку функции обратного вызова при получении состояния туннеля
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при получении событий доступности/недоступности очереди исходящих данных туннеля
	this->_callback.set("available", callback);
}
/**
 * @brief Метод уничтожения события туннеля
 *
 * @param eid идентификатор события для уничтожения
 */
void awh::unit::Tunnel::destroy(const event::id_t eid) noexcept {
	// Удаляем событие туннеля
	this->_io->destroy(eid);
}
/**
 * @brief Метод получения идентификатора туннеля
 *
 * @param family семейство адресов
 * @return       идентификатор созданного туннеля
 */
awh::event::id_t awh::unit::Tunnel::issue(const event::family_t family) noexcept {
	// Переменная результата
	event::id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Добавляем новое событие туннеля
		result = this->_io->event(event::node_t::TUNNEL, family);
		// Если событие туннеля успешно создано
		if(result > 0){
			// Устанавливаем функцию обратного вызова на событие изменения статуса туннеля
			this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&tunnel_t::status, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие получения ошибок
			this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&tunnel_t::error, this, _1, _2, _3)));
			// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных туннеля
			this->_io->on(result, static_cast <engine::callback::available_t> (std::bind(&tunnel_t::available, this, _1, _2, _3)));
			// Добавляем идентификатор события туннеля в список событий туннеля
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
 */
awh::unit::Tunnel::Tunnel(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Tunnel::~Tunnel() noexcept {
	// Если в списке событий туннеля есть события
	if(!this->_events.empty()){
		// Копируем список событий туннеля для безопасного удаления событий туннеля во время итерации
		auto events = this->_events;
		/**
		 * Выполняем удаление всех событий туннеля
		 */
		for(const auto & eid : events){
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(eid, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения статуса туннеля
			this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения информации о пакетах в туннеле
			this->_io->on(eid, static_cast <engine::callback::tuninfo_t> (nullptr));
			// Снимаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных туннеля
			this->_io->on(eid, static_cast <engine::callback::available_t> (nullptr));
			// Удаляем событие туннеля
			this->_io->destroy(eid);
		}
	}
}
