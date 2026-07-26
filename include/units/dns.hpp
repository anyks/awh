/**
 * @file: dns.hpp
 * @date: 2026-02-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля DNS-резолвера — класс unit::DNS,
 *        выполняющий асинхронный разбор доменных имён по записям A, AAAA, MX, TXT и другим, с пулом DNS-серверов,
 *        раунд-робин распределением, кешированием и контролем таймаутов
 *
 * @copyright: Copyright © 2026
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
#include "../sys/binbox.hpp"

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
		 * @brief Класс DNS-резолвера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ DNS : public unit_t {
			public:
				/**
				 * @brief Идентификатор DNS-резолвера
				 *
				 */
				using id_t = uint16_t;
			public:
				/**
				 * @brief Типы DNS-записей
				 *
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
				 * @brief Структура буфера полезной нагрузки
				 *
				 * @details Используется для хранения данных DNS-запроса или DNS-ответа.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Payload {
					// Размер буфера
					size_t size;
					// Данные буфера
					unique_ptr <uint8_t []> buffer;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Payload() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Payload() noexcept = default;
				} payload_t;
			private:
				/**
				 * @brief Класс активного пакета при выполнении DNS-запросов
				 *
				 * @details Хранит информацию о текущем DNS-запросе, включая время жизни, количество попыток и полезную нагрузку.
				 *
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
						 * @brief Оператор перемещающего присваивания параметров пакета
						 *
						 * @param packet объект параметров пакета
						 * @return       текущие параметры пакета
						 *
						 */
						Packet & operator = (Packet && packet) noexcept;
						/**
						 * @brief Оператор копирующего присваивания параметров пакета
						 *
						 * @param packet объект параметров пакета
						 * @return        текущие параметры пакета
						 *
						 */
						Packet & operator = (const Packet & packet) noexcept;
					public:
						/**
						 * @brief Конструктор перемещения
						 *
						 * @param packet объект параметров пакета
						 *
						 */
						explicit Packet(Packet && packet) noexcept;
						/**
						 * @brief Конструктор копирования
						 *
						 * @param packet объект параметров пакета
						 *
						 */
						explicit Packet(const Packet & packet) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Packet() noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Packet() noexcept = default;
				} packet_t;
				/**
				 * @brief Класс для управления очередью идентификаторов событий
				 *
				 * @details Используется для хранения идентификаторов событий DNS-резолвера, которые могут быть повторно использованы.
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ SimpleQueue {
					private:
						// Очередь для хранения идентификаторов событий
						std::queue <event::id_t> _ids;
					public:
						/**
						 * @brief Метод очистки очереди идентификаторов событий
						 *
						 */
						void clear() noexcept;
					public:
						/**
						 * @brief Метод получения размера очереди идентификаторов событий
						 *
						 * @return размер очереди идентификаторов событий
						 *
						 */
						size_t size() const noexcept;
					public:
						/**
						 * @brief Метод добавления идентификатора события в очередь
						 *
						 * @param eid идентификатор события для добавления в очередь
						 *
						 */
						void push(event::id_t eid) noexcept;
					public:
						/**
						 * @brief Метод извлечения идентификатора события из очереди
						 *
						 * @param eid идентификатор события для извлечения из очереди
						 * @return    результат извлечения идентификатора
						 *
						 */
						bool pop(event::id_t & eid) noexcept;
					public:
						/**
						 * @brief Метод удаления идентификатора события из очереди
						 *
						 * @param eid идентификатор события для удаления из очереди
						 *
						 */
						void remove(const event::id_t eid) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit SimpleQueue() noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~SimpleQueue() noexcept = default;
				} queue_t;
				/**
				 * @brief Класс для управления списком DNS-серверов
				 *
				 * @details Хранит список DNS-серверов для выполнения запросов,
				 *          поддерживает раунд-робин распределение нагрузки и инициализацию из переменных окружения.
				 *
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
						 * @brief Метод инициализации списка DNS-серверов из переменных окружения или стандартных значений
						 *
						 */
						void init() noexcept;
					public:
						/**
						 * @brief Метод сброса списка DNS-серверов
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 *
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод получения текущего DNS-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект DNS-сервера для выполнения запроса
						 *
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод добавления DNS-сервера в список
						 *
						 * @param server объект DNS-сервера для добавления в список
						 *
						 */
						void push(const net::addr_t * server) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Servers() noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Servers() noexcept = default;
				} servers_t;
			private:
				/**
				 * @brief Структура для управления состоянием DNS-резолвера
				 *
				 * @details Хранит настройки DNS-резолвера, включая префикс для переменных окружения,
				 *          порт, таймаут, очередь идентификаторов событий и список DNS-серверов.
				 *
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
					 * @brief Конструктор
					 *
					 */
					explicit Resolver() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Resolver() noexcept = default;
				} resolver_t;
				/**
				 * @brief Структура для управления очередью и состоянием DNS-запросов
				 *
				 * @details Хранит количество попыток DNS-запроса, максимальное количество пакетов в очереди,
				 *          очередь пакетов, ожидающих отправки, активные DNS-запросы, ожидающие ответа,
				 *          и соответствие между идентификатором события и идентификатором запроса.
				 *
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
					 * @brief Конструктор
					 *
					 */
					explicit Transfer() noexcept;
					/**
					 * @brief Деструктор
					 *
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
				 * @brief Метод сохранения дампа DNS-кэша в файл
				 *
				 * @param tid    идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 *
				 */
				void dumping(const event::id_t, const event::status_t status) noexcept;
				/**
				 * @brief Метод очистки устаревших записей DNS-кэша
				 *
				 * @param tid    идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 *
				 */
				void collector(const event::id_t, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки событий загрузки локальных хостов
				 *
				 * @param      идентификатор события загрузки локальных хостов
				 * @param data данные события загрузки локальных хостов
				 * @param size размер данных события загрузки локальных хостов
				 *
				 */
				void hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки ответов DNS-сервера
				 *
				 * @param eid  идентификатор события чтения из DNS-резолвера
				 * @param data данные события чтения из DNS-резолвера
				 * @param size размер данных события чтения из DNS-резолвера
				 *
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки истечения таймаута DNS-запроса
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param action тип действия для истекшего таймаута
				 * @param delay  длительность таймаута в миллисекундах
				 * @return       нужно ли завершить обработчик после истечения таймаута
				 *
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
				/**
				 * @brief Метод обработки ошибок событий DNS-резолвера
				 *
				 * @param eid         идентификатор события DNS-резолвера
				 * @param error       код ошибки события DNS-резолвера
				 * @param description описание ошибки события DNS-резолвера
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
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
				 * @brief Метод установки числа попыток DNS-запроса
				 *
				 * @param attempts количество попыток DNS-запроса
				 *
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * @brief Метод установки максимального количества пакетов в очереди ожидания выполнения запроса к DNS-серверу
				 *
				 * @param count максимальное количество пакетов
				 *
				 */
				void setMaxPackets(const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод кодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для кодирования
				 * @return       результат работы кодирования
				 *
				 */
				string encode(string_view domain) const noexcept;
				/**
				 * @brief Метод декодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для декодирования
				 * @return       результат работы декодирования
				 *
				 */
				string decode(string_view domain) const noexcept;
			public:
				/**
				 * @brief Метод пересортировки адресов в кэше для доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 *
				 */
				void shuffle(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * @brief Метод очистки чёрного списка
				 *
				 */
				void clearBlacklist() noexcept;
				/**
				 * @brief Метод очистки чёрного списка
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 *
				 */
				void clearBlacklist(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 *
				 */
				void removeAddressInBlacklist(string_view ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 *
				 */
				void removeAddressInBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для удаления из чёрного списка
				 *
				 */
				void removeAddressInBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 *
				 */
				void pushAddressToBlacklist(string_view ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 *
				 */
				void pushAddressToBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для добавления в чёрный список
				 *
				 */
				void pushAddressToBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 *
				 */
				bool checkAddressInBlacklist(string_view ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 *
				 */
				bool checkAddressInBlacklist(const net::addr_t * ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
				 *
				 */
				bool checkAddressInBlacklist(const event::family_t family, string_view ip) const noexcept;
			public:
				/**
				 * @brief Метод очистки кэша
				 *
				 */
				void clearCache() noexcept;
				/**
				 * @brief Метод очистки кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 *
				 */
				void clearCache(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param domain доменное имя, для которого выполняется очистка кэша
				 *
				 */
				void clearCache(string_view domain) noexcept;
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя, для которого выполняется очистка кэша
				 *
				 */
				void clearCache(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @return       IP-адрес находящийся в кэше
				 *
				 */
				string extractAddressFromCache(const event::family_t family, string_view domain) noexcept;
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param value  IP-адрес находящийся в кэше
				 * @return       результат выполнения операции
				 *
				 */
				bool extractAddressFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 */
				void pushAddressToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 */
				void pushAddressToCache(string_view domain, const net::addr_t * ip, const uint32_t ttl) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 *
				 */
				void pushAddressToCache(const event::family_t family, string_view domain, string_view ip, const uint32_t ttl) noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 *
				 */
				void setPrefixEnvironment(string_view prefix) noexcept;
			public:
				/**
				 * @brief Метод установки пути к файлу локальных хостов
				 *
				 * @param filename путь к файлу /etc/hosts или аналогу
				 *
				 */
				void setHostsAddress(string_view filename) noexcept;
			public:
				/**
				 * @brief Метод установки пути к файлу дампа кэша
				 *
				 * @param filename путь к файлу дампа кэша
				 * @param interval интервал сохранения дампа кэша в миллисекундах
				 *
				 */
				void setDumpAddress(string_view filename, const uint32_t interval) noexcept;
			public:
				/**
				 * @brief Метод установки таймаута для ожидания ответа от DNS-сервера
				 *
				 * @param delay время ожидания ответа от DNS-сервера (в миллисекундах)
				 *
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * @brief Метод получения количества DNS-резолверов для выполнения запросов к DNS-серверам
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @return       количество DNS-резолверов
				 *
				 */
				uint16_t resolvers(const event::family_t family) const noexcept;
			public:
				/**
				 * @brief Метод инициализации DNS-резолверов
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param count  количество DNS-резолверов для инициализации
				 * @return       результат выполнения операции
				 *
				 */
				bool init(const event::family_t family, const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод получения UDP-порта DNS-сервера
				 *
				 * @return UDP-порт DNS-сервера
				 *
				 */
				uint16_t getTargetPort() const noexcept;
				/**
				 * @brief Метод установки UDP-порта DNS-сервера
				 *
				 * @param port UDP-порт DNS-сервера
				 *
				 */
				void setTargetPort(const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 *
				 */
				void setServer(string_view server) noexcept;
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 *
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для установки
				 *
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 *
				 */
				void addServer(string_view server) noexcept;
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 *
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для добавления
				 *
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param servers адреса DNS-серверов для установки
				 *
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param servers адреса DNS-серверов для установки
				 *
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса DNS-серверов для установки
				 *
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод генерации идентификатора DNS-запроса
				 *
				 * @return уникальный идентификатор DNS-запроса
				 *
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id    идентификатор DNS-запроса
				 * @param ip    адрес для обратного DNS-запроса
				 * @param alive срок ожидания ответа (в миллисекундах)
				 * @return      результат постановки запроса в очередь
				 *
				 */
				bool search(const id_t id, string_view ip, const uint32_t alive = 0) noexcept;
				/**
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id    идентификатор DNS-запроса
				 * @param ip    адрес для обратного DNS-запроса
				 * @param alive срок ожидания ответа (в миллисекундах)
				 * @return      результат постановки запроса в очередь
				 *
				 */
				bool search(const id_t id, const net::addr_t * ip, const uint32_t alive = 0) noexcept;
				/**
				 * @brief Метод обратного DNS-разрешения (поиск доменного имени по IP-адресу)
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для обратного DNS-запроса
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 */
				bool search(const id_t id, const event::family_t family, string_view ip, const uint32_t alive = 0) noexcept;
			public:
				/**
				 * @brief Метод выполнения произвольного DNS-запроса
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param record тип DNS-записи, которую необходимо получить
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 */
				bool request(const id_t id, const record_t record, string_view domain, const uint32_t alive = 0) noexcept;
			public:
				/**
				 * @brief Метод разрешения доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 */
				bool resolve(const id_t id, string_view domain, const uint32_t alive = 0) noexcept;
				/**
				 * @brief Метод разрешения доменного имени
				 *
				 * @param id     идентификатор DNS-запроса
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя
				 * @param alive  срок ожидания ответа (в миллисекундах)
				 * @return       результат постановки запроса в очередь
				 *
				 */
				bool resolve(const id_t id, const event::family_t family, string_view domain, const uint32_t alive = 0) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				DNS(const DNS &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				DNS & operator = (const DNS &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit DNS(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 */
				explicit DNS(const event::family_t family, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~DNS() noexcept;
		} dns_t;
	};
};

#endif // __AWH_UNIT_DNS_RESOLVER__
