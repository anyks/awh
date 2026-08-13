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
 * \~russian
 * @brief Заголовочный файл модуля туннеля — класс unit::Tunnel, создающий и обслуживающий виртуальный сетевой
 *        интерфейс TUN/TAP для построения VPN-каналов на всех поддерживаемых операционных системах
 *
 * \~english
 * @brief Header file of the tunnel module — the unit::Tunnel class, which creates and serves a virtual network
 *        TUN/TAP interface for building VPN channels on all the supported operating systems
 *
 * \~
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
		 * @brief Класс туннеля
		 *
		 * \~english
		 * @brief Tunnel class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Tunnel : public unit_t {
			private:
				// Список идентификаторов событий туннеля
				unordered_set <event::id_t> _events;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса туннеля
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус туннеля
				 *
				 * \~english
				 * @brief Method of processing tunnel status change events
				 * @param eid    event identifier
				 * @param status new tunnel status
				 *
				 * \~
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных туннеля
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 * \~english
				 * @brief Method of processing availability/unavailability events of the tunnel outgoing data queue
				 * @param eid    event identifier
				 * @param status queue availability status
				 * @param size   size of the available queue data
				 *
				 * \~
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок туннеля
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 * \~english
				 * @brief Method of processing tunnel error events
				 * @param eid         event identifier
				 * @param error       error type
				 * @param description error description
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий получения информации о пакетах в туннеле
				 *
				 * @param eid    идентификатор события туннеля
				 * @param mid    идентификатор события посредника
				 * @param action действие туннеля
				 * @param info   информация о пакетах в туннеле
				 *
				 * \~english
				 * @brief Method of processing events of receiving information about packets in the tunnel
				 * @param eid    tunnel event identifier
				 * @param mid    mediator event identifier
				 * @param action tunnel action
				 * @param info   information about the packets in the tunnel
				 *
				 * \~
				 */
				void info(const event::id_t eid, const event::id_t mid, const event::action_t action, const net::tun_info_t & info) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the tunnel settings
				 * @param eid tunnel event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки данных через туннель
				 *
				 * @param eid    идентификатор события туннеля
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных через туннель
				 *
				 * \~english
				 * @brief Method of sending data through the tunnel
				 * @param eid    tunnel event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of data bytes sent through the tunnel
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    опции туннеля
				 *
				 * \~english
				 * @brief Method of getting the tunnel options
				 * @param eid tunnel event identifier
				 * @return    tunnel options
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param options опции туннеля для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the tunnel options
				 * @param eid     tunnel event identifier
				 * @param options tunnel options to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции туннеля
				 *
				 * @param eid    идентификатор события туннеля
				 * @param option опция туннеля для установки
				 * @param mode   режим установки опции туннеля
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting a tunnel option
				 * @param eid    tunnel event identifier
				 * @param option tunnel option to be set
				 * @param mode   mode of setting the tunnel option
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса туннеля
				 *
				 * @param eid идентификатор события туннеля
				 * @return    сетевой интерфейс туннеля
				 *
				 * \~english
				 * @brief Method of getting the network interface of the tunnel
				 * @param eid tunnel event identifier
				 * @return    network interface of the tunnel
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса туннеля
				 *
				 * @param eid  идентификатор события туннеля
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the tunnel
				 * @param eid  tunnel event identifier
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события туннеля
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid tunnel event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события туннеля
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    tunnel event identifier
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
				 * @param eid    идентификатор события туннеля
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    tunnel event identifier
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
				 * @param eid    идентификатор события туннеля
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid    tunnel event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of extracting the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @return        значение адреса туннеля
				 *
				 * \~english
				 * @brief Method of getting the tunnel address
				 * @param eid     tunnel event identifier
				 * @param address tunnel address type
				 * @return        value of the tunnel address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   значение адреса туннеля
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the tunnel address
				 * @param eid     tunnel event identifier
				 * @param address tunnel address type
				 * @param value   value of the tunnel address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   значение адреса туннеля
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the tunnel address
				 * @param eid     tunnel event identifier
				 * @param address tunnel address type
				 * @param value   value of the tunnel address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса туннеля
				 *
				 * @param eid     идентификатор события туннеля
				 * @param address тип адреса туннеля
				 * @param value   объект для извлечения адреса туннеля
				 * @return        результат выполнения извлечения адреса туннеля
				 *
				 * \~english
				 * @brief Method of getting the tunnel address
				 * @param eid     tunnel event identifier
				 * @param address tunnel address type
				 * @param value   object for extracting the tunnel address
				 * @return        result of extracting the tunnel address
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события туннеля
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 * @param eid tunnel event identifier
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события туннеля
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 * @param eid tunnel event identifier
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
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
				 * @brief Метод уничтожения события туннеля
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a tunnel event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификатора туннеля
				 *
				 * @param family семейство адресов
				 * @return       идентификатор созданного туннеля
				 *
				 * \~english
				 * @brief Method of getting the tunnel identifier
				 * @param family address family
				 * @return       identifier of the created tunnel
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
				Tunnel(const Tunnel &) = delete;
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
				Tunnel & operator = (const Tunnel &) = delete;
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
				explicit Tunnel(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Tunnel() noexcept;
		} tunnel_t;
	};
};

#endif // __AWH_UNIT_TUNNEL__
