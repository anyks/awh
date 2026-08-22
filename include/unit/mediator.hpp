/**
 * @file mediator.hpp
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
 * \~russian
 * @brief Заголовочный файл модуля посредника — класс unit::Mediator, связывающий два независимых узла в
 *        двунаправленный канал передачи данных и обеспечивающий проксирование трафика между ними
 *
 * \~english
 * @brief Header file of the mediator module — the unit::Mediator class, which links two independent units into
 *        a bidirectional data transfer channel and proxies the traffic between them
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_MEDIATOR__
#define __AWH_UNIT_MEDIATOR__

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
		 * @brief Класс посредника
		 *
		 * \~english
		 * @brief Mediator class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Mediator : public unit_t {
			private:
				// Список идентификаторов событий посредника
				unordered_set <event::id_t> _events;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки действий посредника
				 *
				 * @param eid    идентификатор события
				 * @param action действие посредника
				 *
				 * \~english
				 * @brief Method of processing the mediator actions
				 * @param eid    event identifier
				 * @param action mediator action
				 *
				 * \~
				 */
				void action(const event::id_t eid, const event::action_t action) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса посредника
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус посредника
				 *
				 * \~english
				 * @brief Method of processing mediator status change events
				 * @param eid    event identifier
				 * @param status new mediator status
				 *
				 * \~
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий получения данных посредником
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных посредником
				 * @param size размер данных события получения данных посредником
				 *
				 * \~english
				 * @brief Method of processing data reception events of the mediator
				 * @param eid  event identifier
				 * @param data data of the mediator data reception event
				 * @param size size of the data of the mediator data reception event
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок посредника
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 * \~english
				 * @brief Method of processing mediator error events
				 * @param eid         event identifier
				 * @param error       error type
				 * @param description error description
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек посредника
				 *
				 * @param eid идентификатор события посредника
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the mediator settings
				 * @param eid mediator event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки данных посредником
				 *
				 * @param eid    идентификатор события посредника
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных посредником
				 *
				 * \~english
				 * @brief Method of sending data by the mediator
				 * @param eid    mediator event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of data bytes sent by the mediator
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перемещения данных между посредником и другим событием
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения перемещения
				 *
				 * \~english
				 * @brief Method of moving data between the mediator and another event
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of performing the move
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события посредника
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid mediator event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    mediator event identifier
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    mediator event identifier
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid    mediator event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of extracting the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @return        значение адреса посредника
				 *
				 * \~english
				 * @brief Method of getting the mediator address
				 * @param eid     mediator event identifier
				 * @param address mediator address type
				 * @return        value of the mediator address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   значение адреса посредника
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the mediator address
				 * @param eid     mediator event identifier
				 * @param address mediator address type
				 * @param value   value of the mediator address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   значение адреса посредника
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the mediator address
				 * @param eid     mediator event identifier
				 * @param address mediator address type
				 * @param value   value of the mediator address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   объект для извлечения адреса посредника
				 * @return        результат выполнения извлечения адреса посредника
				 *
				 * \~english
				 * @brief Method of getting the mediator address
				 * @param eid     mediator event identifier
				 * @param address mediator address type
				 * @param value   object for extracting the mediator address
				 * @return        result of extracting the mediator address
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
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
				 * @brief Метод уничтожения события посредника
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a mediator event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификатора посредника для перехвата пакетов тоннеля
				 *
				 * @param family семейство адресов
				 * @return       идентификатор созданного посредника
				 *
				 * \~english
				 * @brief Method of getting the mediator identifier for intercepting tunnel packets
				 * @param family address family
				 * @return       identifier of the created mediator
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family) noexcept;
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
				Mediator(const Mediator &) = delete;
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
				Mediator & operator = (const Mediator &) = delete;
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
				explicit Mediator(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Mediator() noexcept;
		} mediator_t;
	};
};

#endif // __AWH_UNIT_MEDIATOR__
