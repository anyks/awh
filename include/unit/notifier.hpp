/**
 * @file notifier.hpp
 * @date 2026-02-22
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
 * @brief Заголовочный файл модуля уведомителя — класс unit::Notifier,
 *        обеспечивающий пробуждение цикла событий и доставку пользовательских уведомлений между потоками
 *
 * \~english
 * @brief Header file of the notifier module — the unit::Notifier class,
 *        which wakes up the event loop and delivers user notifications between threads
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
		 * @brief Класс узла уведомителя
		 *
		 * \~english
		 * @brief Notifier unit class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Notifier : public unit_t {
			private:
				// Список идентификаторов событий файловой системы
				unordered_set <event::id_t> _events;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий записи сообщений уведомителя
				 *
				 * @param eid  идентификатор события
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing notifier message write events
				 * @param eid  event identifier
				 * @param size message size
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий чтения сообщений уведомителя
				 *
				 * @param eid  идентификатор события
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing notifier message read events
				 * @param eid  event identifier
				 * @param data message data
				 * @param size message size
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки состояния уведомителя
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 *
				 * \~english
				 * @brief Method of processing the notifier state
				 * @param eid    event identifier
				 * @param status event status
				 *
				 * \~
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки исключений событий уведомителя
				 *
				 * @param eid     идентификатор события
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 *
				 * \~english
				 * @brief Method of processing notifier event exceptions
				 * @param eid     event identifier
				 * @param error   error type
				 * @param message error message
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий доступного размера очереди события уведомителя
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 * @param size   доступный размер очереди в байтах
				 *
				 * \~english
				 * @brief Method of processing available queue size events of the notifier event
				 * @param eid    event identifier
				 * @param status event status
				 * @param size   available queue size in bytes
				 *
				 * \~
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания события уведомителя
				 *
				 * @return идентификатор события уведомителя
				 *
				 * \~english
				 * @brief Method of creating a notifier event
				 * @return notifier event identifier
				 *
				 * \~
				 */
				event::id_t create() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод уничтожения события уведомителя
				 *
				 * @param eid идентификатор события уведомителя
				 *
				 * \~english
				 * @brief Method of destroying a notifier event
				 * @param eid notifier event identifier
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
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
				 * @brief Метод триггера события уведомителя
				 *
				 * @param eid    идентификатор события уведомителя
				 * @param buffer буфер данных для отправки
				 * @param size   размер буфера данных
				 * @return       количество отправленных байт
				 *
				 * \~english
				 * @brief Method of triggering a notifier event
				 * @param eid    notifier event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data buffer
				 * @return       number of bytes sent
				 *
				 * \~
				 */
				size_t trigger(const event::id_t eid, const void * buffer, const size_t size) noexcept;
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
				Notifier(const Notifier &) = delete;
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
				Notifier & operator = (const Notifier &) = delete;
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
				explicit Notifier(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Notifier() noexcept;
		} notifier_t;
	};
};

#endif // __AWH_UNIT_NOTIFIER__
