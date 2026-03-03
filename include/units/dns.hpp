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
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
				// Бинарный контейнер для хранения кэша доменных имён
				binbox_t _binbox;
			private:
				/**
				 * @brief Метод обработки событий дампинга DNS-кэша
				 *
				 * @param        идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 */
				void dumping(const event::id_t, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий таймаута при ожидании ответа от DNS-сервера
				 *
				 * @param eid    идентификатор таймера DNS-резолвера
				 * @param status статус события таймера DNS-резолвера
				 */
				void timeout(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий загрузки локальных хостов
				 *
				 * @param      идентификатор события загрузки локальных хостов
				 * @param data данные события загрузки локальных хостов
				 * @param size размер данных события загрузки локальных хостов
				 */
				void hosts(const event::id_t, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий чтения из DNS-резолвера
				 *
				 * @param eid  идентификатор события чтения из DNS-резолвера
				 * @param data данные события чтения из DNS-резолвера
				 * @param size размер данных события чтения из DNS-резолвера
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки ошибок событий DNS-резолвера
				 *
				 * @param eid         идентификатор события DNS-резолвера
				 * @param error       код ошибки события DNS-резолвера
				 * @param description описание ошибки события DNS-резолвера
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
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
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				void removeAddressInBlacklist(const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списка
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				void pushAddressToBlacklist(const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				bool checkAddressInBlacklist(const unique_ptr <net::addr_t> & ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя для которого выполняется очистка кэша
				 */
				void clearCache(const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @return       IP-адрес находящийся в кэше
				 */
				string extractAddressFromCache(const event::family_t family, string_view domain) noexcept;
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				void pushAddressToCache(string_view domain, const unique_ptr <net::addr_t> & ip, const uint32_t ttl) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
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
				 * @brief Метод фиксации параметров события
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @return    результат выполнения фиксации
				 */
				bool commit(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод уничтожения события DNS-резолвера
				 *
				 * @param eid идентификатор события DNS-резолвера
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод создания события DNS-резолвера
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @return       идентификатор события DNS-резолвера
				 */
				event::id_t create(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод получения порта события
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @return    порт события
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта события
				 *
				 * @param eid  идентификатор события DNS-резолвера
				 * @param port порт события
				 * @return     результат выполнения установки
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута события
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param action тип действия события
				 * @return       значение таймаута в миллисекундах
				 */
				uint32_t getTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки времени ожидания выполнения запроса
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод добавления сервера DNS
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param server адрес DNS-сервера
				 */
				void addServer(const event::id_t eid, string_view server) noexcept;
				/**
				 * @brief Метод добавления сервера DNS
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param server адрес DNS-сервера
				 */
				void addServer(const event::id_t eid, const unique_ptr <net::addr_t> & server) noexcept;
				/**
				 * @brief Метод добавления сервера DNS
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param server адрес DNS-сервера
				 */
				void addServer(const event::id_t eid, const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса сети с которого будет выполняться запрос
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param source адрес сети для выполнения запроса
				 */
				void addSource(const event::id_t eid, string_view source) noexcept;
				/**
				 * @brief Метод добавления адреса сети с которого будет выполняться запрос
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param source адрес сети для выполнения запроса
				 */
				void addSource(const event::id_t eid, const unique_ptr <net::addr_t> & source) noexcept;
				/**
				 * @brief Метод добавления адреса сети с которого будет выполняться запрос
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 */
				void addSource(const event::id_t eid, const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    результат выполнения операции
				 */
				bool search(const event::id_t eid, string_view ip) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    результат выполнения операции
				 */
				bool search(const event::id_t eid, const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param ip     адрес для поиска доменного имени
				 * @return       результат выполнения операции
				 */
				bool search(const event::id_t eid, const event::family_t family, string_view ip) noexcept;
			public:
				/**
				 * @brief Метод выполнения произвольного запроса
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param record тип DNS-записи которую необходимо получить
				 * @param domain доменное имя сервера
				 * @return       результат выполнения операции
				 */
				bool request(const event::id_t eid, const record_t record, string_view domain) noexcept;
			public:
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param domain доменное имя сервера
				 * @return       результат выполнения операции
				 */
				bool resolve(const event::id_t eid, string_view domain) noexcept;
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param domain доменное имя сервера
				 * @return       результат выполнения операции
				 */
				bool resolve(const event::id_t eid, const event::family_t family, string_view domain) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit DNS(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~DNS() noexcept;
		} dns_t;
	};
};

#endif // __AWH_UNIT_DNS_RESOLVER__
