/**
 * @file icmp.hpp
 * @date 2026-03-06
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
 * @brief Заголовочный файл модуля ICMP-клиента — класс unit::ICMP,
 *        выполняющий проверку доступности удалённого узла (ping) с контролем TTL, номеров последовательности,
 *        времени отклика и количества повторов
 *
 * \~english
 * @brief Header file of the ICMP client module — the unit::ICMP class,
 *        which checks the availability of a remote node (ping) with control over the TTL, the sequence numbers,
 *        the response time and the number of retries
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_ICMP__
#define __AWH_UNIT_ICMP__

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
		 * @brief Класс ICMP-клиента
		 *
		 * \~english
		 * @brief ICMP client class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ ICMP : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief Идентификатор ICMP-клиента
				 *
				 * \~english
				 * @brief ICMP client identifier
				 *
				 * \~
				 */
				using id_t = uint16_t;
			public:
				/**
				 * \~russian
				 * @brief Режим работы ICMP-клиента
				 *
				 * \~english
				 * @brief Working mode of the ICMP client
				 *
				 * \~
				 */
				enum class mode_t : uint8_t {
					SYNC  = 0x00, // Синхронный режим работы
					ASYNC = 0x01  // Асинхронный режим работы
				};
			public:
				/**
				 * \~russian
				 * @brief Структура ответа от удалённого сервера на запрос ICMP-клиента
				 *
				 * @details Содержит информацию о размере ответа, времени выполнения запроса,
				 *          индексе последовательности запроса, времени жизни пакета (TTL) и адресе удалённого сервера,
				 *          от которого пришёл ответ на запрос ICMP-клиента.
				 *
				 * \~english
				 * @brief Structure of the response of a remote server to a request of the ICMP client
				 * @details Contains information about the size of the response, the execution time of the request,
				 *          the sequence index of the request, the time to live of the packet (TTL) and the address of the remote server
				 *          from which the response to the request of the ICMP client has arrived.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Response {
					// Размер полученного ответа от удалённого сервера на запрос ICMP-клиента
					size_t size;
					// Время выполнения запроса в миллисекундах
					uint64_t elapsed;
					// Индекс последовательности запроса
					uint16_t sequence;
					// Время жизни пакета (TTL) в хопах
					uint32_t timeToLive;
					// Адрес удалённого сервера, от которого пришёл ответ на запрос ICMP-клиента
					net::addr_t * address;
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
					explicit Response() noexcept;
				} response_t;
			private:
				/**
				 * \~russian
				 * @brief Структура для управления состоянием ICMP-клиента
				 *
				 * @details Содержит информацию о времени ожидания ответа от удалённого сервера, идентификаторе события ICMP-клиента,
				 *          адресе удалённого сервера для выполнения запросов и локальном адресе, с которого выполняется запрос.
				 *
				 * \~english
				 * @brief Structure for managing the state of the ICMP client
				 * @details Contains information about the time of waiting for a response from the remote server, the event identifier of the ICMP client,
				 *          the address of the remote server for performing the requests and the local address from which the request is performed.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Client {
					// Время ожидания ответа от удалённого сервера (в миллисекундах, по умолчанию 5000 мс)
					uint32_t delay;
					// Идентификатор события для ICMP-клиента
					event::id_t eid;
					// Адрес удалённого сервера для выполнения запросов
					unique_ptr <net::addr_t> target;
					// Локальный адрес, с которого выполняется запрос
					unique_ptr <net::addr_t> source;
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
					explicit Client() noexcept;
				} client_t;
				/**
				 * \~russian
				 * @brief Структура для управления передачей данных при выполнении запросов ICMP
				 *
				 * @details Содержит информацию о текущем идентификаторе запроса, флаге ожидания ответа от удалённого сервера,
				 *          количестве повторений запросов, номере последовательности последнего отправленного запроса
				 *          и штампе времени начала запроса.
				 *
				 * \~english
				 * @brief Structure for managing the data transfer while performing ICMP requests
				 * @details Contains information about the current request identifier, the flag of waiting for a response from the remote server,
				 *          the number of request repetitions, the sequence number of the last sent request
				 *          and the timestamp of the beginning of the request.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Активный идентификатор запроса
					id_t id;
					// Флаг ожидания ответа от сервера
					bool waiting;
					// Количество повторений запросов
					uint16_t count;
					// Номер последовательности последнего отправленного запроса
					uint16_t sequence;
					// Штамп времени начала запроса
					uint64_t timestamp;
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
					explicit Transfer() noexcept;
				} transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние ICMP-клиента
				client_t _client;
			private:
				// Объект управления передачей данных при выполнении запросов ICMP
				transfer_t _transfer;
			private:
				// Адрес последнего ответа для передачи в callback ping
				unique_ptr <net::addr_t> _replyAddress;
			private:
				/**
				 * \~russian
				 * @brief Метод уничтожения события ICMP-клиента
				 *
				 * \~english
				 * @brief Method of destroying the event of the ICMP client
				 *
				 * \~
				 */
				void destroyClient() noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки ICMP Echo-запроса
				 *
				 * @param eid      идентификатор события ICMP-клиента
				 * @param id       идентификатор ICMP-запроса
				 * @param sequence номер последовательности запроса
				 * @return         количество отправленных байт
				 *
				 * \~english
				 * @brief Method of sending an ICMP Echo request
				 * @param eid      event identifier of the ICMP client
				 * @param id       identifier of the ICMP request
				 * @param sequence sequence number of the request
				 * @return         number of bytes sent
				 *
				 * \~
				 */
				size_t sendEcho(const event::id_t eid, const id_t id, const uint16_t sequence) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки ошибок событий ICMP-клиента
				 *
				 * @param eid         идентификатор события ICMP-клиента
				 * @param error       код ошибки события ICMP-клиента
				 * @param description описание ошибки события ICMP-клиента
				 *
				 * \~english
				 * @brief Method of processing the errors of the events of the ICMP client
				 * @param eid         event identifier of the ICMP client
				 * @param error       error code of the event of the ICMP client
				 * @param description error description of the event of the ICMP client
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки таймаута ожидания ответа ICMP-сервера
				 *
				 * @param eid    идентификатор события ICMP-клиента
				 * @param action действие события таймера ICMP-клиента
				 * @param delay  задержка таймера ICMP-клиента
				 * @return       нужно ли завершить клиента после истечения таймаута
				 *
				 * \~english
				 * @brief Method of processing the timeout of waiting for a response of the ICMP server
				 * @param eid    event identifier of the ICMP client
				 * @param action action of the timer event of the ICMP client
				 * @param delay  timer delay of the ICMP client
				 * @return       whether the client should be terminated after the timeout has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод продолжения ожидания своего ответа
				 *
				 * @details Сокет ICMP сырой, и приходит на него **весь** ICMP машины: чужие
				 *          отклики эха, извещения о недостижимости, обмен соседних опросов.
				 *          Всё это к заданному вопросу касательства не имеет и отбрасывается,
				 *          но отбрасывать пакет и прекращать ожидание - разные вещи
				 *
				 *          Движок же прекращает: приход данных ожидание чтения завершает, и
				 *          решает он это раньше, чем пакет разобран. Оттого всякий чужой
				 *          пакет прежде съедал ожидание, и опрос повисал навсегда, не дав ни
				 *          ответа, ни отказа. На машине сколько-нибудь занятой ICMP такое не
				 *          редкость, а обыденность
				 *
				 * @note Ожидание продолжается **остатком**, а не сроком полным: иначе поток
				 *       чужих пакетов отодвигал бы отказ без конца
				 *
				 * @param eid идентификатор события чтения ICMP-ответа
				 *
				 * \~english
				 * @brief Method of continuing to wait for one's own response
				 * @details The ICMP socket is a raw one, and **all** the ICMP traffic of the machine arrives at it: other
				 *          echo responses, unreachability notices, the exchange of the neighbour discovery.
				 *          All of that has no relation to the question that has been asked and is discarded,
				 *          but discarding a packet and terminating the waiting are different things
				 *          The engine, however, does terminate it: the arrival of data completes the read waiting, and
				 *          it decides that earlier than the packet is parsed. Because of that every foreign
				 *          packet used to eat up the waiting, and the poll hung forever, giving neither
				 *          a response nor a refusal. On a machine that is even slightly busy with ICMP such a thing is not
				 *          a rarity but an everyday occurrence
				 * @note The waiting is continued with the **remainder**, not with the full term: otherwise a stream
				 *       of foreign packets would postpone the refusal endlessly
				 * @param eid event identifier of the reading of the ICMP response
				 *
				 * \~
				 */
				void keepWaiting(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки ответов удалённого сервера на ICMP-запросы
				 *
				 * @param eid  идентификатор события чтения ICMP-ответа
				 * @param mode режим обработки события чтения ICMP-ответа
				 * @param data данные события чтения ICMP-ответа
				 * @param size размер данных события чтения ICMP-ответа
				 *
				 * \~english
				 * @brief Method of processing the responses of the remote server to the ICMP requests
				 * @param eid  event identifier of the reading of the ICMP response
				 * @param mode processing mode of the event of the reading of the ICMP response
				 * @param data data of the event of the reading of the ICMP response
				 * @param size data size of the event of the reading of the ICMP response
				 *
				 * \~
				 */
				void response(const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод инициализации события ICMP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 * @return       результат инициализации события ICMP-клиента
				 *
				 * \~english
				 * @brief Method of initializing the event of the ICMP client
				 * @param family family of the protocols (for example: IPv4 or IPv6)
				 * @return       result of initializing the event of the ICMP client
				 *
				 * \~
				 */
				bool init(const event::family_t family) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова (ping, timeout, datagram)
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions (ping, timeout, datagram)
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки таймаута для ожидания ответа от сервера
				 *
				 * @param delay время ожидания ответа от сервера (в миллисекундах)
				 *
				 * \~english
				 * @brief Method of setting the timeout for waiting for a response from the server
				 * @param delay time of waiting for a response from the server (in milliseconds)
				 *
				 * \~
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа события
				 *
				 * @return тип события
				 *
				 * \~english
				 * @brief Method of getting the event type
				 * @return event type
				 *
				 * \~
				 */
				event::type_t type() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения типа узла события
				 *
				 * @return тип узла события
				 *
				 * \~english
				 * @brief Method of getting the unit type of the event
				 * @return unit type of the event
				 *
				 * \~
				 */
				event::node_t node() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения семейства события
				 *
				 * @return семейство адресов
				 *
				 * \~english
				 * @brief Method of getting the family of the event
				 * @return address family
				 *
				 * \~
				 */
				event::family_t family() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения статуса события
				 *
				 * @return статус события
				 *
				 * \~english
				 * @brief Method of getting the event status
				 * @return event status
				 *
				 * \~
				 */
				event::status_t status() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(string_view target) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const net::addr_t * target) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the host address of the target machine
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param target host address of the target machine
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setTarget(const event::family_t family, string_view target) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param source network address for performing the request
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setSource(string_view source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param source network address for performing the request
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setSource(const net::addr_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сети, с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param source network address for performing the request
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификатора ICMP-клиента для выполнения запроса к удалённому серверу
				 *
				 * @return идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 *
				 * \~english
				 * @brief Method of getting the identifier of the ICMP client for performing a request to a remote server
				 * @return identifier of the ICMP client for performing a request to a remote server
				 *
				 * \~
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выполнения пингов удалённого сервера
				 *
				 * @param id    идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
				 * @param count количество выполняемых запросов
				 * @param mode  режим выполнения запросов
				 * @return      результат выполнения запроса
				 *
				 * \~english
				 * @brief Method of performing pings of a remote server
				 * @param id    identifier of the ICMP client for performing a request to a remote server
				 * @param count number of the requests to be performed
				 * @param mode  mode of performing the requests
				 * @return      result of performing the request
				 *
				 * \~
				 */
				bool ping(const id_t id, const uint16_t count, const mode_t mode) noexcept;
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
				ICMP(const ICMP &) = delete;
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
				ICMP & operator = (const ICMP &) = delete;
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
				explicit ICMP(const fmk_t * fmk, const log_t * log) noexcept;
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
				~ICMP() noexcept;
		} icmp_t;
	};
};

#endif // __AWH_UNIT_ICMP__
