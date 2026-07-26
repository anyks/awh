/**
 * @file: mediator.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_MEDIATOR__
#define __AWH_UNIT_MEDIATOR__

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
		 * @brief Класс посредника
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Mediator : public unit_t {
			private:
				// Список идентификаторов событий посредника
				unordered_set <event::id_t> _events;
			private:
				/**
				 * @brief Метод обработки действий посредника
				 *
				 * @param eid    идентификатор события
				 * @param action действие посредника
				 */
				void action(const event::id_t eid, const event::action_t action) noexcept;
				/**
				 * @brief Метод обработки событий изменения статуса посредника
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус посредника
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий получения данных посредником
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных посредником
				 * @param size размер данных события получения данных посредником
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий ошибок посредника
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек посредника
				 *
				 * @param eid идентификатор события посредника
				 * @return    результат выполнения фиксации
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод отправки данных посредником
				 *
				 * @param eid    идентификатор события посредника
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных посредником
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод перемещения данных между посредником и другим событием
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения перемещения
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события посредника
				 * @return    адрес хоста целевой машины
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
			public:
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события посредника
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @return        значение адреса посредника
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   значение адреса посредника
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   значение адреса посредника
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса посредника
				 *
				 * @param eid     идентификатор события посредника
				 * @param address тип адреса посредника
				 * @param value   объект для извлечения адреса посредника
				 * @return        результат выполнения извлечения адреса посредника
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события посредника
				 *
				 * @param eid идентификатор события для уничтожения
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора посредника для перехвата пакетов тоннеля
				 *
				 * @param family семейство адресов
				 * @return       идентификатор созданного посредника
				 */
				event::id_t issue(const event::family_t family) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Mediator(const Mediator &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				Mediator & operator = (const Mediator &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Mediator(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Mediator() noexcept;
		} mediator_t;
	};
};

#endif // __AWH_UNIT_MEDIATOR__
