/**
 * @file server.hpp
 * @date 2026-03-22
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
 * @brief Заголовочный файл модуля сервера — класс unit::Server, реализующий приём и обслуживание входящих подключений
 *        поверх движка ввода-вывода с поддержкой кластерного режима и управлением жизненным циклом клиентов
 *
 * \~english
 * @brief Header file of the server module — the unit::Server class, which implements the acceptance and the servicing of the incoming connections
 *        on top of the input-output engine with support for the cluster mode and with management of the life cycle of the clients
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
		 * @brief Класс сервера
		 *
		 * \~english
		 * @brief Server class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Server : public unit_t {
			private:
				/**
				 * \~russian
				 * @brief Структура параметров кластера
				 *
				 * @details Параметры кластера задаются при его создании и не могут быть изменены в процессе работы.
				 *
				 * \~english
				 * @brief Structure of the parameters of the cluster
				 * @details The parameters of the cluster are given at its creation and cannot be changed in the course of the work.
				 *
				 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit ClusterParams() noexcept;
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
			/**
			 * Для операционной системы MS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				private:
					/**
					 * Признак просьбы работника о передаче слушающих событий
					 *
					 * @details Работник просит передачу сам, и просьба эта служит мастеру
					 *          признаком связанности канала: порождение работника связи
					 *          ещё не означает, а написанное работником - означает
					 *
					 * @note Значение выбрано непечатным намеренно: просьба уходит первым
					 *       сообщением работника и с полезной нагрузкой не смешивается
					 */
					static constexpr uint8_t HANDOVER_REQUEST = 0x01;
					/**
					 * Список слушающих событий работника, ждущих передачи от мастера
					 *
					 * @details Работник кластера своего слушающего сокета не заводит: он
					 *          получает его снимком от мастера. Прослушивание оттого
					 *          откладывается - событие с числом подключений в очереди
					 *          запоминается здесь и поднимается по приходу снимка
					 */
					unordered_map <event::id_t, uint32_t> _handover;
			#endif
			private:
				// Список клиентов по идентификатору серверного события
				unordered_map <event::id_t, list <event::id_t>> _serverClients;
			private:
				// Позиция клиента в списке клиентов его сервера
				unordered_map <event::id_t, list <event::id_t>::iterator> _clientPositions;
			private:
				/**
				 * \~russian
				 * @brief Метод удаления связи клиента с сервером
				 *
				 * @param cid идентификатор клиентского события
				 *
				 * \~english
				 * @brief Method of removing the link of a client with the server
				 * @param cid identifier of the client event
				 *
				 * \~
				 */
				void unlinkClient(const event::id_t cid) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления всех клиентов серверного события
				 *
				 * @param sid идентификатор серверного события
				 *
				 * \~english
				 * @brief Method of removing all the clients of a server event
				 * @param sid identifier of the server event
				 *
				 * \~
				 */
				void unlinkServerClients(const event::id_t sid) noexcept;
				/**
				 * \~russian
				 * @brief Метод регистрации связи клиента с сервером
				 *
				 * @param sid идентификатор серверного события
				 * @param cid идентификатор клиентского события
				 *
				 * \~english
				 * @brief Method of registering the link of a client with the server
				 * @param sid identifier of the server event
				 * @param cid identifier of the client event
				 *
				 * \~
				 */
				void linkClient(const event::id_t sid, const event::id_t cid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы сервера
				 *
				 * @param status статус запуска/остановки сервера
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the server
				 * @param status status of the launch/stop of the server
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события пересоздания процесса
				 *
				 * @param old старый идентификатор процесса
				 * @param pid текущий идентификатор процесса
				 *
				 * \~english
				 * @brief Method of processing the event of the recreation of a process
				 * @param old old process identifier
				 * @param pid current process identifier
				 *
				 * \~
				 */
				void rebase(const pid_t old, const pid_t pid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод получения события завершения работы процесса
				 *
				 * @param pid    идентификатор процесса
				 * @param status состояние, с которым завершился процесс
				 *
				 * \~english
				 * @brief Method of receiving the event of the termination of the work of a process
				 * @param pid    process identifier
				 * @param status state with which the process has terminated
				 *
				 * \~
				 */
				void exit(const pid_t pid, const int32_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события отправки сообщения процессу кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param size размер отправленного сообщения
				 *
				 * \~english
				 * @brief Method of processing the event of sending a message to a process of the cluster
				 * @param pid  process identifier
				 * @param size size of the sent message
				 *
				 * \~
				 */
				void sending(const pid_t pid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий записи данных сервером
				 *
				 * @param eid  идентификатор события
				 * @param size размер данных для записи
				 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing data write events of the server
				 * @param eid  event identifier
				 * @param size size of the data to be written
				 * @param ctx  intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события разрешения подключения
				 *
				 * @param eid идентификатор сервера
				 * @param cid идентификатор клиента
				 *
				 * \~english
				 * @brief Method of processing the event of permitting a connection
				 * @param eid server identifier
				 * @param cid client identifier
				 *
				 * \~
				 */
				void accept(const event::id_t eid, const event::id_t cid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки действий сервера
				 *
				 * @param eid    идентификатор события
				 * @param action действие сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing the server actions
				 * @param eid    event identifier
				 * @param action server action
				 * @param ctx    intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void action(const event::id_t eid, const event::action_t action, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса кластера
				 *
				 * @param pid    идентификатор события
				 * @param status новый статус кластера
				 *
				 * \~english
				 * @brief Method of processing cluster status change events
				 * @param pid    event identifier
				 * @param status new cluster status
				 *
				 * \~
				 */
				void status(const pid_t pid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий изменения статуса сервера
				 *
				 * @param eid    идентификатор события
				 * @param status новый статус сервера
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing server status change events
				 * @param eid    event identifier
				 * @param status new server status
				 * @param ctx    intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void status(const event::id_t eid, const event::status_t status, void * ctx) noexcept;
			private:
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
			private:
				/**
				 * \~russian
				 * @brief Метод получения событий активации/деактивации кластера
				 *
				 * @param pid   идентификатор процесса
				 * @param event флаг события кластера
				 *
				 * \~english
				 * @brief Method of receiving the activation/deactivation events of the cluster
				 * @param pid   process identifier
				 * @param event flag of the cluster event
				 *
				 * \~
				 */
				void cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
			/**
			 * Для операционной системы MS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				private:
					/**
					 * \~russian
					 * @brief Метод передачи слушающих событий работнику кластера
					 *
					 * @details Ветвления у этой системы нет, и по наследству слушающее
					 *          событие работнику не достаётся: мастер снимает с каждого
					 *          своего слушающего события переносимый снимок и отдаёт его
					 *          работнику первым же сообщением
					 *
					 * @warning Порядок здесь несущий: снимки уходят ПРЕЖДЕ отклика
					 *          потребителя о запуске работника, оттого опередить их
					 *          потребитель не может, и первым сообщением работника всегда
					 *          оказывается передача событий
					 *
					 * @param pid идентификатор процесса работника
					 *
					 * \~english
					 * @brief Method of the handover of the listening events to a worker of the cluster
					 * @param pid identifier of the process of the worker
					 *
					 * \~
					 */
					void handover(const pid_t pid) noexcept;
					/**
					 * \~russian
					 * @brief Метод подъёма слушающих событий из снимков, присланных мастером
					 *
					 * @details Разбирает первое сообщение канала обмена и поднимает по нему
					 *          свои слушающие события: своего сокета работник не заводит
					 *          вовсе - у этой системы нет ни `SO_REUSEPORT`, ни годного
					 *          `SO_REUSEADDR`, и привязка работника к тому же порту была
					 *          бы не разделением работы, а перехватом
					 *
					 * @param data данные полученного сообщения
					 * @param size размер данных полученного сообщения
					 * @return     признак того, что сообщение было передачей событий
					 *
					 * \~english
					 * @brief Method of the raising of the listening events from the snapshots sent by the master
					 * @param data data of the received message
					 * @param size data size of the received message
					 * @return     flag that the message was a handover of the events
					 *
					 * \~
					 */
					bool handover(const uint8_t * data, const size_t size) noexcept;
			#endif
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события получения сообщения от процесса кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param data данные полученного сообщения
				 * @param size размер данных полученного сообщения
				 *
				 * \~english
				 * @brief Method of processing the event of receiving a message from a process of the cluster
				 * @param pid  process identifier
				 * @param data data of the received message
				 * @param size data size of the received message
				 *
				 * \~
				 */
				void message(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий получения данных сервером
				 *
				 * @param eid  идентификатор события
				 * @param data данные события получения данных сервером
				 * @param size размер данных события получения данных сервером
				 * @param ctx  промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing data reception events of the server
				 * @param eid  event identifier
				 * @param data data of the server data reception event
				 * @param size data size of the server data reception event
				 * @param ctx  intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 *
				 * \~english
				 * @brief Method of processing availability/unavailability events of the outgoing message queue of the cluster
				 * @param pid    process identifier
				 * @param status queue availability status
				 * @param size   size of the available queue data
				 *
				 * \~
				 */
				void available(const pid_t pid, const event::status_t status, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки события доступности/недоступности очереди исходящих данных сервера
				 *
				 * @param eid    идентификатор события
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing availability/unavailability events of the server outgoing data queue
				 * @param eid    event identifier
				 * @param status queue availability status
				 * @param size   size of the available queue data
				 * @param ctx    intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий истечения таймаута подключённого клиента
				 *
				 * @param eid    идентификатор подключённого клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  задержка таймаута в миллисекундах
				 * @param ctx    промежуточный контекст для передачи в функцию обратного вызова
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 * \~english
				 * @brief Method of processing timeout expiration events of a connected client
				 * @param eid    identifier of the connected client
				 * @param action action type for the expired timeout
				 * @param delay  timeout delay in milliseconds
				 * @param ctx    intermediate context for passing into the callback function
				 * @return       whether the client should be terminated after the timeout has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок кластера
				 *
				 * @param pid         идентификатор процесса
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 *
				 * \~english
				 * @brief Method of processing cluster error events
				 * @param pid         process identifier
				 * @param error       error type
				 * @param description error description
				 *
				 * \~
				 */
				void error(const pid_t pid, const event::error_t error, const string & description) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий ошибок сервера
				 *
				 * @param eid         идентификатор события
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 * @param ctx         промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing server error events
				 * @param eid         event identifier
				 * @param error       error type
				 * @param description error description
				 * @param ctx         intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description, void * ctx) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки события неотправленных данных сервера
				 *
				 * @param eid   идентификатор события
				 * @param error тип ошибки отправки данных
				 * @param data  данные, которые не получилось отправить
				 * @param size  размер данных, которые не получилось отправить
				 * @param ctx   промежуточный контекст для передачи в функцию обратного вызова
				 *
				 * \~english
				 * @brief Method of processing the event of the unsent data of the server
				 * @param eid   event identifier
				 * @param error error type of the data sending
				 * @param data  data that could not be sent
				 * @param size  size of the data that could not be sent
				 * @param ctx   intermediate context for passing into the callback function
				 *
				 * \~
				 */
				void spool(const event::id_t eid, const event::send_error_t error, const uint8_t * data, const size_t size, void * ctx) noexcept;
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
				 * @brief Метод очистки чёрного списка события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения очистки
				 *
				 * \~english
				 * @brief Method of clearing the black list of an event
				 * @param eid event identifier
				 * @return    result of performing the clearing
				 *
				 * \~
				 */
				bool clearBlacklist(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки белого списка события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения очистки
				 *
				 * \~english
				 * @brief Method of clearing the white list of an event
				 * @param eid event identifier
				 * @return    result of performing the clearing
				 *
				 * \~
				 */
				bool clearWhitelist(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления адреса в чёрный список события
				 *
				 * @param eid   идентификатор события
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of adding an address to the black list of an event
				 * @param eid   event identifier
				 * @param value value of the event address
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool addToBlacklist(const event::id_t eid, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления адреса в белый список события
				 *
				 * @param eid   идентификатор события
				 * @param value значение адреса события
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of adding an address to the white list of an event
				 * @param eid   event identifier
				 * @param value value of the event address
				 * @return      result of performing the setting
				 *
				 * \~
				 */
				bool addToWhitelist(const event::id_t eid, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления адреса из чёрного списка события
				 *
				 * @param eid   идентификатор события
				 * @param value адрес для удаления из чёрного списка
				 * @return      результат выполнения удаления
				 *
				 * \~english
				 * @brief Method of removing an address from the black list of an event
				 * @param eid   event identifier
				 * @param value address to be removed from the black list
				 * @return      result of performing the removal
				 *
				 * \~
				 */
				bool removeFromBlacklist(const event::id_t eid, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления адреса из белого списка события
				 *
				 * @param eid   идентификатор события
				 * @param value адрес для удаления из белого списка
				 * @return      результат выполнения удаления
				 *
				 * \~english
				 * @brief Method of removing an address from the white list of an event
				 * @param eid   event identifier
				 * @param value address to be removed from the white list
				 * @return      result of performing the removal
				 *
				 * \~
				 */
				bool removeFromWhitelist(const event::id_t eid, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения чёрного списка события
				 *
				 * @param eid идентификатор события
				 * @return    чёрный список события
				 *
				 * \~english
				 * @brief Method of getting the black list of an event
				 * @param eid event identifier
				 * @return    black list of the event
				 *
				 * \~
				 */
				const unordered_map <string, event::address_t> & getFromBlacklist(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения белого списка события
				 *
				 * @param eid идентификатор события
				 * @return    белый список события
				 *
				 * \~english
				 * @brief Method of getting the white list of an event
				 * @param eid event identifier
				 * @return    white list of the event
				 *
				 * \~
				 */
				const unordered_map <string, event::address_t> & getFromWhitelist(const event::id_t eid) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод фиксации настроек сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения фиксации
				 *
				 * \~english
				 * @brief Method of committing the server settings
				 * @param eid server event identifier
				 * @return    result of performing the commit
				 *
				 * \~
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод запуска работы сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения запуска
				 *
				 * \~english
				 * @brief Method of launching the work of the server
				 * @param eid server event identifier
				 * @return    result of performing the launch
				 *
				 * \~
				 */
				bool launch(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приостановки обработки события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения приостановки
				 *
				 * \~english
				 * @brief Method of suspending the processing of an event
				 * @param eid event identifier
				 * @return    result of performing the suspension
				 *
				 * \~
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод возобновления обработки события
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения возобновления
				 *
				 * \~english
				 * @brief Method of resuming the processing of an event
				 * @param eid event identifier
				 * @return    result of performing the resumption
				 *
				 * \~
				 */
				bool resume(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки промежуточного контекста события подключённого клиента
				 *
				 * @param eid идентификатор события сервера
				 * @param ctx указатель на контекст события
				 * @return    результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the intermediate context of the event of a connected client
				 * @param eid server event identifier
				 * @param ctx pointer to the context of the event
				 * @return    result of performing the setting
				 *
				 * \~
				 */
				bool setContext(const event::id_t eid, void * ctx) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перевода события в режим прослушивания входящих соединений
				 *
				 * @param eid идентификатор события сервера
				 * @param max максимальное количество входящих соединений
				 * @return    результат выполнения перевода в режим прослушивания
				 *
				 * \~english
				 * @brief Method of switching an event into the mode of listening for incoming connections
				 * @param eid server event identifier
				 * @param max maximum number of the incoming connections
				 * @return    result of performing the switch into the listening mode
				 *
				 * \~
				 */
				bool listen(const event::id_t eid, const uint32_t max) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных от клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 *
				 * \~english
				 * @brief Method of receiving data from a client
				 * @param eid client event identifier
				 * @return    result of receiving the data
				 *
				 * \~
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки данных клиенту
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных клиенту
				 *
				 * \~english
				 * @brief Method of sending data to a client
				 * @param eid    client event identifier
				 * @param buffer data buffer to be sent
				 * @param size   size of the data to be sent
				 * @return       number of data bytes sent to the client
				 *
				 * \~
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод назначения источника данных для вытягивающей модели отправки
				 *
				 * @note Движок спрашивает данные у источника ровно тогда, когда готов их
				 *       отправить, поэтому при переполнении очереди данные не теряются
				 * @note Отказ источника гасит вытягивание: заводится оно заново пустой
				 *       отправкой `send(eid, nullptr, 0)`
				 * @param eid    идентификатор события клиента
				 * @param source функция обратного вызова источника данных
				 *
				 * \~english
				 * @brief Method of setting the source of the data for the pull model of the sending
				 * @note The engine asks the source for the data exactly when it is ready to send
				 *       them, so at an overflow of the queue the data are not lost
				 * @note A refusal of the source extinguishes the pulling: it is set going again
				 *       by an empty sending `send(eid, nullptr, 0)`
				 * @param eid    client event identifier
				 * @param source callback function of the source of the data
				 *
				 * \~
				 */
				void source(const event::id_t eid, engine::callback::source_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод объединения данных между сервером и другим событием
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения объединения
				 *
				 * \~english
				 * @brief Method of joining the data between the server and another event
				 * @param eid  identifier of the source event
				 * @param dest identifier of the destination event
				 * @return     result of performing the joining
				 *
				 * \~
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения опций сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    опции сервера
				 *
				 * \~english
				 * @brief Method of getting the server options
				 * @param eid server event identifier
				 * @return    server options
				 *
				 * \~
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опций сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param options опции сервера для установки
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the server options
				 * @param eid     server event identifier
				 * @param options server options to be set
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки опции сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param option опция сервера для установки
				 * @param mode   режим установки опции сервера
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting a server option
				 * @param eid    server event identifier
				 * @param option server option to be set
				 * @param mode   mode of setting the server option
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
				 * @param eid идентификатор события сервера
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 *
				 * \~english
				 * @brief Method of getting the metadata of the last received datagram packet
				 * @param eid server event identifier
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
				 * @param eid идентификатор события сервера
				 * @return    количество хопов последнего принятого пакета
				 *
				 * \~english
				 * @brief Method of getting the number of the hops of the last received packet
				 * @param eid server event identifier
				 * @return    number of the hops of the last received packet
				 *
				 * \~
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the number of the hops of the last received packet
				 * @param eid  server event identifier
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
				 * @param eid идентификатор события сервера
				 * @return    максимальное количество хопов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the hops through which a packet can pass
				 * @param eid server event identifier
				 * @return    maximum number of the hops
				 *
				 * \~
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the hops through which a packet can pass
				 * @param eid  server event identifier
				 * @param hops maximum number of the hops
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 *
				 * \~english
				 * @brief Method of getting the network interface of the server
				 * @param eid server event identifier
				 * @return    network interface of the server
				 *
				 * \~
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network interface of the server
				 * @param eid  server event identifier
				 * @param name name of the network interface to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения порта сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    порт сервера
				 *
				 * \~english
				 * @brief Method of getting the port of the server
				 * @param eid server event identifier
				 * @return    port of the server
				 *
				 * \~
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the port of the server
				 * @param eid  server event identifier
				 * @param port port of the server to be set
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @return        значение адреса сервера
				 *
				 * \~english
				 * @brief Method of getting the address of the server
				 * @param eid     server event identifier
				 * @param address address type of the server
				 * @return        value of the address of the server
				 *
				 * \~
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the server
				 * @param eid     server event identifier
				 * @param address address type of the server
				 * @param value   value of the address of the server
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the address of the server
				 * @param eid     server event identifier
				 * @param address address type of the server
				 * @param value   value of the address of the server
				 * @return        result of performing the setting
				 *
				 * \~
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   объект для извлечения адреса сервера
				 * @return        результат выполнения извлечения адреса сервера
				 *
				 * \~english
				 * @brief Method of getting the address of the server
				 * @param eid     server event identifier
				 * @param address address type of the server
				 * @param value   object for extracting the address of the server
				 * @return        result of extracting the address of the server
				 *
				 * \~
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @return    MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of getting the MTU of the network interface
				 * @param eid server event identifier
				 * @return    MTU of the network interface
				 *
				 * \~
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 * \~english
				 * @brief Method of setting the MTU of the network interface
				 * @param eid server event identifier
				 * @param mtu MTU size of the interface
				 * @return    result of setting the MTU of the network interface
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима трансляции пакетов сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 * \~english
				 * @brief Method of getting the packet delivery mode of the server
				 * @param eid server event identifier
				 * @return    packet delivery mode (unicast, multicast, broadcast)
				 *
				 * \~
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима трансляции пакетов сервера
				 *
				 * @param eid      идентификатор события сервера
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the packet delivery mode of the server
				 * @param eid      server event identifier
				 * @param delivery packet delivery mode (unicast, multicast, broadcast)
				 * @return         result of performing the setting
				 *
				 * \~
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       размер буфера сервера
				 *
				 * \~english
				 * @brief Method of getting the buffer size of the server
				 * @param eid    server event identifier
				 * @param action action type of the server
				 * @return       buffer size of the server
				 *
				 * \~
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @param size   размер буфера сервера
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the buffer size of the server
				 * @param eid    server event identifier
				 * @param action action type of the server
				 * @param size   buffer size of the server
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
				 * @brief Метод получения таймаута сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of getting the timeout of the server
				 * @param eid    server event identifier
				 * @param action action type of the server
				 * @return       value of the timeout in milliseconds
				 *
				 * \~
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки таймаута сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param action  тип действия сервера
				 * @param timeout значение таймаута в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the timeout of the server
				 * @param eid     server event identifier
				 * @param action  action type of the server
				 * @param timeout value of the timeout in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности сервера
				 *
				 * @param eid       идентификатор события сервера
				 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
				 * @param bandwidth пропускная способность сервера для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the server
				 * @param eid       server event identifier
				 * @param limiting  mode of limiting the bandwidth of the server (egress or ingress)
				 * @param bandwidth bandwidth of the server to be set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 * @return          result of performing the setting
				 *
				 * \~
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки параметров keep-alive для сервера
				 *
				 * @param eid   идентификатор события сервера
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the keep-alive parameters for the server
				 * @param eid   server event identifier
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
				 * @param eid идентификатор события сервера
				 * @return    значение DSCP
				 *
				 * \~english
				 * @brief Method of getting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid server event identifier
				 * @return    DSCP value
				 *
				 * \~
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the value of the Differentiated Services Code Point (DSCP) field in the header of an IP packet
				 * @param eid  server event identifier
				 * @param dscp DSCP value
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим обнаружения максимального размера пакета (MTU)
				 *
				 * \~english
				 * @brief Method of getting the discovery of the maximum packet size (MTU)
				 * @param eid server event identifier
				 * @return    discovery mode of the maximum packet size (MTU)
				 *
				 * \~
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события сервера
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 *
				 * \~english
				 * @brief Method of setting the discovery of the maximum packet size (MTU)
				 * @param eid  server event identifier
				 * @param mode discovery mode of the maximum packet size (MTU)
				 * @return     result of the work of the function
				 *
				 * \~
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации мультикаст группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    server event identifier
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
				 * @brief Метод активации/деактивации мультикаст группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of activating/deactivating a multicast group
				 * @param eid    server event identifier
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
				 * @brief Метод очистки событий сервера
				 *
				 * \~english
				 * @brief Method of clearing the events of the server
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки сервера
				 *
				 * \~english
				 * @brief Method of stopping the server
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска сервера
				 *
				 * \~english
				 * @brief Method of launching the server
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
				 * @brief Метод уничтожения события сервера
				 *
				 * @param eid идентификатор события для уничтожения
				 *
				 * \~english
				 * @brief Method of destroying a server event
				 * @param eid identifier of the event to be destroyed
				 *
				 * \~
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания серверного события
				 *
				 * @param family   семейство адресов
				 * @param type     тип события
				 * @param protocol протокол события
				 * @return         идентификатор созданного серверного события
				 *
				 * \~english
				 * @brief Method of creating a server event
				 * @param family   address family
				 * @param type     event type
				 * @param protocol event protocol
				 * @return         identifier of the created server event
				 *
				 * \~
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 *
				 * \~english
				 * @brief Method of setting the name of the cluster
				 * @param name name of the cluster to be set
				 *
				 * \~
				 */
				void clusterName(string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 *
				 * \~english
				 * @brief Method of getting the family of the cluster
				 * @return family to which the cluster belongs (MASTER or CHILDREN)
				 *
				 * \~
				 */
				cluster_t::family_t clusterFamily() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима активации кластера
				 *
				 * @return режим активации кластера
				 *
				 * \~english
				 * @brief Method of getting the activation mode of the cluster
				 * @return activation mode of the cluster
				 *
				 * \~
				 */
				event::mode_t clusterMode() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима работы кластера
				 *
				 * @param mode режим активации/деактивации кластера
				 *
				 * \~english
				 * @brief Method of setting the working mode of the cluster
				 * @param mode activation/deactivation mode of the cluster
				 *
				 * \~
				 */
				void clusterMode(const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the processes
				 * @return maximum number of the processes
				 *
				 * \~
				 */
				uint16_t clusterCount() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the processes
				 * @param count maximum number of the processes
				 *
				 * \~
				 */
				void clusterCount(const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 *
				 * \~english
				 * @brief Method of getting the list of the child processes
				 * @return list of the child processes
				 *
				 * \~
				 */
				unordered_set <pid_t> clusterWorkers() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to the parent process
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterSend(const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to a child process
				 * @param pid    identifier of the process for receiving the message
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to all the child processes
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 *
				 * \~english
				 * @brief Method of setting the flag of the automatic revival of the processes
				 * @param mode flag of the revival of the processes
				 *
				 * \~
				 */
				void clusterRebirth(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the parameters of the protection against a loop of restarts of the processes of the cluster
				 * @param limit  maximum number of consecutive fast falls before the cluster is stopped (0 — without a limit)
				 * @param window time window of a «fast» (early) fall of a process in milliseconds
				 *
				 * \~
				 */
				void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 *
				 * \~english
				 * @brief Method of getting the type of the data transfer protocol between the workers
				 * @return type of the data transfer protocol between the workers
				 *
				 * \~
				 */
				event::type_t clusterGetTypeEventMessage() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 *
				 * \~english
				 * @brief Method of setting the type of the data transfer protocol between the workers
				 * @param type type of the data transfer protocol between the workers to be set
				 *
				 * \~
				 */
				void clusterSetTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 * \~english
				 * @brief Method of getting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @return       event buffer size
				 *
				 * \~
				 */
				size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @param size   event buffer size
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования удалён
				 *
				 * \~english
				 * @brief Copy constructor deleted
				 *
				 * \~
				 */
				Server(const Server &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирующего присваивания удалён
				 *
				 * @return текущее значение объекта
				 *
				 * \~english
				 * @brief Copy assignment operator deleted
				 * @return current value of the object
				 *
				 * \~
				 */
				Server & operator = (const Server &) = delete;
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
				explicit Server(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Server() noexcept;
		} server_t;
	};
};

#endif // __AWH_UNIT_SERVER__
