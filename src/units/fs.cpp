/**
 * @file: fs.cpp
 * @date: 2026-02-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля наблюдения за файловой системой — отслеживание создания, изменения,
 *        переименования и удаления файлов и каталогов через нативные механизмы операционной системы
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/fs.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод обработки событий записи в файл
 *
 * @param eid  идентификатор события файловой системы
 * @param size размер сообщения
 *
 */
void awh::unit::Filesystem::write(const event::id_t eid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const size_t)> ("write", eid, size);
}
/**
 * @brief Метод обработки событий чтения из файла
 *
 * @param eid  идентификатор события файловой системы
 * @param data данные сообщения
 * @param size размер сообщения
 *
 */
void awh::unit::Filesystem::read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const uint8_t *, const size_t)> ("read", eid, data, size);
}
/**
 * @brief Метод обработки состояния файловой системы
 *
 * @param eid    идентификатор события файловой системы
 * @param status статус события
 *
 */
void awh::unit::Filesystem::state(const event::id_t eid, const event::status_t status) noexcept {
	// Если статус файловой системы представляет из себя уничтожение
	if(status == event::status_t::DESTROYED){
		// Если в списке событий файловой системы есть события
		if(!this->_events.empty()){
			// Выполняем поиск идентификатора события файловой системы в списке событий файловой системы
			auto i = this->_events.find(eid);
			// Если идентификатор события файловой системы найден в списке событий файловой системы
			if(i != this->_events.end())
				// Удаляем идентификатор события файловой системы из списка событий файловой системы
				this->_events.erase(i);
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::status_t)> ("state", eid, status);
}
/**
 * @brief Метод обработки исключений событий файловой системы
 *
 * @param eid     идентификатор события файловой системы
 * @param error   тип ошибки
 * @param message сообщение об ошибке
 *
 */
void awh::unit::Filesystem::error(const event::id_t eid, const event::error_t error, const string & message) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, message);
}
/**
 * @brief Метод обработки событий каталогов
 *
 * @param eid    идентификатор события файловой системы
 * @param action экшен события каталога (добавление/удаление)
 * @param vnode  событие файловой системы
 * @param path   адрес по которому произошло событие
 *
 */
void awh::unit::Filesystem::vnode(const event::id_t eid, const event::action_t action, const event::vnode_t vnode, const std::string & path) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::action_t, const event::vnode_t, const string &)> ("vnode", eid, action, vnode, path);
}
/**
 * @brief Метод уничтожения события файловой системы
 *
 * @param eid идентификатор события файловой системы
 *
 */
void awh::unit::Filesystem::destroy(const event::id_t eid) noexcept {
	// Удаляем событие файловой системы
	this->_io->destroy(eid);
}
/**
 * @brief Метод создания события файловой системы
 *
 * @param type тип файловой системы для создания
 * @return     идентификатор события файловой системы
 *
 */
