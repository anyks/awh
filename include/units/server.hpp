/**
 * @file: server.hpp
 * @date: 2026-03-22
 * @license: GPL-3.0
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
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_SERVER__
#define __AWH_UNIT_SERVER__

/**
 * Стандартный заголовочный файл
 */
#include <list>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "unit.hpp"
#include "cluster.hpp"

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
		 * @brief Класс сервера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Server : public unit_t {
			private:
				/**
				 * @brief Структура параметров кластера
				 *
				 * @details Параметры кластера задаются при его создании и не могут быть изменены в процессе работы.
				 */
				typedef struct __AWH_SHARED_EXPORT__ ClusterParams {
					// Имя кластера
					string name;
					// Флаг пересоздания процесса при его завершении
					bool rebirth;
					// Максимальное количество процессов в кластере
					uint16_t count;
					// Максимальное число подряд идущих быстрых падений процессов до остановки кластера (0 — без ограничения, по умолчанию 10)
					uint16_t restartLimit;
					// Временное окно «быстрого» (раннего) падения процесса в миллисекундах (по умолчанию 30000)
					uint64_t restartWindow;
					// Режим активации кластера (по умолчанию event::mode_t::DISABLED)
					event::mode_t mode;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit ClusterParams() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~ClusterParams() noexcept = default;
				} cluster_params_t;
			private:
				// Объект работы с кластером
				unique_ptr <cluster_t> _cluster;
			private:
				// Параметры кластера
				cluster_params_t _clusterParams;
			private:
				// Список идентификаторов событий сервера
				unordered_map <event::id_t, event::id_t> _events;
			private:
				// Список клиентов по идентификатору серверного события
				unordered_map <event::id_t, list <event::id_t>> _serverClients;
			private:
				// Позиция клиента в списке клиентов его сервера
				unordered_map <event::id_t, list <event::id_t>::iterator> _clientPositions;
			private:
				/**
				 * @brief Метод удаления связи клиента с сервером
				 *
				 * @param cid идентификатор клиентского события
				 */
				void unlinkClient(const event::id_t cid) noexcept;
				/**
				 * @brief Метод удаления всех клиентов серверного события
				 *
				 * @param sid идентификатор серверного события
				 */
				void unlinkServerClients(const event::id_t sid) noexcept;
				/**
				 * @brief Метод регистрации связи клиента с сервером
				 *
				 * @param sid идентификатор серверного события
				 * @param cid идентификатор клиентского события
				 */
				void linkClient(const event::id_t sid, const event::id_t cid) noexcept;
			private:
				/**
				 * @brief Метод запуска/остановки работы сервера
				 *
				 * @param status статус запуска/остановки сервера
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки события пересоздания процесса
				 *
				 * @param old старый идентификатор процесса
				 * @param pid текущий идентификатор процесса
				 */
				void rebase(const pid_t old, const pid_t pid) noexcept;
			private:
				/**
				 * @brief Метод получения события завершения работы процесса
				 *
				 * @param pid    идентификатор процесса
				 * @param signal сигнал с которым завершился процесс
				 */
				void exit(const pid_t pid, const int32_t signal) noexcept;
			private:
				/**
				 * @brief Метод обработки события отправки сообщения процессу кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param size размер отправленного сообщения
				 */
				void sending(const pid_t pid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий записи данных сервером
				 *
				 * @param eid  идентификатор события
				 * @param size размер данных для записи
				 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
				 */
				void write(const event::id_t eid, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки события разрешения подключения
				 *
				 * @param eid идентификатор сервера
				 * @param cid идентификатор клиента
				 */
				void accept(const event::id_t eid, const event::id_t cid) noexcept;
			private:
				/**
				 * @brief Метод обработки действий сервера
				 *
				 * @param eid    идентификатор события
				 * @param action действие сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 */
				void action(const event::id_t eid, const event::action_t action, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки событий изменения статуса кластера
				 *
				 * @param pid    идентификатор события
				 * @param status новый статус кластера
				 */
				void status(const pid_t pid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий изменения статуса сервера
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 */
				void status(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки информационных метаданных о дейтаграммном пакете
				 *
				 * @param eid  идентификатор события
				 * @param info информационные метаданные о дейтаграммном пакете
				 */
				void traffic(const event::id_t eid, const net::dgram_info_t & info) noexcept;
			private:
				/**
				 * @brief Метод получения событий активации/деактивации кластера
				 *
				 * @param pid   идентификатор процесса
				 * @param event флаг события кластера
				 */
				void cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
			private:
				/**
				 * @brief Метод обработки события получения сообщения от процесса кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param data данные полученного сообщения
				 * @param size размер данных полученного сообщения
				 */
				void message(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий получения данных сервером
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных сервером
				 * @param size размер данных события получения данных сервером
				 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 */
				void available(const pid_t pid, const event::status_t status, const size_t size) noexcept;
				/**
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных сервера
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки событий истечения таймаута подключённого клиента
				 *
				 * @param eid    идентификатор подключённого клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 * @return       нужно ли завершить клиента после истечения таймаута
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки событий ошибок кластера
				 *
				 * @param pid         идентификатор процесса
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 */
				void error(const pid_t pid, const event::error_t error, const string & description) noexcept;
				/**
				 * @brief Метод обработки событий ошибок сервера
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 * @param ctx         промежуточный контекст для передачи в функцию обратного вызова
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки события неотправленных данных сервера
				 *
				 * @param eid   идентификатор события
				 * @param error тип ошибки отправки данных
				 * @param data  данные, которые не получилось отправить
				 * @param size  размер данных, которые не получилось отправить
				 * @param ctx   промежуточный контекст для передачи в функцию обратного вызова
				 */
				void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size, void * ctx) noexcept;
			public:
				/**
				 * @brief Метод проверки актуальности события
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 */
				bool isActual(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод очистки чёрного списка события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения очистки
				 */
				bool clearBlacklist(const event::id_t eid) noexcept;
				/**
				 * @brief Метод очистки белого списка события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения очистки
				 */
				bool clearWhitelist(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса в чёрный список события
				 *
				 * @param eid   идентификатор события
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 */
				bool addToBlacklist(const event::id_t eid, string_view value) noexcept;
				/**
				 * @brief Метод добавления адреса в белый список события
				 *
				 * @param eid   идентификатор события
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 */
				bool addToWhitelist(const event::id_t eid, string_view value) noexcept;
			public:
				/**
				 * @brief Метод удаления адреса из чёрного списка события
				 *
				 * @param eid   идентификатор события
				 * @param value адрес для удаления из чёрного списка
				 * @return      результат выполнения удаления
				 */
				bool removeFromBlacklist(const event::id_t eid, string_view value) noexcept;
				/**
				 * @brief Метод удаления адреса из белого списка события
				 *
				 * @param eid   идентификатор события
				 * @param value адрес для удаления из белого списка
				 * @return      результат выполнения удаления
				 */
				bool removeFromWhitelist(const event::id_t eid, string_view value) noexcept;
			public:
				/**
				 * @brief Метод получения чёрного списка события
				 *
				 * @param eid идентификатор события
				 * @return    чёрный список события
				 */
				const unordered_map <string, event::address_t> & getFromBlacklist(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения белого списка события
				 *
				 * @param eid идентификатор события
				 * @return    белый список события
				 */
				const unordered_map <string, event::address_t> & getFromWhitelist(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения фиксации
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод запуска работы сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения запуска
				 */
				bool launch(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод приостановки обработки события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения приостановки
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления обработки события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения возобновления
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод установки промежуточного контекста события подключённого клиента
				 *
				 * @param eid идентификатор события сервера
				 * @param ctx указатель на контекст события
				 * @return    результат выполнения установки
				 */
				bool setContext(const event::id_t eid, void * ctx) noexcept;
			public:
				/**
				 * @brief Метод перевода события в режим прослушивания входящих соединений
				 *
				 * @param eid идентификатор события сервера
				 * @param max максимальное количество входящих соединений
				 * @return    результат выполнения перевода в режим прослушивания
				 */
				bool listen(const event::id_t eid, const uint16_t max) noexcept;
			public:
				/**
				 * @brief Метод получения данных от клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * @brief Метод отправки данных клиенту
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных клиенту
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод объединения данных между сервером и другим событием
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения объединения
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * @brief Метод получения опций сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    опции сервера
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param options опции сервера для установки
				 * @return        результат выполнения установки
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param option опция сервера для установки
				 * @param mode   режим установки опции сервера
				 * @return       результат выполнения установки
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    количество хопов последнего принятого пакета
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события сервера
				 * @return    максимальное количество хопов
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения порта сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    порт сервера
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @return        значение адреса сервера
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   объект для извлечения адреса сервера
				 * @return        результат выполнения извлечения адреса сервера
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @return    MTU сетевого интерфейса
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов сервера
				 *
				 * @param eid      идентификатор события сервера
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       размер буфера сервера
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @param size   размер буфера сервера
				 * @return       результат выполнения установки
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       значение таймаута в миллисекундах
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param action  тип действия сервера
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сервера
				 *
				 * @param eid       идентификатор события сервера
				 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
				 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод установки параметров keep-alive для сервера
				 *
				 * @param eid   идентификатор события сервера
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    значение DSCP
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим обнаружения максимального размера пакета (MTU)
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события сервера
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
			public:
				/**
				 * @brief Метод очистки событий сервера
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод остановки сервера
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска сервера
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события сервера
				 *
				 * @param eid идентификатор события для уничтожения
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод создания серверного события
				 *
				 * @param family   семейство адресов
				 * @param type     тип события
				 * @param protocol протокол события
				 * @return         идентификатор созданного серверного события
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 */
				void clusterName(string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 */
				cluster_t::family_t clusterFamily() const noexcept;
			public:
				/**
				 * @brief Метод получения режима активации кластера
				 *
				 * @return режим активации кластера
				 */
				event::mode_t clusterMode() const noexcept;
				/**
				 * @brief Метод установки режима работы кластера
				 *
				 * @param mode режим активации/деактивации кластера
				 */
				void clusterMode(const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 */
				uint16_t clusterCount() const noexcept;
				/**
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 */
				void clusterCount(const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 */
				unordered_set <pid_t> clusterWorkers() const noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterSend(const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 */
				void clusterRebirth(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
				 */
				void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 */
				event::type_t clusterGetTypeEventMessage() const noexcept;
				/**
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 */
				void clusterSetTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 */
				size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 */
				bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * @brief Конструктор копирования удалён
				 *
				 */
				Server(const Server &) = delete;
				/**
				 * @brief Оператор копирующего присваивания удалён
				 *
				 * @return текущее значение объекта
				 */
				Server & operator = (const Server &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Server(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Server() noexcept;
		} server_t;
	};
};

#endif // __AWH_UNIT_SERVER__
