/**
 * @file: tunnel.hpp
 * @date: 2026-03-23
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля туннеля — класс unit::Tunnel, создающий и обслуживающий виртуальный сетевой
 *        интерфейс TUN/TAP для построения VPN-каналов на всех поддерживаемых операционных системах
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_TUNNEL__
#define __AWH_UNIT_TUNNEL__

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
		 * @brief Класс туннеля
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Tunnel : public unit_t {
			private:
				// Список идентификаторов событий туннеля
				unordered_set <event::id_t> _events;
			private:
				/**
				 * @brief Метод обработки событий изменения статуса туннеля
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус туннеля
				 *
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных туннеля
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий ошибок туннеля
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
				/**
				 * @brief Метод обработки событий получения информации о пакетах в туннеле
				 *
				 * @param eid    идентификатор события туннеля
				 * @param mid    идентификатор события посредника
				 * @param action действие туннеля
				 * @param info   информация о пакетах в туннеле
				 *
				 */
				void info(const event::id_t eid, const event::id_t mid, const event::action_t action, const net::tun_info_t & info) noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    результат выполнения фиксации
				 *
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод отправки данных через туннель
				 *
				 * @param eid    идентификатор события туннеля
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных через туннель
				 *
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения опций туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    опции туннеля
				 *
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param options опции туннеля для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции туннеля
				 *
				 * @param eid    идентификатор события туннеля
				 * @param option опция туннеля для установки
				 * @param mode   режим установки опции туннеля
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    сетевой интерфейс туннеля
				 *
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса туннеля
				 *
				 * @param eid  идентификатор события туннеля
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события туннеля
				 * @return    адрес хоста целевой машины
				 *
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события туннеля
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
			public:
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события туннеля
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события туннеля
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @return        значение адреса туннеля
				 *
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   значение адреса туннеля
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   значение адреса туннеля
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   объект для извлечения адреса туннеля
				 * @return        результат выполнения извлечения адреса туннеля
				 *
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события туннеля
				 * @return    MTU сетевого интерфейса
				 *
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события туннеля
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept;
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
				 * @brief Метод уничтожения события туннеля
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора туннеля
				 *
				 * @param family семейство адресов
				 * @return       идентификатор созданного туннеля
				 *
				 */
				event::id_t issue(const event::family_t family) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Tunnel(const Tunnel &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Tunnel & operator = (const Tunnel &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Tunnel(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Tunnel() noexcept;
		} tunnel_t;
	};
};

#endif // __AWH_UNIT_TUNNEL__
