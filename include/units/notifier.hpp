/**
 * @file: notifier.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_NOTIFIER__
#define __AWH_UNIT_NOTIFIER__

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
		 * @brief Класс узла уведомителя
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Notifier : public unit_t {
			private:
				// Список идентификаторов событий файловой системы
				unordered_set <event::id_t> _events;
			private:
				/**
				 * @brief Метод обработки событий записи сообщений уведомителя
				 *
				 * @param eid  идентификатор события
				 * @param size размер сообщения
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий чтения сообщений уведомителя
				 *
				 * @param eid  идентификатор события
				 * @param data данные сообщения
				 * @param size размер сообщения
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки состояния уведомителя
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки исключений событий уведомителя
				 *
				 * @param eid     идентификатор события
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * @brief Метод обработки событий доступного размера очереди события уведомителя
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 * @param size   доступный размер очереди в байтах
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод создания события уведомителя
				 *
				 * @return идентификатор события уведомителя
				 */
				event::id_t create() noexcept;
			public:
				/**
				 * @brief Метод уничтожения события уведомителя
				 *
				 * @param eid идентификатор события уведомителя
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод триггера события уведомителя
				 *
				 * @param eid    идентификатор события уведомителя
				 * @param buffer буфер данных для отправки
				 * @param size   размер буфера данных
				 * @return       количество отправленных байт
				 */
				size_t trigger(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Notifier(const Notifier &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				Notifier & operator = (const Notifier &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Notifier(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Notifier() noexcept;
		} notifier_t;
	};
};

#endif // __AWH_UNIT_NOTIFIER__
