/**
 * @file: fs.hpp
 * @date: 2026-02-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля наблюдения за файловой системой — класс unit::Filesystem, отслеживающий создание,
 *        изменение, переименование и удаление файлов и каталогов через нативные механизмы операционной системы
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модулей
	 *
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс узла файловой системы
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Filesystem : public unit_t {
			public:
				/**
				 * @brief Тип ноды файловой системы
				 *
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
				 * @brief Метод обработки событий записи в файл
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param size размер сообщения
				 *
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий чтения из файла
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки состояния файловой системы
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param status статус события
				 *
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки исключений событий файловой системы
				 *
				 * @param eid     идентификатор события файловой системы
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * @brief Метод обработки событий каталогов
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action экшен события каталога (добавление/удаление)
				 * @param vnode  событие файловой системы
				 * @param path   адрес по которому произошло событие
				 *
				 */
				void vnode(const event::id_t eid, const event::action_t action, const event::vnode_t vnode, const std::string & path) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события файловой системы
				 *
				 * @param eid идентификатор события файловой системы
				 *
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод создания события файловой системы
				 *
				 * @param type тип файловой системы для создания
				 * @return     идентификатор события файловой системы
				 *
				 */
				event::id_t create(const type_t type) noexcept;
			public:
				/**
				 * @brief Метод получения типа ноды файловой системы
				 *
				 * @param eid идентификатор события файловой системы
				 * @return    тип ноды файловой системы
				 *
				 */
				type_t type(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод получения адреса события
				 *
				 * @param eid идентификатор события файловой системы
				 * @return    значение адреса события
				 *
				 */
				string getAddress(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса события
				 *
				 * @param eid   идентификатор события файловой системы
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const string & value) noexcept;
			public:
				/**
				 * @brief Метод отправки данных в файл
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param buffer буфер данных для отправки
				 * @param size   размер буфера данных
				 * @return       количество отправленных байт
				 *
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения смещения в файле события
				 *
				 * @param eid  идентификатор события файловой системы
				 * @param seek тип смещения в файле события
				 * @return     смещение в файле события
				 *
				 */
				size_t getSeek(const event::id_t eid, const event::seek_t seek) noexcept;
				/**
				 * @brief Метод установки смещения в файле события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param seek   тип смещения в файле события
				 * @param offset смещение в файле события
				 * @return       результат выполнения установки
				 *
				 */
				bool setSeek(const event::id_t eid, const event::seek_t seek, const size_t offset) noexcept;
			public:
				/**
				 * @brief Метод получения опций события
				 *
				 * @param eid идентификатор события
				 * @return    опции события
				 *
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций события
				 *
				 * @param eid    идентификатор события
				 * @param options опции события для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции события
				 *
				 * @param eid    идентификатор события
				 * @param option опция события для установки
				 * @param mode   режим установки опции события
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param eid    идентификатор события файловой системы
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Filesystem(const Filesystem &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Filesystem & operator = (const Filesystem &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Filesystem(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Filesystem() noexcept;
		} fs_t;
	};
};

#endif // __AWH_UNIT_FILESYSTEM__
