/**
 * @file dns.hpp
 * @date 2026-02-26
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
 * @brief Заголовочный файл модуля DNS-резолвера — класс unit::DNS,
 *        выполняющий асинхронный разбор доменных имён по записям A, AAAA, MX, TXT и другим, с пулом DNS-серверов,
 *        раунд-робин распределением, кешированием и контролем таймаутов
 *
 * \~english
 * @brief Header file of the DNS resolver module — the unit::DNS class,
 *        which performs the asynchronous resolution of the domain names by the A, AAAA, MX, TXT and other records, with a pool of DNS servers,
 *        round-robin distribution, caching and control of the timeouts
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Защита от повторного подключения заголовка
 */
#ifndef __AWH_UNIT_DNS_RESOLVER__
#define __AWH_UNIT_DNS_RESOLVER__

/**
 * Стандартный заголовочный файл
 */
#include <queue>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "unit.hpp"
#include "../sys/locker.hpp"
#include "../container/binbox.hpp"

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
		 * @brief Класс DNS-резолвера
		 *
		 * \~english
		 * @brief DNS resolver class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ DNS : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief Идентификатор DNS-резолвера
				 *
				 * \~english
				 * @brief DNS resolver identifier
				 *
				 * \~
				 */
				using id_t = uint16_t;
			public:
				/**
				 * \~russian
				 * @brief Типы DNS-записей
				 *
				 * \~english
				 * @brief Types of the DNS records
				 *
				 * \~
				 */
				enum class record_t : uint8_t {
					NONE  = 0x00, // Запись не установлена
					A     = 0x01, // IPv4-адрес
					NS    = 0x02, // Доменное имя авторитетного сервера имён
					CNAME = 0x05, // Каноническое имя (псевдоним для другого доменного имени)
					SOA   = 0x06, // Информация об авторитете зоны (Start of Authority)
					PTR   = 0x0C, // Доменное имя, на которое указывает PTR-запись (обычно используется для обратного DNS)
					MX    = 0x0F, // Почтовый обмен (Mail Exchange) - указывает на почтовый сервер для домена
					TXT   = 0x10, // Текстовая запись - содержит произвольные текстовые данные, связанные с доменом
					AAAA  = 0x1C, // IPv6-адрес
					ANY   = 0xFF  // Любой тип записи (используется для запроса всех типов записей для домена)
				};
			private:
				/**
				 * \~russian
				 * @brief Структура буфера полезной нагрузки
				 *
				 * @details Используется для хранения данных DNS-запроса или DNS-ответа.
				 *
				 * \~english
				 * @brief Structure of the payload buffer
				 * @details Used for storing the data of a DNS request or a DNS response.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Payload {
					// Размер буфера
					size_t size;
					// Данные буфера
					unique_ptr <uint8_t []> buffer;
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
					explicit Payload() noexcept;
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
					~Payload() noexcept = default;
				} payload_t;
			private:
				/**
				 * \~russian
				 * @brief Класс активного пакета при выполнении DNS-запросов
				 *
				 * @details Хранит информацию о текущем DNS-запросе, включая время жизни, количество попыток и полезную нагрузку.
				 *
				 * \~english
				 * @brief Class of an active packet while performing the DNS requests
				 * @details Stores the information about the current DNS request, including the lifetime, the number of the attempts and the payload.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Packet {
					public:
						// Срок истечения DNS-запроса (метка времени в миллисекундах)
						uint64_t alive;
						// Количество попыток DNS-запроса
						uint8_t attempt;
						// Полезная нагрузка для отправки DNS-запроса
						payload_t payload;
					public:
						/**
						 * \~russian
						 * @brief Оператор перемещающего присваивания параметров пакета
						 *
						 * @param packet объект параметров пакета
						 * @return       текущие параметры пакета
						 *
						 * \~english
						 * @brief Move assignment operator of the parameters of the packet
						 * @param packet object of the parameters of the packet
						 * @return       current parameters of the packet
						 *
						 * \~
						 */
						Packet & operator = (Packet && packet) noexcept;
						/**
						 * \~russian
						 * @brief Оператор копирующего присваивания параметров пакета
						 *
						 * @param packet объект параметров пакета
						 * @return        текущие параметры пакета
						 *
						 * \~english
						 * @brief Copy assignment operator of the parameters of the packet
						 * @param packet object of the parameters of the packet
						 * @return        current parameters of the packet
						 *
						 * \~
						 */
						Packet & operator = (const Packet & packet) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор перемещения
						 *
						 * @param packet объект параметров пакета
						 *
						 * \~english
						 * @brief Move constructor
						 * @param packet object of the parameters of the packet
						 *
						 * \~
						 */
						explicit Packet(Packet && packet) noexcept;
						/**
						 * \~russian
						 * @brief Конструктор копирования
						 *
						 * @param packet объект параметров пакета
						 *
						 * \~english
						 * @brief Copy constructor
						 * @param packet object of the parameters of the packet
						 *
						 * \~
						 */
						explicit Packet(const Packet & packet) noexcept;
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
						explicit Packet() noexcept;
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
						~Packet() noexcept = default;
				} packet_t;
				/**
				 * \~russian
				 * @brief Класс для управления очередью идентификаторов событий
				 *
				 * @details Используется для хранения идентификаторов событий DNS-резолвера, которые могут быть повторно использованы.
				 *
				 * \~english
				 * @brief Class for managing the queue of the event identifiers
				 * @details Used for storing the identifiers of the events of the DNS resolver which can be reused.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ SimpleQueue {
					private:
						// Очередь для хранения идентификаторов событий
						std::queue <event::id_t> _ids;
					public:
						/**
						 * \~russian
						 * @brief Метод очистки очереди идентификаторов событий
						 *
						 * \~english
						 * @brief Method of clearing the queue of the event identifiers
						 *
						 * \~
						 */
						void clear() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения размера очереди идентификаторов событий
						 *
						 * @return размер очереди идентификаторов событий
						 *
						 * \~english
						 * @brief Method of getting the size of the queue of the event identifiers
						 * @return size of the queue of the event identifiers
						 *
						 * \~
						 */
						size_t size() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод добавления идентификатора события в очередь
						 *
						 * @param eid идентификатор события для добавления в очередь
						 *
						 * \~english
						 * @brief Method of adding an event identifier to the queue
						 * @param eid event identifier to be added to the queue
						 *
						 * \~
						 */
						void push(event::id_t eid) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод извлечения идентификатора события из очереди
						 *
						 * @param eid идентификатор события для извлечения из очереди
						 * @return    результат извлечения идентификатора
						 *
						 * \~english
						 * @brief Method of extracting an event identifier from the queue
						 * @param eid event identifier to be extracted from the queue
						 * @return    result of extracting the identifier
						 *
						 * \~
						 */
						bool pop(event::id_t & eid) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод удаления идентификатора события из очереди
						 *
						 * @param eid идентификатор события для удаления из очереди
						 *
						 * \~english
						 * @brief Method of removing an event identifier from the queue
						 * @param eid event identifier to be removed from the queue
						 *
						 * \~
						 */
						void remove(const event::id_t eid) noexcept;
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
						explicit SimpleQueue() noexcept;
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
						~SimpleQueue() noexcept = default;
				} queue_t;
				/**
				 * \~russian
				 * @brief Класс для управления списком DNS-серверов
				 *
				 * @details Хранит список DNS-серверов для выполнения запросов,
				 *          поддерживает раунд-робин распределение нагрузки и инициализацию из переменных окружения.
				 *
				 * \~english
				 * @brief Class for managing the list of the DNS servers
				 * @details Stores the list of the DNS servers for performing the requests,
				 *          supports the round-robin distribution of the load and the initialization from the environment variables.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Servers {
					private:
						// Индекс текущего DNS-сервера для выполнения запроса IPv4 (для раунд-робин распределения нагрузки)
						size_t _indexIPv4;
						// Индекс текущего DNS-сервера для выполнения запроса IPv6 (для раунд-робин распределения нагрузки)
						size_t _indexIPv6;
					private:
						// Флаг инициализации списка DNS-серверов IPv4
						bool _initializedIPv4;
						// Флаг инициализации списка DNS-серверов IPv6
						bool _initializedIPv6;
					private:
						// Список DNS-серверов для выполнения запросов (IPv4)
						vector <unique_ptr <net::addr_t>> _ipv4;
						// Список DNS-серверов для выполнения запросов (IPv6)
						vector <unique_ptr <net::addr_t>> _ipv6;
					public:
						/**
						 * \~russian
						 * @brief Метод инициализации списка DNS-серверов из переменных окружения или стандартных значений
						 *
						 * \~english
						 * @brief Method of initializing the list of the DNS servers from the environment variables or the standard values
						 *
						 * \~
						 */
						void init() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод сброса списка DNS-серверов
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 *
						 * \~english
						 * @brief Method of resetting the list of the DNS servers
						 * @param family family of the IP addresses IPv4/IPv6
						 *
						 * \~
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод получения текущего DNS-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект DNS-сервера для выполнения запроса
						 *
						 * \~english
						 * @brief Method of getting the current DNS server
						 * @param family family of the IP addresses IPv4/IPv6
						 * @return       DNS server object for performing the request
						 *
						 * \~
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод добавления DNS-сервера в список
						 *
						 * @param server объект DNS-сервера для добавления в список
						 *
						 * \~english
						 * @brief Method of adding a DNS server to the list
						 * @param server DNS server object to be added to the list
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
			private:
				/**
				 * \~russian
				 * @brief Структура для управления состоянием DNS-резолвера
				 *
				 * @details Хранит настройки DNS-резолвера, включая префикс для переменных окружения,
				 *          порт, таймаут, очередь идентификаторов событий и список DNS-серверов.
				 *
				 * \~english
				 * @brief Structure for managing the state of the DNS resolver
				 * @details Stores the settings of the DNS resolver, including the prefix for the environment variables,
				 *          the port, the timeout, the queue of the event identifiers and the list of the DNS servers.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Resolver {
					// Префикс для переменных окружения (по умолчанию "AWH")
					string prefix;
					// UDP-порт DNS-сервера (по умолчанию 53)
					uint16_t port;
					// Таймаут ожидания ответа от DNS-сервера (в миллисекундах, по умолчанию 5000)
					uint32_t delay;
					// Очередь свободных идентификаторов событий DNS-резолвера (IPv4 и IPv6)
					queue_t queue;
					// Список DNS-серверов для выполнения запросов
					servers_t nameServers;
					// Идентификаторы событий DNS-резолвера для IPv4
					vector <event::id_t> idv4;
					// Идентификаторы событий DNS-резолвера для IPv6
					vector <event::id_t> idv6;
					// Локальный адрес источника для запросов IPv4
					unique_ptr <net::addr_t> sourceIPv4;
					// Локальный адрес источника для запросов IPv6
					unique_ptr <net::addr_t> sourceIPv6;
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
					explicit Resolver() noexcept;
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
					~Resolver() noexcept = default;
				} resolver_t;
				/**
				 * \~russian
				 * @brief Структура для управления очередью и состоянием DNS-запросов
				 *
				 * @details Хранит количество попыток DNS-запроса, максимальное количество пакетов в очереди,
				 *          очередь пакетов, ожидающих отправки, активные DNS-запросы, ожидающие ответа,
				 *          и соответствие между идентификатором события и идентификатором запроса.
				 *
				 * \~english
				 * @brief Structure for managing the queue and the state of the DNS requests
				 * @details Stores the number of the attempts of a DNS request, the maximum number of the packets in the queue,
				 *          the queue of the packets waiting to be sent, the active DNS requests waiting for a response,
				 *          and the correspondence between the event identifier and the request identifier.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Количество попыток DNS-запроса (по умолчанию 3)
					uint8_t attempts;
					// Максимальное количество пакетов в очереди ожидания выполнения запроса к DNS-серверу (по умолчанию 200)
					uint16_t maxPackets;
					// Очередь пакетов, ожидающих отправки
					std::queue <packet_t> packets;
					// Активные DNS-запросы, ожидающие ответа
					unordered_map <id_t, packet_t> waiting;
					// Соответствие между идентификатором события и идентификатором запроса
					unordered_map <event::id_t, id_t> attached;
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
				} transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
				// Бинарный контейнер для хранения кэша доменных имён
				binbox_t _binbox;
			private:
				// Состояние DNS-резолвера
				resolver_t _resolver;
				// Объект управления очередью и состоянием DNS-запросов
				transfer_t _transfer;
			private:
				// Блокировка доступа к состоянию передачи DNS-запросов
				mutable lock_state_t <std::mutex>  _mtx;
			private:
				/**
				 * \~russian
				 * @brief Метод сохранения дампа DNS-кэша в файл
				 *
				 * @param tid    идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 *
				 * \~english
				 * @brief Method of saving the dump of the DNS cache into a file
				 * @param tid    timer identifier of the DNS resolver
				 * @param status status of the timer event of the DNS resolver
				 *
				 * \~
				 */
				void dumping(const event::id_t, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки устаревших записей DNS-кэша
				 *
				 * @param tid    идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 *
				 * \~english
				 * @brief Method of clearing the stale records of the DNS cache
				 * @param tid    timer identifier of the DNS resolver
				 * @param status status of the timer event of the DNS resolver
				 *
				 * \~
				 */
				void collector(const event::id_t, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий загрузки локальных хостов
				 *
				 * @param      идентификатор события загрузки локальных хостов
				 * @param data данные события загрузки локальных хостов
				 * @param size размер данных события загрузки локальных хостов
				 *
				 * \~english
				 * @brief Method of processing the loading events of the local hosts
				 * @param      identifier of the loading event of the local hosts
				 * @param data data of the loading event of the local hosts
				 * @param size data size of the loading event of the local hosts
				 *
				 * \~
				 */
				void hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки ответов DNS-сервера
				 *
				 * @param eid  идентификатор события чтения из DNS-резолвера
				 * @param data данные события чтения из DNS-резолвера
				 * @param size размер данных события чтения из DNS-резолвера
				 *
				 * \~english
				 * @brief Method of processing the responses of the DNS server
				 * @param eid  event identifier of the reading from the DNS resolver
				 * @param data data of the reading event from the DNS resolver
				 * @param size data size of the reading event from the DNS resolver
				 *
				 * \~
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки истечения таймаута DNS-запроса
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param action тип действия для истекшего таймаута
				 * @param delay  длительность таймаута в миллисекундах
				 * @return       нужно ли завершить обработчик после истечения таймаута
				 *
				 * \~english
				 * @brief Method of processing the expiration of the timeout of a DNS request
				 * @param eid    event identifier of the DNS resolver
				 * @param action action type for the expired timeout
				 * @param delay  duration of the timeout in milliseconds
				 * @return       whether the handler should be terminated after the timeout has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки ошибок событий DNS-резолвера
				 *
				 * @param eid         идентификатор события DNS-резолвера
				 * @param error       код ошибки события DNS-резолвера
				 * @param description описание ошибки события DNS-резолвера
				 *
				 * \~english
				 * @brief Method of processing the errors of the events of the DNS resolver
				 * @param eid         event identifier of the DNS resolver
				 * @param error       error code of the event of the DNS resolver
				 * @param description error description of the event of the DNS resolver
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
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
				 * @brief Метод установки числа попыток DNS-запроса
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
				 * @param attempts количество попыток DNS-запроса
				 *
				 * \~english
				 * @brief Method of setting the number of the attempts of a DNS request
				 * @par Deliberate decisions
				 * An attempt here is an **additional** one, not the total number of calls: the question
				 * is asked by itself, while an attempt is the permission to ask it anew once
				 * the waiting term has run out. Because of that with three attempts the server is polled
				 * four times — once and then three times again — while the full waiting time of the
				 * caller is the term multiplied by the number of attempts plus one
				 * @note This is not a repetition. A repetition is when the answer has been received while the question
				 *       is asked once more; an attempt, on the other hand, happens only when no answer
				 *       has arrived at all
				 * @param attempts number of the attempts of the DNS request
				 *
				 * \~
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки максимального количества пакетов в очереди ожидания выполнения запроса к DNS-серверу
				 *
				 * @param count максимальное количество пакетов
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the packets in the queue waiting for a request to a DNS server to be performed
				 * @param count maximum number of the packets
				 *
				 * \~
				 */
				void setMaxPackets(const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод кодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для кодирования
				 * @return       результат работы кодирования
				 *
				 * \~english
				 * @brief Method of encoding an international domain name
				 * @param domain domain name to be encoded
				 * @return       result of the work of the encoding
				 *
				 * \~
				 */
				string encode(string_view domain) const noexcept;
				/**
				 * \~russian
				 * @brief Метод декодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для декодирования
				 * @return       результат работы декодирования
				 *
				 * \~english
				 * @brief Method of decoding an international domain name
				 * @param domain domain name to be decoded
				 * @return       result of the work of the decoding
				 *
				 * \~
				 */
				string decode(string_view domain) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод пересортировки адресов в кэше для доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 *
				 * \~english
				 * @brief Method of resorting the addresses in the cache for a domain name
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name corresponding to the IP address
				 *
				 * \~
				 */
				void shuffle(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки чёрного списка
				 *
				 * \~english
				 * @brief Method of clearing the black list
				 *
				 * \~
				 */
				void clearBlacklist() noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки чёрного списка
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 *
				 * \~english
				 * @brief Method of clearing the black list
				 * @param family family of the IP addresses IPv4/IPv6
				 *
				 * \~
				 */
				void clearBlacklist(const event::family_t family) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 *
				 * \~english
				 * @brief Method of removing an IP address from the black list
				 * @param ip address to be removed from the black list
				 *
				 * \~
				 */
				void removeAddressInBlacklist(string_view ip) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 *
				 * \~english
				 * @brief Method of removing an IP address from the black list
				 * @param ip address to be removed from the black list
				 *
				 * \~
				 */
				void removeAddressInBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для удаления из чёрного списка
				 *
				 * \~english
				 * @brief Method of removing an IP address from the black list
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param ip     address to be removed from the black list
				 *
				 * \~
				 */
				void removeAddressInBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 *
				 * \~english
				 * @brief Method of adding an IP address to the black list
				 * @param ip address to be added to the black list
				 *
				 * \~
				 */
				void pushAddressToBlacklist(string_view ip) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 *
				 * \~english
				 * @brief Method of adding an IP address to the black list
				 * @param ip address to be added to the black list
				 *
				 * \~
				 */
				void pushAddressToBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для добавления в чёрный список
				 *
				 * \~english
				 * @brief Method of adding an IP address to the black list
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param ip     address to be added to the black list
				 *
				 * \~
				 */
				void pushAddressToBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 *
				 * \~english
				 * @brief Method of checking the presence of an IP address in the black list
				 * @param ip address to be checked for the presence in the black list
				 * @return   result of checking the presence of the IP address in the black list
				 *
				 * \~
				 */
				bool checkAddressInBlacklist(string_view ip) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 *
				 * \~english
				 * @brief Method of checking the presence of an IP address in the black list
				 * @param ip address to be checked for the presence in the black list
				 * @return   result of checking the presence of the IP address in the black list
				 *
				 * \~
				 */
				bool checkAddressInBlacklist(const net::addr_t * ip) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
				 *
				 * \~english
				 * @brief Method of checking the presence of an IP address in the black list
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param ip     address to be checked for the presence in the black list
				 * @return       result of checking the presence of the IP address in the black list
				 *
				 * \~
				 */
				bool checkAddressInBlacklist(const event::family_t family, string_view ip) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки кэша
				 *
				 * \~english
				 * @brief Method of clearing the cache
				 *
				 * \~
				 */
				void clearCache() noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 *
				 * \~english
				 * @brief Method of clearing the cache
				 * @param family family of the IP addresses IPv4/IPv6
				 *
				 * \~
				 */
				void clearCache(const event::family_t family) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param domain доменное имя, для которого выполняется очистка кэша
				 *
				 * \~english
				 * @brief Method of clearing the cache for the specified domain name
				 * @param domain domain name for which the clearing of the cache is performed
				 *
				 * \~
				 */
				void clearCache(string_view domain) noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя, для которого выполняется очистка кэша
				 *
				 * \~english
				 * @brief Method of clearing the cache for the specified domain name
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name for which the clearing of the cache is performed
				 *
				 * \~
				 */
				void clearCache(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @return       IP-адрес находящийся в кэше
				 *
				 * \~english
				 * @brief Method of getting an IP address from the cache
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name corresponding to the IP address
				 * @return       IP address located in the cache
				 *
				 * \~
				 */
				string extractAddressFromCache(const event::family_t family, string_view domain) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param value  IP-адрес находящийся в кэше
				 * @return       результат выполнения операции
				 *
				 * \~english
				 * @brief Method of getting an IP address from the cache
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name corresponding to the IP address
				 * @param value  IP address located in the cache
				 * @return       result of performing the operation
				 *
				 * \~
				 */
				bool extractAddressFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 * \~english
				 * @brief Method of adding an IP address to the cache
				 * @param domain domain name corresponding to the IP address
				 * @param ip     address to be added to the cache
				 * @param ttl    lifetime of the cache of the domain name (in seconds)
				 *
				 * \~
				 */
				void pushAddressToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 * \~english
				 * @brief Method of adding an IP address to the cache
				 * @param domain domain name corresponding to the IP address
				 * @param ip     address to be added to the cache
				 * @param ttl    lifetime of the cache of the domain name (in seconds)
				 *
				 * \~
				 */
				void pushAddressToCache(string_view domain, const net::addr_t * ip, const uint32_t ttl) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 * \~english
				 * @brief Method of adding an IP address to the cache
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name corresponding to the IP address
				 * @param ip     address to be added to the cache
				 * @param ttl    lifetime of the cache of the domain name (in seconds)
				 *
				 * \~
				 */
				void pushAddressToCache(const event::family_t family, string_view domain, string_view ip, const uint32_t ttl) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * \~english
				 * @brief Method of setting the safety of the work of the threads
				 * @param mode flag of the thread safety mode
				 *
				 * \~
				 */
				void threadSafety(const bool mode) noexcept;
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
				 * @brief Метод установки пути к файлу локальных хостов
				 *
				 * @param filename путь к файлу /etc/hosts или аналогу
				 *
				 * \~english
				 * @brief Method of setting the path to the file of the local hosts
				 * @param filename path to the /etc/hosts file or its counterpart
				 *
				 * \~
				 */
				void setHostsAddress(string_view filename) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пути к файлу дампа кэша
				 *
				 * @param filename путь к файлу дампа кэша
				 * @param interval интервал сохранения дампа кэша в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the path to the file of the dump of the cache
				 * @param filename path to the file of the dump of the cache
				 * @param interval interval of saving the dump of the cache in milliseconds
				 *
				 * \~
				 */
				void setDumpAddress(string_view filename, const uint32_t interval) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки таймаута для ожидания ответа от DNS-сервера
				 *
				 * @param delay время ожидания ответа от DNS-сервера (в миллисекундах)
				 *
				 * \~english
				 * @brief Method of setting the timeout for waiting for a response from the DNS server
				 * @param delay time of waiting for a response from the DNS server (in milliseconds)
				 *
				 * \~
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества DNS-резолверов для выполнения запросов к DNS-серверам
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @return       количество DNS-резолверов
				 *
				 * \~english
				 * @brief Method of getting the number of the DNS resolvers for performing the requests to the DNS servers
				 * @param family family of the IP addresses IPv4/IPv6
				 * @return       number of the DNS resolvers
				 *
				 * \~
				 */
				uint16_t resolvers(const event::family_t family) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод инициализации DNS-резолверов
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param count  количество DNS-резолверов для инициализации
				 * @return       результат выполнения операции
				 *
				 * \~english
				 * @brief Method of initializing the DNS resolvers
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param count  number of the DNS resolvers to be initialized
				 * @return       result of performing the operation
				 *
				 * \~
				 */
				bool init(const event::family_t family, const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения UDP-порта DNS-сервера
				 *
				 * @return UDP-порт DNS-сервера
				 *
				 * \~english
				 * @brief Method of getting the UDP port of the DNS server
				 * @return UDP port of the DNS server
				 *
				 * \~
				 */
				uint16_t getTargetPort() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки UDP-порта DNS-сервера
				 *
				 * @param port UDP-порт DNS-сервера
				 *
				 * \~english
				 * @brief Method of setting the UDP port of the DNS server
				 * @param port UDP port of the DNS server
				 *
				 * \~
				 */
				void setTargetPort(const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the DNS server
				 * @param server address of the DNS server to be set
				 *
				 * \~
				 */
				void setServer(string_view server) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the DNS server
				 * @param server address of the DNS server to be set
				 *
				 * \~
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the DNS server
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param server address of the DNS server to be set
				 *
				 * \~
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of a DNS server
				 * @param server address of the DNS server to be added
				 *
				 * \~
				 */
				void addServer(string_view server) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of a DNS server
				 * @param server address of the DNS server to be added
				 *
				 * \~
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для добавления
				 *
				 * \~english
				 * @brief Method of adding an address of a DNS server
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param server address of the DNS server to be added
				 *
				 * \~
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param servers адреса DNS-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the DNS servers
				 * @param servers addresses of the DNS servers to be set
				 *
				 * \~
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param servers адреса DNS-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the DNS servers
				 * @param servers addresses of the DNS servers to be set
				 *
				 * \~
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса DNS-серверов для установки
				 *
				 * \~english
				 * @brief Method of setting the list of the addresses of the DNS servers
				 * @param family  family of the IP addresses IPv4/IPv6
				 * @param servers addresses of the DNS servers to be set
				 *
				 * \~
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(string_view source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 *
				 * \~english
				 * @brief Method of setting the network address from which the request will be performed
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param source network address for performing the request
				 *
				 * \~
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод генерации идентификатора DNS-запроса
				 *
				 * @par Намеренные решения
				 *
				 * **Номер выдаётся случайным, а не последовательным.** Прежде здесь стоял
				 * счётчик с приращением, и заменён он был на случайную выдачу молча, без
				 * пояснения - оттого решение и записано здесь, чтобы больше не менялось
				 * втихую ни в ту, ни в другую сторону.
				 *
				 * Довод не в договоре как таковом, хотя RFC 5452 случайности и требует.
				 * Довод в устройстве самого резолвера: события обмена заводятся однажды и
				 * живут весь срок работы, переиспользуясь через очередь свободных. Порт
				 * каждому ядро выдаёт единожды, и все вопросы за всё время уходят с одних
				 * и тех же портов.
				 *
				 * Отсюда следствие: порт перестаёт быть тайной после первого же обмена, и
				 * единственное, что отделяет подложный ответ от принятого, - это номер
				 * вопроса. При счётчике узнавший порт знает и следующий номер, а принятый
				 * подлог оседает в кэше на весь срок жизни записи, отравляя его для всех
				 * служб приложения разом - кэш общий на процесс намеренно.
				 *
				 * Будь событие одноразовым, порт менялся бы на каждый вопрос и давал свои
				 * шестнадцать разрядов - тогда счётчик был бы терпим. При долгоживущих
				 * событиях эти разряды достаются противнику даром.
				 *
				 * @warning Плата за случайность - совпадение номеров у одновременно
				 *          ожидающих вопросов. Диапазон шестнадцатиразрядный, и по
				 *          парадоксу дней рождения совпадение наступает не при
				 *          шестидесяти пяти тысячах вопросов, а при **трёхстах**: сто
				 *          одновременных дают семь процентов. Оттого номер и сличается
				 *          перед постановкой на учёт - без сличения вопрос терялся молча,
				 *          а вместо него повторно уходил чужой
				 *
				 * @return уникальный идентификатор DNS-запроса
				 *
				 * \~english
				 * @brief Method of generating the identifier of a DNS request
				 * @par Deliberate decisions
				 * **The number is issued randomly rather than sequentially.** Previously there was
				 * an incrementing counter here, and it was replaced by a random issuance silently, without
				 * an explanation — that is why the decision is recorded here, so that it is no longer changed
				 * quietly in either direction.
				 * The argument is not in the protocol as such, although RFC 5452 does require randomness.
				 * The argument is in the design of the resolver itself: the exchange events are created once and
				 * live for the whole term of the work, being reused through a queue of the free ones. The port
				 * is issued to each of them by the kernel once, and all the questions over the whole time go out from the
				 * same ports.
				 * Hence the consequence: the port ceases to be a secret after the very first exchange, and
				 * the only thing that separates a forged response from an accepted one is the number of the
				 * question. With a counter, whoever has learnt the port knows the next number as well, while an accepted
				 * forgery settles in the cache for the whole lifetime of the record, poisoning it for all the
				 * services of the application at once — the cache is common to the process deliberately.
				 * Were the event a disposable one, the port would change with every question and would give its own
				 * sixteen bits — then a counter would be tolerable. With long-living
				 * events those bits go to the adversary for free.
				 * @warning The price of the randomness is a coincidence of the numbers of the simultaneously
				 *          waiting questions. The range is a sixteen-bit one, and by the
				 *          birthday paradox a coincidence comes not at
				 *          sixty-five thousand questions but at **three hundred**: a hundred
				 *          simultaneous ones give seven per cent. That is why the number is checked
				 *          before being registered — without the check a question was lost silently,
				 *          while a foreign one was sent out again in its place
				 * @return unique identifier of the DNS request
				 *
				 * \~
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id    идентификатор DNS-запроса
				 * @param ip    адрес для обратного DNS-запроса
				 * @param alive срок ожидания ответа (в миллисекундах)
				 * @return      результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of the reverse DNS resolution (searching for a domain name by an IP address)
				 * @param id    identifier of the DNS request
				 * @param ip    address for the reverse DNS request
				 * @param alive term of waiting for a response (in milliseconds)
				 * @return      result of placing the request into the queue
				 *
				 * \~
				 */
				bool search(const id_t id, string_view ip, const uint32_t alive = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id    идентификатор DNS-запроса
				 * @param ip    адрес для обратного DNS-запроса
				 * @param alive срок ожидания ответа (в миллисекундах)
				 * @return      результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of the reverse DNS resolution (searching for a domain name by an IP address)
				 * @param id    identifier of the DNS request
				 * @param ip    address for the reverse DNS request
				 * @param alive term of waiting for a response (in milliseconds)
				 * @return      result of placing the request into the queue
				 *
				 * \~
				 */
				bool search(const id_t id, const net::addr_t * ip, const uint32_t alive = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для обратного DNS-запроса
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of the reverse DNS resolution (searching for a domain name by an IP address)
				 * @param id     identifier of the DNS request
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param ip     address for the reverse DNS request
				 * @param alive  term of waiting for a response (in milliseconds)
				 * @return       result of placing the request into the queue
				 *
				 * \~
				 */
				bool search(const id_t id, const event::family_t family, string_view ip, const uint32_t alive = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выполнения произвольного DNS-запроса
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param record тип DNS-записи, которую необходимо получить
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of performing an arbitrary DNS request
				 * @param id     identifier of the DNS request
				 * @param record type of the DNS record which is to be obtained
				 * @param domain domain name
				 * @param alive  term of waiting for a response (in milliseconds)
				 * @return       result of placing the request into the queue
				 *
				 * \~
				 */
				bool request(const id_t id, const record_t record, string_view domain, const uint32_t alive = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разрешения доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of resolving a domain name
				 * @param id     identifier of the DNS request
				 * @param domain domain name
				 * @param alive  term of waiting for a response (in milliseconds)
				 * @return       result of placing the request into the queue
				 *
				 * \~
				 */
				bool resolve(const id_t id, string_view domain, const uint32_t alive = 0) noexcept;
				/**
				 * \~russian
				 * @brief Метод разрешения доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 * \~english
				 * @brief Method of resolving a domain name
				 * @param id     identifier of the DNS request
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param domain domain name
				 * @param alive  term of waiting for a response (in milliseconds)
				 * @return       result of placing the request into the queue
				 *
				 * \~
				 */
				bool resolve(const id_t id, const event::family_t family, string_view domain, const uint32_t alive = 0) noexcept;
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
				DNS(const DNS &) = delete;
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
				DNS & operator = (const DNS &) = delete;
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
				explicit DNS(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param family family of the IP addresses IPv4/IPv6
				 * @param fmk    framework object
				 * @param log    object for working with logs
				 *
				 * \~
				 */
				explicit DNS(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept;
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
				~DNS() noexcept;
		} dns_t;
	};
};

#endif // __AWH_UNIT_DNS_RESOLVER__
