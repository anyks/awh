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
 * @brief Заголовочный файл модуля клиента — класс unit::Client,
 *        реализующий клиентскую логику подключения к удалённому узлу поверх движка ввода-вывода:
 *        установку соединения, обмен данными, переподключение и уведомление о событиях
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
 * @brief Пространство имён библиотеки
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
		 * @brief Класс клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Client : public unit_t {
			private:
				// Список идентификаторов событий клиента
				unordered_set <event::id_t> _events;
			private:
				/**
				 * @brief Метод запуска/остановки работы клиента
				 *
				 * @param status статус запуска/остановки клиента
				 *
				 */
				void launch(const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки события подключения клиента к удалённому узлу
				 *
				 * @param eid идентификатор события
				 * @param ok  результат подключения
				 *
				 */
				void connect(const event::id_t eid, const bool ok) noexcept;
				/**
				 * @brief Метод обработки событий записи данных клиентом
				 *
				 * @param eid  идентификатор события
				 * @param size размер данных для записи
				 *
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий изменения статуса клиента
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус клиента
				 *
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки действий клиента
				 *
				 * @param eid    идентификатор события
				 * @param action действие клиента
				 *
				 */
				void action(const event::id_t eid, const event::action_t action) noexcept;
				/**
				 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
				 *
				 * @param eid  идентификатор события
				 * @param info информационные метаданные о дейтаграммном пакете
				 *
				 */
				void traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept;
				/**
				 * @brief Метод обработки событий получения данных клиентом
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных клиентом
				 * @param size размер данных события получения данных клиентом
				 *
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных клиента
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий истечения таймаута клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * @brief Метод обработки событий ошибок клиента
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
				/**
				 * @brief Метод обработки события неотправленных данных клиента
				 *
				 * @param eid   идентификатор события
				 * @param error тип ошибки отправки данных
				 * @param data  данные, которые не получилось отправить
				 * @param size  размер данных, которые не получилось отправить
				 *
				 */
				void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод проверки актуальности события
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 *
				 */
				bool isActual(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения фиксации
				 *
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод запуска работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения запуска
				 *
				 */
				bool launch(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод приостановки работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 *
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления работы клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 *
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод отключения клиента от удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения отключения
				 *
				 */
				bool disconnect(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Шаблон метода мультиподключения клиентов к удалённым хостам
				 *
				 * @tparam Args список идентификаторов клиентов для подключения
				 *
				 */
				template <typename... Args>
				/**
				 * @brief Метод мультиподключения клиентов к удалённым хостам
				 *
				 * @param args список идентификаторов событий для подключения
				 * @return     результат выполнения подключения
				 *
				 */
				bool connect(Args&&... args) noexcept {
					// Выполняем подключение к списку удалённых серверов
					return this->connect({args...});
				}
				/**
				 * @brief Метод мультиподключения клиентов к удалённым хостам
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * @brief Метод получения данных от удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * @brief Метод отправки данных удалённому узлу
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, отправленных удалённому узлу
				 *
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 *
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * @brief Метод получения опций клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции клиента
				 *
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции клиента для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция клиента для установки
				 * @param mode   режим установки опции клиента
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    количество хопов последнего принятого пакета
				 *
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события клиента
				 * @return    максимальное количество хопов
				 *
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    сетевой интерфейс клиента
				 *
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса клиента
				 *
				 * @param eid  идентификатор события клиента
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события
				 * @return    внутренний порт события
				 *
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки внутреннего порта события
				 *
				 * @param eid  идентификатор события
				 * @param port внутренний порт события
				 * @return     результат выполнения установки
				 *
				 */
				bool setSourcePort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения порта удалённого узла
				 *
				 * @param eid идентификатор события клиента
				 * @return    порт удалённого узла
				 *
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта удалённого узла
				 *
				 * @param eid  идентификатор события клиента
				 * @param port порт удалённого узла для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setTargetPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 *
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
			public:
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @return        значение адреса клиента
				 *
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   объект для извлечения адреса клиента
				 * @return        результат выполнения извлечения адреса клиента
				 *
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @return    MTU сетевого интерфейса
				 *
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов клиента
				 *
				 * @param eid      идентификатор события клиента
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 *
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 *
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 *
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 *
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    значение DSCP
				 *
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события клиента
				 * @return    текущий режим обнаружения MTU
				 *
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события клиента
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
			public:
				/**
				 * @brief Метод остановки клиента
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска клиента
				 *
				 */
				void start() noexcept;
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
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора клиента для выполнения запросов к серверу
				 *
				 * @param family   семейство адресов
				 * @param type     тип события
				 * @param protocol протокол события
				 * @return         идентификатор созданного клиента
				 *
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Client(const Client &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Client & operator = (const Client &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Client(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Client() noexcept;
		} client_t;
	};
};

#endif // __AWH_UNIT_CLIENT__
