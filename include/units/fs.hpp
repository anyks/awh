/**
 * @file fs.hpp
 * @date 2026-02-25
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
 * \~russian
 * @brief Заголовочный файл модуля наблюдения за файловой системой — класс unit::Filesystem, отслеживающий создание,
 *        изменение, переименование и удаление файлов и каталогов через нативные механизмы операционной системы
 *
 * \~english
 * @brief Header file of the filesystem watching module — the unit::Filesystem class, which tracks the creation,
 *        modification, renaming and removal of files and directories through the native mechanisms of the operating system
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_FILESYSTEM__
#define __AWH_UNIT_FILESYSTEM__

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс узла файловой системы
		 *
		 * \~english
		 * @brief Filesystem unit class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Filesystem : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief Тип ноды файловой системы
				 *
				 * \~english
				 * @brief Filesystem node type
				 *
				 * \~
				 */
				enum class type_t : uint8_t {
					NONE = 0x00, // Тип ноды файловой системы не установлен
					DIR  = 0x01, // Тип ноды файловой системы принадлежит к каталогам
					FILE = 0x02  // Тип ноды файловой системы принадлежит к файлам
				};
			private:
				// Список идентификаторов событий файловой системы
				unordered_set <event::id_t> _events;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий записи в файл
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing file write events
				 * @param eid  filesystem event identifier
				 * @param size message size
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий чтения из файла
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing file read events
				 * @param eid  filesystem event identifier
				 * @param data message data
				 * @param size message size
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки состояния файловой системы
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param status статус события
				 *
				 * \~english
				 * @brief Method of processing the filesystem state
				 * @param eid    filesystem event identifier
				 * @param status event status
				 *
				 * \~
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки исключений событий файловой системы
				 *
				 * @param eid     идентификатор события файловой системы
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 *
				 * \~english
				 * @brief Method of processing filesystem event exceptions
				 * @param eid     filesystem event identifier
				 * @param error   error type
				 * @param message error message
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий каталогов
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action экшен события каталога (добавление/удаление)
				 * @param vnode  событие файловой системы
				 * @param path   адрес по которому произошло событие
				 *
				 * \~english
				 * @brief Method of processing directory events
				 * @param eid    filesystem event identifier
				 * @param action directory event action (addition/removal)
				 * @param vnode  filesystem event
				 * @param path   address at which the event has occurred
				 *
				 * \~
				 */
				void vnode(const event::id_t eid, const event::action_t action, const event::vnode_t vnode, const std::string & path) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод уничтожения события файловой системы
				 *
				 * @param eid идентификатор события файловой системы
				 *
				 * \~english
				 * @brief Method of destroying a filesystem event
				 * @param eid filesystem event identifier
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания события файловой системы
				 *
				 * @param type тип файловой системы для создания
				 * @return     идентификатор события файловой системы
				 *
				 * \~english
				 * @brief Method of creating a filesystem event
				 * @param type type of the filesystem to be created
				 * @return     filesystem event identifier
				 *
				 * \~
				 */
				event::id_t create(const type_t type) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа ноды файловой системы
				 *
				 * @param eid идентификатор события файловой системы
				 * @return    тип ноды файловой системы
				 *
				 * \~english
				 * @brief Method of getting the filesystem node type
				 * @param eid filesystem event identifier
				 * @return    filesystem node type
				 *
				 * \~
				 */
				type_t type(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса события
				 *
				 * @param eid идентификатор события файловой системы
				 * @return    значение адреса события
				 *
				 * \~english
				 * @brief Method of getting the event address
				 * @param eid filesystem event identifier
				 * @return    value of the event address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса события
				 *
				 * @param eid   идентификатор события файловой системы
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event address
				 * @param eid   filesystem event identifier
				 * @param value value of the event address
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const string & value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки данных в файл
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param buffer буфер данных для отправки
				 * @param size   размер буфера данных
				 * @return       количество отправленных байт
				 *
				 * \~english
				 * @brief Method of sending data to a file
				 * @param eid    filesystem event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data buffer
				 * @return       number of bytes sent
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения смещения в файле события
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param seek тип смещения в файле события
				 * @return     смещение в файле события
				 *
				 * \~english
				 * @brief Method of getting the offset in the event file
				 * @param eid  filesystem event identifier
				 * @param seek type of the offset in the event file
				 * @return     offset in the event file
				 *
				 * \~
				 */
				size_t getSeek(const event::id_t eid, const event::seek_t seek) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки смещения в файле события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param seek   тип смещения в файле события
				 * @param offset смещение в файле события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the offset in the event file
				 * @param eid    filesystem event identifier
				 * @param seek   type of the offset in the event file
				 * @param offset offset in the event file
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setSeek(const event::id_t eid, const event::seek_t seek, const size_t offset) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций события
				 *
				 * @param eid идентификатор события
				 * @return    опции события
				 *
				 * \~english
				 * @brief Method of getting the event options
				 * @param eid event identifier
				 * @return    event options
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций события
				 *
				 * @param eid    идентификатор события
				 * @param options опции события для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event options
				 * @param eid    event identifier
				 * @param options event options to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции события
				 *
				 * @param eid    идентификатор события
				 * @param option опция события для установки
				 * @param mode   режим установки опции события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting an event option
				 * @param eid    event identifier
				 * @param option event option to be set
				 * @param mode   mode of setting the event option
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 * \~english
				 * @brief Method of getting the event buffer size
				 * @param eid    filesystem event identifier
				 * @param action event action type
				 * @return       event buffer size
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event buffer size
				 * @param eid    filesystem event identifier
				 * @param action event action type
				 * @param size   event buffer size
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				Filesystem(const Filesystem &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				Filesystem & operator = (const Filesystem &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Filesystem(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Filesystem() noexcept;
		} fs_t;
	};
};

#endif // __AWH_UNIT_FILESYSTEM__