awh::event::id_t awh::unit::Filesystem::create(const type_t type) noexcept {
	// Переменная результата
	event::id_t result = 0;
	/**
	 * Определяем тип файловой системы для создания
	 */
	switch(static_cast <uint8_t> (type)){
		// Если тип файловой системы является наблюдателем за каталогами
		case static_cast <uint8_t> (type_t::DIR):
			// Выполняем создание события файловой системы для узла каталога
			result = this->_io->event(event::node_t::DIR, event::family_t::FSYS);
		break;
		// Если тип файловой системы является наблюдателем за файлами
		case static_cast <uint8_t> (type_t::FILE):
			// Выполняем создание события файловой системы для узла файла
			result = this->_io->event(event::node_t::FILE, event::family_t::FSYS);
		break;
		// Для неизвестного типа файловой системы
		default: {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог создания события
				this->_log->debug("Filesystem event could not be created because its type is not defined", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type)), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог создания события
				this->_log->print("Filesystem event could not be created because its type is not defined", log_t::flag_t::WARNING);
			#endif
			// Возвращаем результат
			return result;
		}
	}
	// Если событие файловой системы успешно создано
	if(result > 0){
		// Если тип файловой системы является наблюдателем за файлами
		if(type == type_t::FILE){
			// Устанавливаем функцию обратного вызова на событие записи сообщений
			this->_io->on(result, static_cast <engine::callback::write_t> (std::bind(&fs_t::write, this, _1, _2)));
			// Устанавливаем функцию обратного вызова на событие чтения сообщений
			this->_io->on(result, static_cast <engine::callback::read_t> (std::bind(&fs_t::read, this, _1, _2, _3)));
		}
		// Устанавливаем функцию обратного вызова на событие изменения статуса
		this->_io->on(result, static_cast <engine::callback::status_t> (std::bind(&fs_t::state, this, _1, _2)));
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(result, static_cast <engine::callback::error_t> (std::bind(&fs_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие изменения состояния каталога
		this->_io->on(result, static_cast <engine::callback::vnode_t> (std::bind(&fs_t::vnode, this, _1, _2, _3, _4)));
		// Добавляем идентификатор события файловой системы в список событий файловой системы
		this->_events.emplace(result);
	// Если событие файловой системы не может быть создано
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог создания события
			this->_log->debug("Filesystem event could not be created", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (type)), log_t::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог создания события
			this->_log->print("Filesystem event could not be created", log_t::flag_t::WARNING);
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения типа ноды файловой системы
 *
 * @param eid идентификатор события файловой системы
 * @return    тип ноды файловой системы
 *
 */
awh::unit::Filesystem::type_t awh::unit::Filesystem::type(const event::id_t eid) const noexcept {
	/**
	 * Определяем тип узла события файловой системы
	 */
	switch(static_cast <uint8_t> (this->_io->node(eid))){
		// Если тип узла события является каталогом
		case static_cast <uint8_t> (event::node_t::DIR):
			// Устанавливаем результат типа ноды файловой системы как каталог
			return type_t::DIR;
		// Если тип узла события является файлом
		case static_cast <uint8_t> (event::node_t::FILE):
			// Устанавливаем результат типа ноды файловой системы как файл
			return type_t::FILE;
	}
	// Возвращаем значение по умолчанию
	return type_t::NONE;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::Filesystem::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при чтении из файла
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова при записи в файл
	this->_callback.set("write", callback);
	// Выполняем установку функции обратного вызова при изменении состояния события файловой системы
	this->_callback.set("state", callback);
	// Выполняем установку функции обратного вызова при изменении состояния узла файловой системы
	this->_callback.set("vnode", callback);
}
/**
 * @brief Метод получения адреса события
 *
 * @param eid идентификатор события файловой системы
 * @return    значение адреса события
 *
 */
string awh::unit::Filesystem::getAddress(const event::id_t eid) const noexcept {
	// Выполняем получение адреса события
	return this->_io->getAddress(eid, event::address_t::FS);
}
/**
 * @brief Метод установки адреса события
 *
 * @param eid   идентификатор события файловой системы
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 */
bool awh::unit::Filesystem::setAddress(const event::id_t eid, const string & value) noexcept {
	// Переменная результата
	bool result = this->_io->setAddress(eid, event::address_t::FS, value);
	// Если результат выполнения установки адреса события является успешным
	if(result)
		// Выполняем запуск события файловой системы
		result = (this->_io->commit(eid) && this->_io->launch(eid));
	// Выполняем результат
	return result;
}
/**
 * @brief Метод отправки данных в файл
 *
 * @param eid    идентификатор события файловой системы
 * @param buffer буфер данных для отправки
 * @param size   размер буфера данных
 * @return       количество отправленных байт
 *
 */
size_t awh::unit::Filesystem::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Выполняем отправку данных в файл
	return this->_io->send(eid, buffer, size);
}
/**
 * @brief Метод получения смещения в файле события
 *
 * @param eid  идентификатор события файловой системы
 * @param seek тип смещения в файле события
 * @return     смещение в файле события
 *
 */
size_t awh::unit::Filesystem::getSeek(const event::id_t eid, const event::seek_t seek) noexcept {
	// Выполняем получение смещения в файле события
	return this->_io->getSeek(eid, seek);
}
/**
 * @brief Метод установки смещения в файле события
 *
 * @param eid    идентификатор события файловой системы
 * @param seek   тип смещения в файле события
 * @param offset смещение в файле события
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Filesystem::setSeek(const event::id_t eid, const event::seek_t seek, const size_t offset) noexcept {
	// Выполняем установку смещения в файле события
	return this->_io->setSeek(eid, seek, offset);
}
/**
 * @brief Метод получения опций события
 *
 * @param eid идентификатор события
 * @return    опции события
 *
 */
uint16_t awh::unit::Filesystem::getOptions(const event::id_t eid) const noexcept {
	// Выполняем получение опций события
	return this->_io->getOptions(eid);
}
/**
 * @brief Метод установки опций события
 *
 * @param eid    идентификатор события
 * @param options опции события для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::Filesystem::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Выполняем установку опций события
	return this->_io->setOptions(eid, options);
}
/**
 * @brief Метод установки опции события
 *
 * @param eid    идентификатор события
 * @param option опция события для установки
 * @param mode   режим установки опции события
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Filesystem::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Выполняем установку опции события
	return this->_io->setOption(eid, option, mode);
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param eid    идентификатор события файловой системы
 * @param action тип действия события
 * @return       размер буфера события
 *
 */
size_t awh::unit::Filesystem::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Выполняем получение размера буфера события
	return this->_io->getBufferSize(eid, action);
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param eid    идентификатор события файловой системы
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
 */
bool awh::unit::Filesystem::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Выполняем установку размера буфера события
	return this->_io->setBufferSize(eid, action, size);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::Filesystem::Filesystem(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::Filesystem::~Filesystem() noexcept {
	// Если в списке событий файловой системы есть события
	if(!this->_events.empty()){
		// Копируем список событий файловой системы для безопасного удаления событий файловой системы во время итерации
		auto events = this->_events;
		/**
		 * Выполняем удаление всех событий файловой системы
		 */
		for(const auto & eid : events){
			// Если тип файловой системы является наблюдателем за файлами
			if(event::node_t::FILE == this->_io->node(eid)){
				// Снимаем функцию обратного вызова на событие записи сообщений
				this->_io->on(eid, static_cast <engine::callback::write_t> (nullptr));
				// Снимаем функцию обратного вызова на событие чтения сообщений
				this->_io->on(eid, static_cast <engine::callback::read_t> (nullptr));
			}
			// Снимаем функцию обратного вызова на событие изменения статуса
			this->_io->on(eid, static_cast <engine::callback::status_t> (nullptr));
			// Снимаем функцию обратного вызова на событие получения ошибок
			this->_io->on(eid, static_cast <engine::callback::error_t> (nullptr));
			// Снимаем функцию обратного вызова на событие изменения состояния каталога
			this->_io->on(eid, static_cast <engine::callback::vnode_t> (nullptr));
			// Удаляем событие файловой системы
			this->_io->destroy(eid);
		}
	}
}
