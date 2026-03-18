/**
 * @file: dns.hpp
 * @date: 2026-02-26
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_DNS_RESOLVER__
#define __AWH_UNIT_DNS_RESOLVER__

/**
 * Наши модули
 */
#include "unit.hpp"
#include "../sys/binbox.hpp"
#include "../sys/locker.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён узла источника
	 *
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;
		/**
		 * @brief Класс DNS ресолвера
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
				 * @brief Класс для управления списком DNS-серверов
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
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод получения текущего DNS-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект DNS-сервера для выполнения запроса
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод добавления DNS-сервера в список
						 *
						 * @param server объект DNS-сервера для добавления в список
						 */
						void push(const net::addr_t * server) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Servers() noexcept;
				} servers_t;
				/**
				 * @brief Структура для управления состоянием DNS-резолвера
				 *
				 */
				typedef struct Resolver {
					// Префикс для переменных окружения
					string prefix;
					// Порт сервера DNS-резолвера
					uint16_t port;
					// Идентификатор события для DNS-резолвера
					event::id_t eid;
					// Адрес DNS-сервера для выполнения запросов
					servers_t nameServers;
					// Адрес сети для выполнения запроса
					unique_ptr <net::addr_t> source;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Resolver() noexcept :
					 prefix{""}, port(53),
					 eid(0), source(nullptr) {}
				} resolver_t;
				/**
				 * @brief Структура активного пакета при выполнении DNS-запросов
				 *
				 */
				typedef struct Packet {
					// Доменное имя, для которого произошёл таймаут
					string domain;
					// Время ожидания ответа от DNS-сервера (в миллисекундах)
					uint32_t delay;
					// Количество попыток резолвинга доменного имени
					uint8_t attempt;
					// Идентификатор события для таймера DNS-резолвера
					event::id_t eid;
					// Тип DNS-записи, для которой произошёл таймаут
					record_t record;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Packet() noexcept :
					 domain{AWH_SHORT_NAME},
					 delay(5000), attempt(0), eid(0),
					 record(record_t::NONE) {}
				} packet_t;
				/**
				 * @brief Структура для управления передачей данных при резолвинге доменных имён
				 *
				 */
				typedef struct Transfer {
					// Количество попыток резолвинга доменного имени
					uint8_t attempts;
					// Мьютекс для блокировки потока
					lock_state_t <std::shared_mutex> mtx;
					// Активные пакеты при резолвинге доменных имён
					unordered_map <id_t, packet_t> waiting;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Transfer() noexcept : attempts(3) {}
				} transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
				// Бинарный контейнер для хранения кэша доменных имён
				binbox_t _binbox;
			private:
				// Состояние DNS-резолвера
				resolver_t _resolver;
				// Объект управления передачей данных при резолвинге доменных имён
				transfer_t _transfer;
			private:
				/**
				 * @brief Метод создания события DNS-резолвера
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 */
				void create(const event::family_t family) noexcept;
			private:
				/**
				 * @brief Метод обработки событий дампинга DNS-кэша
				 *
				 * @param        идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 */
				void dumping(const event::id_t, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий коллектора DNS-кэша
				 *
				 * @param        идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 */
				void collector(const event::id_t, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки событий загрузки локальных хостов
				 *
				 * @param      идентификатор события загрузки локальных хостов
				 * @param data данные события загрузки локальных хостов
				 * @param size размер данных события загрузки локальных хостов
				 */
				void hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки ошибок событий DNS-резолвера
				 *
				 * @param eid         идентификатор события DNS-резолвера
				 * @param error       код ошибки события DNS-резолвера
				 * @param description описание ошибки события DNS-резолвера
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * @brief Метод обработки ответов от DNS-сервера на запросы резолвинга доменных имён
				 *
				 * @param eid  идентификатор события чтения из DNS-резолвера
				 * @param data данные события чтения из DNS-резолвера
				 * @param size размер данных события чтения из DNS-резолвера
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий таймаута при ожидании ответа от DNS-сервера
				 *
				 * @param id     идентификатор DNS-резолвера
				 * @param        идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 * @param packet объект активного пакета DNS-запроса
				 */
				void timeout(const id_t id, const event::id_t, const event::status_t status, packet_t * packet) noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод установки количества попыток резолвинга доменного имени
				 *
				 * @param attempts количество попыток резолвинга доменного имени
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * @brief Метод кодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для кодирования
				 * @return       результат работы кодирования
				 */
				string encode(string_view domain) const noexcept;
				/**
				 * @brief Метод декодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для декодирования
				 * @return       результат работы декодирования
				 */
				string decode(string_view domain) const noexcept;
			public:
				/**
				 * @brief Метод пересортировки адресов в кэше для доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
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
				 */
				void clearBlacklist(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 */
				void removeAddressInBlacklist(string_view ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param ip адрес для удаления из чёрного списка
				 */
				void removeAddressInBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void removeAddressInBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 */
				void pushAddressToBlacklist(string_view ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param ip адрес для добавления в чёрный список
				 */
				void pushAddressToBlacklist(const net::addr_t * ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для добавления в чёрный список
				 */
				void pushAddressToBlacklist(const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 */
				bool checkAddressInBlacklist(string_view ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param ip адрес для проверки наличия в чёрном списке
				 * @return   результат проверки наличия IP-адреса в чёрном списке
				 */
				bool checkAddressInBlacklist(const net::addr_t * ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
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
				 */
				void clearCache(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param domain доменное имя для которого выполняется очистка кэша
				 */
				void clearCache(string_view domain) noexcept;
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя для которого выполняется очистка кэша
				 */
				void clearCache(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @return       IP-адрес находящийся в кэше
				 */
				string extractAddressFromCache(const event::family_t family, string_view domain) noexcept;
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param value  IP-адрес находящийся в кэше
				 * @return       результат выполнения операции
				 */
				bool extractAddressFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 */
				void pushAddressToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 */
				void pushAddressToCache(string_view domain, const net::addr_t * ip, const uint32_t ttl) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в кэш
				 * @param ttl    время жизни кэша доменного имени (в секундах)
				 */
				void pushAddressToCache(const event::family_t family, string_view domain, string_view ip, const uint32_t ttl) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 */
				void setPrefixEnvironment(string_view prefix) noexcept;
			public:
				/**
				 * @brief Метод установки адреса файла локальных хостов
				 *
				 * @param filename адрес файла для установки
				 */
				void setHostsAddress(string_view filename) noexcept;
			public:
				/**
				 * @brief Метод установки адреса файлового дампа кэша
				 *
				 * @param filename адрес файла для установки
				 * @param interval интервал сохранения дампа кэша в миллисекундах
				 */
				void setDumpAddress(string_view filename, const uint32_t interval) noexcept;
			public:
				/**
				 * @brief Метод сброса DNS-резолвера
				 *
				 * @return результат выполнения операции
				 */
				bool reset() noexcept;
			public:
				/**
				 * @brief Метод фиксации параметров DNS-резолвера
				 *
				 * @return результат выполнения операции
				 */
				bool commit() noexcept;
			public:
				/**
				 * @brief Метод получения порта сервера DNS-резолвера
				 *
				 * @return порт сервера DNS-резолвера
				 */
				uint16_t getPort() const noexcept;
				/**
				 * @brief Метод установки порта сервера DNS-резолвера
				 *
				 * @param port порт сервера DNS-резолвера
				 */
				void setPort(const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 */
				void setServer(string_view server) noexcept;
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для установки
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод установки адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для установки
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 */
				void addServer(string_view server) noexcept;
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param server адрес DNS-сервера для добавления
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод добавления адреса DNS-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера для добавления
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param server адреса DNS-серверов для установки
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param server адреса DNS-серверов для установки
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов DNS-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса DNS-серверов для установки
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки адреса сети с которого будет выполняться запрос
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод получения идентификатора DNS-резолвера для выполнения запроса к DNS-серверу
				 *
				 * @return идентификатор DNS-резолвера для выполнения запроса к DNS-серверу
				 */
				id_t issue() const noexcept;
			public:
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param ip      адрес для поиска доменного имени
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool search(const id_t id, string_view ip, const uint32_t timeout = 0) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param ip      адрес для поиска доменного имени
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool search(const id_t id, const net::addr_t * ip, const uint32_t timeout = 0) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param family  тип интернет-протокола IPv4/IPv6
				 * @param ip      адрес для поиска доменного имени
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool search(const id_t id, const event::family_t family, string_view ip, const uint32_t timeout = 0) noexcept;
			public:
				/**
				 * @brief Метод выполнения произвольного запроса
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param record  тип DNS-записи которую необходимо получить
				 * @param domain  доменное имя сервера
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool request(const id_t id, const record_t record, string_view domain, const uint32_t timeout = 0) noexcept;
			public:
				/**
				 * @brief Метод резолвинга доменного имени
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param domain  доменное имя сервера
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool resolve(const id_t id, string_view domain, const uint32_t timeout = 0) noexcept;
				/**
				 * @brief Метод резолвинга доменного имени
				 *
				 * @param id      идентификатор DNS-резолвера для которого выполняется поиск доменного имени
				 * @param family  тип интернет-протокола IPv4/IPv6
				 * @param domain  доменное имя сервера
				 * @param timeout время ожидания ответа от DNS-сервера (в миллисекундах)
				 * @return        результат выполнения запроса
				 */
				bool resolve(const id_t id, const event::family_t family, string_view domain, const uint32_t timeout = 0) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
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
