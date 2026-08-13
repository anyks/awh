/**
 * @file: client.hpp
 * @date: 2026-03-15
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля клиента — класс unit::Client,
 *        реализующий клиентскую логику подключения к удалённому узлу поверх движка ввода-вывода:
 *        установку соединения, обмен данными, переподключение и уведомление о событиях
 *
 * \~english
 * @brief Header file of the client module — the unit::Client class,
 *        which implements the client logic of connecting to a remote node on top of the input-output engine:
 *        the establishment of the connection, the data exchange, the reconnection and the notification about the events
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_CLIENT__
#define __AWH_UNIT_CLIENT__

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

/**
 * \~russian
 * @brief Пространство имён библиотеки
 *
 * \~english
 * @brief Library namespace
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
		 * @brief Класс клиента
		 *
		 * \~english
		 * @brief Client class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Client : public unit_t {
			private:
				// Список идентификаторов событий клиента
				unordered_set <event::id_t> _events;
			private:
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы клиента
				 *
				 * @param status статус запуска/остановки клиента
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the client
				 * @param status status of the launch/stop of the client
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события подключения клиента к удалённому узлу
				 *
				 * @param eid идентификатор события
				 * @param ok  результат подключения
				 *
				 * \~english
				 * @brief Method of processing the event of the connection of the client to a remote node
				 * @param eid event identifier
				 * @param ok  result of the connection
				 *
				 * \~
				 */
				void connect(const event::id_t eid, const bool ok) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий записи данных клиентом
				 *
				 * @param eid  идентификатор события
				 * @param size размер данных для записи
				 *
				 * \~english
				 * @brief Method of processing data write events of the client
				 * @param eid  event identifier
				 * @param size size of the data to be written
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса клиента
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус клиента
				 *
				 * \~english
				 * @brief Method of processing client status change events
				 * @param eid    event identifier
				 * @param status new client status
				 *
				 * \~
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки действий клиента
				 *
				 * @param eid    идентификатор события
				 * @param action действие клиента
				 *
				 * \~english
				 * @brief Method of processing the client actions
				 * @param eid    event identifier
				 * @param action client action
				 *
				 * \~
				 */
				void action(const event::id_t eid, const event::action_t action) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
				 *
				 * @param eid  идентификатор события
				 * @param info информационные метаданные о дейтаграммном пакете
				 *
				 * \~english
				 * @brief Method of processing the informational metadata about a datagram packet
				 * @param eid  event identifier
				 * @param info informational metadata about the datagram packet
				 *
				 * \~
				 */
				void traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий получения данных клиентом
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных клиентом
				 * @param size размер данных события получения данных клиентом
				 *
				 * \~english
				 * @brief Method of processing data reception events of the client
				 * @param eid  event identifier
				 * @param data data of the client data reception event
				 * @param size data size of the client data reception event
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных клиента
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 * \~english
				 * @brief Method of processing availability/unavailability events of the client outgoing data queue
				 * @param eid    event identifier
				 * @param status queue availability status
				 * @param size   size of the available queue data
				 *
				 * \~
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий истечения таймаута клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 * \~english
				 * @brief Method of processing client timeout expiration events
				 * @param eid    client identifier
				 * @param action action type for the expired timeout
				 * @param delay  timeout delay in milliseconds
				 * @return       whether the client should be terminated after the timeout has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок клиента
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 * \~english
				 * @brief Method of processing client error events
				 * @param eid         event identifier
				 * @param error       error type
				 * @param description error description
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события неотправленных данных клиента
				 *
				 * @param eid   идентификатор события
				 * @param error тип ошибки отправки данных
				 * @param data  данные, которые не получилось отправить
				 * @param size  размер данных, которые не получилось отправить
				 *
				 * \~english
				 * @brief Method of processing the event of the unsent data of the client
				 * @param eid   event identifier
				 * @param error error type of the data sending
				 * @param data  data that could not be sent
				 * @param size  size of the data that could not be sent
				 *
				 * \~
				 */
				void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки актуальности события
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 *
				 * \~english
				 * @brief Method of checking the relevance of an event
				 * @param eid event identifier
				 * @return    result of checking the relevance of the event
				 *
				 * \~
				 */
				bool isActual(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the client settings
				 * @param eid client event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод запуска работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of launching the work of the client
				 * @param eid client event identifier
				 * @return    result of performing the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приостановки работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 *
				 * \~english
				 * @brief Method of suspending the work of the client
				 * @param eid client event identifier
				 * @return    result of performing the suspension of the work
				 *
				 * \~
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 *
				 * \~english
				 * @brief Method of resuming the work of the client
				 * @param eid client event identifier
				 * @return    result of performing the resumption of the work
				 *
				 * \~
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отключения клиента от удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения отключения
				 *
				 * \~english
				 * @brief Method of disconnecting the client from the remote node
				 * @param eid client event identifier
				 * @return    result of performing the disconnection
				 *
				 * \~
				 */
				bool disconnect(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода мультиподключения клиентов к удалённым хостам
				 *
				 * @tparam Args список идентификаторов клиентов для подключения
				 *
				 * \~english
				 * @brief Template of the method of the multiconnection of the clients to remote hosts
				 * @tparam Args list of the identifiers of the clients to be connected
				 *
				 * \~
				 */
				template <typename... Args>
				/**
				 * \~russian
				 * @brief Метод мультиподключения клиентов к удалённым хостам
				 *
				 * @param args список идентификаторов событий для подключения
				 * @return     результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of the multiconnection of the clients to remote hosts
				 * @param args list of the event identifiers to be connected
				 * @return     result of performing the connection
				 *
				 * \~
				 */
				bool connect(Args&&... args) noexcept {
					// Выполняем подключение к списку удалённых серверов
					return this->connect({args...});
				}
				/**
				 * \~russian
				 * @brief Метод мультиподключения клиентов к удалённым хостам
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 * \~english
				 * @brief Method of the multiconnection of the clients to remote hosts
				 * @param ids list of the event identifiers to be connected
				 * @return    result of performing the connection
				 *
				 * \~
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных от удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from a remote node
				 * @param eid client event identifier
				 * @return    result of receiving the data
				 *
				 * \~
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных удалённому узлу
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, отправленных удалённому узлу
				 *
				 * \~english
				 * @brief Method of sending data to a remote node
				 * @param eid    client event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of bytes sent to the remote node
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 *
				 * \~english
				 * @brief Method of joining the data streams between two events
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции клиента
				 *
				 * \~english
				 * @brief Method of getting the client options
				 * @param eid client event identifier
				 * @return    client options
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции клиента для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client options
				 * @param eid     client event identifier
				 * @param options client options to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция клиента для установки
				 * @param mode   режим установки опции клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting a client option
				 * @param eid    client event identifier
				 * @param option client option to be set
				 * @param mode   mode of setting the client option
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 * @param eid client event identifier
				 * @return    metadata of the last received datagram packet
				 *
				 * \~
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 * @param eid client event identifier
				 * @return    number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 * @param eid  client event identifier
				 * @param hops number of the hops of the last received packet
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события клиента
				 * @return    максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops through which a packet can pass
				 * @param eid client event identifier
				 * @return    maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops through which a packet can pass
				 * @param eid  client event identifier
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    сетевой интерфейс клиента
				 *
				 * \~english
				 * @brief Method of getting the network interface of the client
				 * @param eid client event identifier
				 * @return    network interface of the client
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса клиента
				 *
				 * @param eid  идентификатор события клиента
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the client
				 * @param eid  client event identifier
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события
				 * @return    внутренний порт события
				 *
				 * \~english
				 * @brief Method of getting the internal port of the event
				 * @param eid event identifier
				 * @return    internal port of the event
				 *
				 * \~
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки внутреннего порта события
				 *
				 * @param eid  идентификатор события
				 * @param port внутренний порт события
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the internal port of the event
				 * @param eid  event identifier
				 * @param port internal port of the event
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setSourcePort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения порта удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    порт удалённого узла
				 *
				 * \~english
				 * @brief Method of getting the port of the remote node
				 * @param eid client event identifier
				 * @return    port of the remote node
				 *
				 * \~
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта удалённого узла
				 *
				 * @param eid  идентификатор события клиента
				 * @param port порт удалённого узла для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the remote node
				 * @param eid  client event identifier
				 * @param port port of the remote node to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setTargetPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid client event identifier
				 * @return    host address of the target machine
				 *
				 * \~
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    client event identifier
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
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param eid    client event identifier
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
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 * \~english
				 * @brief Method of getting the host address of the target machine
				 * @param eid    client event identifier
				 * @param target object for extracting the host address of the target machine
				 * @return       result of extracting the host address of the target machine
				 *
				 * \~
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @return        значение адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @return        value of the client address
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   value of the client address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   value of the client address
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   объект для извлечения адреса клиента
				 * @return        результат выполнения извлечения адреса клиента
				 *
				 * \~english
				 * @brief Method of getting the client address
				 * @param eid     client event identifier
				 * @param address client address type
				 * @param value   object for extracting the client address
				 * @return        result of extracting the client address
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 * @param eid client event identifier
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 * @param eid client event identifier
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the packet delivery mode of the client
				 * @param eid client event identifier
				 * @return    packet delivery mode (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов клиента
				 *
				 * @param eid      идентификатор события клиента
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the packet delivery mode of the client
				 * @param eid      client event identifier
				 * @param delivery packet delivery mode (unicast, multicast, broadcast)
				 * @return         result of performing the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 *
				 * \~english
				 * @brief Method of getting the buffer size of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       buffer size of the client
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the buffer size of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @param size   buffer size of the client
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 *
				 * \~english
				 * @brief Method of getting the usage mode of the read timeout of the event
				 * @param eid event identifier
				 * @return    usage mode of the read timeout of the event
				 *
				 * \~
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 * \~english
				 * @brief Method of setting the usage mode of the read timeout of the event
				 * @param eid   event identifier
				 * @param usage usage mode of the read timeout of the event (reusable or disposable)
				 *
				 * \~
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of the client
				 * @param eid    client event identifier
				 * @param action action type of the client
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the timeout of the client
				 * @param eid     client event identifier
				 * @param action  action type of the client
				 * @param timeout value of the timeout in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the client
				 * @param eid       client event identifier
				 * @param limiting  mode of limiting the bandwidth of the client (egress or ingress)
				 * @param bandwidth bandwidth of the client to be set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the keep-alive parameters for the client
				 * @param eid   client event identifier
				 * @param cnt   number of the keep-alive packets
				 * @param idle  idle time before sending the first keep-alive packet in seconds
				 * @param intvl interval between the keep-alive packets in seconds
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid client event identifier
				 * @return    DSCP value
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid  client event identifier
				 * @param dscp DSCP value
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события клиента
				 * @return    текущий режим обнаружения MTU
				 *
				 * \~english
				 * @brief Method of getting the discovery mode of the maximum packet size (MTU)
				 * @param eid client event identifier
				 * @return    current MTU discovery mode
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события клиента
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery mode of the maximum packet size (MTU)
				 * @param eid  client event identifier
				 * @param mode discovery mode of the maximum packet size (MTU)
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    client event identifier
				 * @param mode   activation/deactivation mode
				 * @param group  multicast group to be activated/deactivated
				 * @param source address of the network interface from which the subscription is performed
				 * @param port   port of the multicast group from which the subscription is performed
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки клиента
				 *
				 * \~english
				 * @brief Method of stopping the client
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска клиента
				 *
				 * \~english
				 * @brief Method of launching the client
				 *
				 * \~
				 */
				void start() noexcept;
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
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a client event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификатора клиента для выполнения запросов к серверу
				 *
				 * @param family   семейство адресов
				 * @param type     тип события
				 * @param protocol протокол события
				 * @return         идентификатор созданного клиента
				 *
				 * \~english
				 * @brief Method of getting the client identifier for performing requests to a server
				 * @param family   address family
				 * @param type     event type
				 * @param protocol event protocol
				 * @return         identifier of the created client
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
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
				Client(const Client &) = delete;
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
				Client & operator = (const Client &) = delete;
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
				explicit Client(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Client() noexcept;
		} client_t;
	};
};

#endif // __AWH_UNIT_CLIENT__
