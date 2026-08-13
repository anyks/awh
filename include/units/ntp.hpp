/**
 * @file: ntp.hpp
 * @date: 2026-03-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля NTP-клиента — класс unit::NTP,
 *        выполняющий синхронизацию времени с пулом NTP-серверов,
 *        расчёт смещения относительно локальных часов и контроль таймаутов запросов
 *
 * \~english
 * @brief Header file of the NTP client module — the unit::NTP class,
 *        which synchronizes the time with a pool of NTP servers,
 *        calculates the offset relative to the local clock and controls the timeouts of the requests
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_NTP__
#define __AWH_UNIT_NTP__

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
		 * @brief Класс NTP-клиента
		 *
		 * \~english
		 * @brief NTP client class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ NTP : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief Версии протокола NTP
				 *
				 * \~english
				 * @brief Versions of the NTP protocol
				 *
				 * \~
				 */
				enum class version_t : uint8_t {
					V1 = 0x01, // Версия 1
					V2 = 0x02, // Версия 2
					V3 = 0x03, // Версия 3
					V4 = 0x04  // Версия 4 (наиболее распространённая)
				};
			private:
				/**
				 * \~russian
				 * @brief Класс для управления списком NTP-серверов
				 *
				 * @details Содержит методы для инициализации, сброса и получения текущего NTP-сервера из списка.
				 *
				 * \~english
				 * @brief Class for managing the list of the NTP servers
				 * @details Contains the methods for initializing, resetting and getting the current NTP server from the list.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Servers {
					private:
						// Индекс текущего NTP-сервера для выполнения запроса IPv4 (для раунд-робин распределения нагрузки)
						size_t _indexIPv4;
						// Индекс текущего NTP-сервера для выполнения запроса IPv6 (для раунд-робин распределения нагрузки)
						size_t _indexIPv6;
					private:
						// Флаг инициализации списка NTP-серверов IPv4
						bool _initializedIPv4;
						// Флаг инициализации списка NTP-серверов IPv6
						bool _initializedIPv6;
					private:
						// Список NTP-серверов для выполнения запросов (IPv4)
						vector <unique_ptr <net::addr_t>> _ipv4;
						// Список NTP-серверов для выполнения запросов (IPv6)
						vector <unique_ptr <net::addr_t>> _ipv6;
					public:
						/**
						 * \~russian
						 * @brief Метод инициализации списка NTP-серверов из переменных окружения или стандартных значений
						 *
						 * \~english
						 * @brief Method of initializing the list of the NTP servers from the environment variables or the standard values
						 *
						 * \~
						 */
						void init() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод сброса списка NTP-серверов
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 *
						 * \~english
						 * @brief Method of resetting the list of the NTP servers
						 * @param family family of the IP addresses IPv4/IPv6
						 *
						 * \~
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения текущего NTP-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект NTP-сервера для выполнения запроса
						 *
						 * \~english
						 * @brief Method of getting the current NTP server
						 * @param family family of the IP addresses IPv4/IPv6
						 * @return       NTP server object for performing the request
						 *
						 * \~
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод добавления NTP-сервера в список
						 *
						 * @param server объект NTP-сервера для добавления в список
						 *
						 * \~english
						 * @brief Method of adding an NTP server to the list
						 * @param server NTP server object to be added to the list
						 *
						 * \~
						 */
						void push(const net::addr_t * server) noexcept;
					public:
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
						explicit Servers() noexcept;
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
						~Servers() noexcept = default;
				} servers_t;
				/**
				 * \~russian
				 * @brief Структура для управления состоянием NTP-клиента
				 *
				 * @details Содержит параметры для выполнения запросов к NTP-серверам и хранения состояния NTP-клиента.
				 *
				 * \~english
				 * @brief Structure for managing the state of the NTP client
				 * @details Contains the parameters for performing the requests to the NTP servers and for storing the state of the NTP client.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Client {
					// Префикс для переменных окружения (по умолчанию: AWH_SHORT_NAME)
					string prefix;
					// Порт NTP-сервера (по умолчанию: 123)
					uint16_t port;
					// Задержка ожидания ответа от NTP-сервера (в миллисекундах, по умолчанию: 5000)
					uint32_t delay;
					// Идентификатор события для NTP-клиента
					event::id_t eid;
					// Адрес NTP-сервера для выполнения запросов
					servers_t servers;
					// Адрес сети для выполнения запроса
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
					~Client() noexcept = default;
				} client_t;
				/**
				 * \~russian
				 * @brief Структура для управления передачей данных при выполнении запросов NTP-клиента
				 *
				 * @details Содержит параметры для управления передачей данных при выполнении запросов NTP-клиента.
				 *
				 * \~english
				 * @brief Structure for managing the data transfer while performing the requests of the NTP client
				 * @details Contains the parameters for managing the data transfer while performing the requests of the NTP client.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Флаг ожидания ответа от NTP-сервера
					bool waiting;
					// Количество попыток получения ответа от NTP-сервера
					uint8_t attempt;
					// Количество попыток получения ответа от NTP-сервера (по умолчанию: 3)
					uint8_t attempts;
					// Метка transmit из последнего запроса (сетевой порядок байтов)
					uint32_t origSec;
					// Дробная часть метки transmit из последнего запроса (сетевой порядок байтов)
					uint32_t origFrac;
					// Версия протокола NTP для выполнения запроса (по умолчанию: version_t::V4)
					version_t version;
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
					~Transfer() noexcept = default;
				} __attribute__((packed)) transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние NTP-клиента
				client_t _client;
				// Объект управления передачей данных при выполнении запросов NTP-клиента
				transfer_t _transfer;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки ошибок событий NTP-клиента
				 *
				 * @param eid         идентификатор события NTP-клиента
				 * @param error       код ошибки события NTP-клиента
				 * @param description описание ошибки события NTP-клиента
				 *
				 * \~english
				 * @brief Method of processing the errors of the events of the NTP client
				 * @param eid         event identifier of the NTP client
				 * @param error       error code of the event of the NTP client
				 * @param description error description of the event of the NTP client
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод продолжения ожидания своего ответа
				 *
				 * @details Дейтаграммный обмен принимает что угодно и от кого угодно, и
				 *          пришедшее бывает не ответом на заданный вопрос: отклик сервера
				 *          прежнего, чужой пакет, подложный ответ со стороны. Отбросить
				 *          такое обмен обязан, но отбросить пакет и прекратить ожидание -
				 *          разные вещи
				 *
				 *          Движок же прекращает: приход данных ожидание чтения завершает, и
				 *          решает он это раньше, чем пакет разобран. Оттого чужой пакет
				 *          прежде обрывал обмен, а вызывающий получал отказ по вопросу,
				 *          ответ на который был ещё в пути
				 *
				 * @note Ожидание продолжается **остатком**, а не сроком полным: иначе поток
				 *       чужих пакетов отодвигал бы отказ без конца, а подложными пакетами
				 *       обмен можно было бы держать открытым сколь угодно долго
				 *
				 * @param eid идентификатор события чтения
				 *
				 * \~english
				 * @brief Method of continuing to wait for one's own response
				 * @details A datagram exchange accepts anything from anyone, and
				 *          what arrives is sometimes not the answer to the question that has been asked: a response of a
				 *          previous server, a foreign packet, a forged response from the outside. The exchange
				 *          is obliged to discard such a thing, but discarding a packet and terminating the waiting are
				 *          different things
				 *          The engine, however, does terminate it: the arrival of data completes the read waiting, and
				 *          it decides that earlier than the packet is parsed. Because of that a foreign packet
				 *          used to break the exchange, while the caller received a refusal on a question
				 *          the answer to which was still on its way
				 * @note The waiting is continued with the **remainder**, not with the full term: otherwise a stream
				 *       of foreign packets would postpone the refusal endlessly, and with forged packets
				 *       the exchange could be held open for as long as one likes
				 * @param eid event identifier of the reading
				 *
				 * \~
				 */
				void keepWaiting(const event::id_t eid) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки ответов от NTP-сервера на запросы NTP-клиента
				 *
				 * @param eid  идентификатор события чтения
				 * @param data данные, полученные от NTP-сервера
				 * @param size размер полученных данных
				 *
				 * \~english
				 * @brief Method of processing the responses of the NTP server to the requests of the NTP client
				 * @param eid  event identifier of the reading
				 * @param data data received from the NTP server
				 * @param size size of the received data
				 *
				 * \~
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки истечения таймаута NTP-запроса
				 *
				 * @param eid    идентификатор события NTP-клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  длительность таймаута в миллисекундах
				 * @return       нужно ли завершить обработчик после истечения таймаута
				 *
				 * \~english
				 * @brief Method of processing the expiration of the timeout of an NTP request
				 * @param eid    event identifier of the NTP client
				 * @param action action type for the expired timeout
				 * @param delay  duration of the timeout in milliseconds
				 * @return       whether the handler should be terminated after the timeout has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод инициализации события NTP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 * @return       результат инициализации события NTP-клиента
				 *
				 * \~english
				 * @brief Method of initializing the event of the NTP client
				 * @param family family of the protocols (for example: IPv4 or IPv6)
				 * @return       result of initializing the event of the NTP client
				 *
				 * \~
				 */
				bool init(const event::family_t family) noexcept;
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
				 * @brief Метод установки таймаута для ожидания ответа от NTP-сервера
				 *
				 * @param delay время ожидания ответа от NTP-сервера (в миллисекундах)
				 *
				 * \~english
				 * @brief Method of setting the timeout for waiting for a response from the NTP server
				 * @param delay time of waiting for a response from the NTP server (in milliseconds)
				 *
				 * \~
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки количества попыток получения ответа от NTP-сервера
				 *
				 * @par Намеренные решения
				 * Попытка здесь - **дополнительная**, а не общее число обращений: вопрос
				 * задаётся сам собой, а попытка есть разрешение задать его заново, когда
				 * срок ожидания вышел. Оттого при трёх попытках сервер опрашивается
				 * четырежды - первый раз и трижды снова, - а полное время ожидания
				 * вызывающего есть срок, умноженный на число попыток плюс одно
				 *
				 * @note Повтором это не является. Повтор - когда ответ получен, а вопрос
				 *       задаётся ещё раз; попытка же случается лишь тогда, когда ответа
				 *       не пришло вовсе
				 *
				 * @param attempts количество попыток получения ответа от NTP-сервера
				 *
				 * \~english
				 * @brief Method of setting the number of attempts to receive a response from the NTP server
				 * @par Deliberate decisions
				 * An attempt here is an **additional** one, not the total number of calls: the question
				 * is asked by itself, while an attempt is the permission to ask it anew once
				 * the waiting term has run out. Because of that with three attempts the server is polled
				 * four times — once and then three times again — while the full waiting time of the
				 * caller is the term multiplied by the number of attempts plus one
				 * @note This is not a repetition. A repetition is when the answer has been received while the question
				 *       is asked once more; an attempt, on the other hand, happens only when no answer
				 *       has arrived at all
				 * @param attempts number of attempts to receive a response from the NTP server
				 *
				 * \~
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 *
				 * \~english
				 * @brief Method of setting the prefix of the environment variable
				 * @param prefix prefix of the environment variable to be set
				 *
				 * \~
				 */
				void setPrefixEnvironment(string_view prefix) noexcept;
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
				 * @brief Метод получения порта NTP-сервера
				 *
				 * @return порт NTP-сервера
				 *
				 * \~english
				 * @brief Method of getting the port of the NTP server
				 * @return port of the NTP server
				 *
				 * \~
				 */
				uint16_t getTargetPort() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки порта NTP-сервера
				 *
				 * @param port порт NTP-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the port of the NTP server
				 * @param port port of the NTP server to be set
				 *
				 * \~
				 */
				void setTargetPort(const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the NTP server
				 * @param server address of the NTP server to be set
				 *
				 * \~
				 */
				void setServer(string_view server) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the NTP server
				 * @param server address of the NTP server to be set
				 *
				 * \~
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the NTP server
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param server address of the NTP server to be set
				 *
				 * \~
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of an NTP server
				 * @param server address of the NTP server to be added
				 *
				 * \~
				 */
				void addServer(string_view server) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of an NTP server
				 * @param server address of the NTP server to be added
				 *
				 * \~
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of an NTP server
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param server address of the NTP server to be added
				 *
				 * \~
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param servers адреса NTP-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the NTP servers
				 * @param servers addresses of the NTP servers to be set
				 *
				 * \~
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param servers адреса NTP-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the NTP servers
				 * @param servers addresses of the NTP servers to be set
				 *
				 * \~
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса NTP-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the NTP servers
				 * @param family  family of the IP addresses IPv4/IPv6
				 * @param servers addresses of the NTP servers to be set
				 *
				 * \~
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the local address for performing the request
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(string_view source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the local address for performing the request
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the local address for performing the request
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод синхронизации времени с NTP-сервером
				 *
				 * @param version версия протокола NTP для выполнения запроса
				 * @return        результат выполнения запроса
				 *
				 * \~english
				 * @brief Method of synchronizing the time with an NTP server
				 * @param version version of the NTP protocol for performing the request
				 * @return        result of performing the request
				 *
				 * \~
				 */
				bool sync(const version_t version = version_t::V4) noexcept;
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
				NTP(const NTP &) = delete;
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
				NTP & operator = (const NTP &) = delete;
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
				explicit NTP(const fmk_t * fmk, const log_t * log) noexcept;
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
				~NTP() noexcept;
		} ntp_t;
	};
};

#endif // __AWH_UNIT_NTP__
