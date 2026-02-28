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
				 * @brief Метод очистки данных DNS-резолвера
				 *
				 * @return результат работы функции
				 */
				bool clear() noexcept;
				/**
				 * @brief Метод сброса кэша DNS-резолвера
				 *
				 * @return результат работы функции
				 */
				bool flush() noexcept;
			public:
				/**
				 * @brief Метод создания события DNS-резолвера
				 *
				 * @return идентификатор события DNS-резолвера
				 */
				event::id_t create() noexcept;
			public:
				/**
				 * @brief Метод уничтожения события DNS-резолвера
				 *
				 * @param eid идентификатор события DNS-резолвера
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод кодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для кодирования
				 * @return       результат работы кодирования
				 */
				string encode(const string & domain) const noexcept;
				/**
				 * @brief Метод декодирования интернационального доменного имени
				 *
				 * @param domain доменное имя для декодирования
				 * @return       результат работы декодирования
				 */
				string decode(const string & domain) const noexcept;
			public:
				/**
				 * @brief Метод установки времени ожидания выполнения запроса
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод пересортировки серверов DNS
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family семейстов IP-адресов IPv4/IPv6
				 */
				void shuffle(const event::id_t eid, const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @return       IP-адрес находящийся в кэше
				 */
				string getFromCache(const event::family_t family, const string & domain) noexcept;
				/**
				 * @brief Метод получения IP-адреса из кэша
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param value  IP-адрес находящийся в кэше
				 * @return       результат выполнения операции
				 */
				bool getFromCache(const event::family_t family, const string & domain, unique_ptr <net::addr_t> & value) noexcept;
			public:
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param domain доменное имя для которого выполняется очистка кэша
				 */
				void clearCache(const string & domain) noexcept;
				/**
				 * @brief Метод очистки кэша для указанного доменного имени
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя для которого выполняется очистка кэша
				 */
				void clearCache(const event::family_t family, const string & domain) noexcept;
			public:
				/**
				 * @brief Метод очистки кэша
				 *
				 * @param localhost флаг обозначающий добавление локального адреса
				 */
				void clearCache(const bool localhost = false) noexcept;
				/**
				 * @brief Метод очистки кэша
				 *
				 * @param family    семейстов IP-адресов IPv4/IPv6
				 * @param localhost флаг обозначающий добавление локального адреса
				 */
				void clearCache(const event::family_t family, const bool localhost = false) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain    доменное имя соответствующее IP-адресу
				 * @param ip        адрес для добавления к кэш
				 * @param ttl       время жизни кэша доменного имени
				 * @param localhost флаг обозначающий добавление локального адреса
				 */
				void addToCache(const string & domain, const string & ip, const uint32_t ttl, const bool localhost = false) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param domain    доменное имя соответствующее IP-адресу
				 * @param ip        адрес для добавления к кэш
				 * @param ttl       время жизни кэша доменного имени
				 * @param localhost флаг обозначающий добавление локального адреса
				 */
				void addToCache(const string & domain, const unique_ptr <net::addr_t> & ip, const uint32_t ttl, const bool localhost = false) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в кэш
				 *
				 * @param family    семейстов IP-адресов IPv4/IPv6
				 * @param domain    доменное имя соответствующее IP-адресу
				 * @param ip        адрес для добавления к кэш
				 * @param ttl       время жизни кэша доменного имени
				 * @param localhost флаг обозначающий добавление локального адреса
				 */
				void addToCache(const event::family_t family, const string & domain, const string & ip, const uint32_t ttl, const bool localhost = false) noexcept;
			public:
				/**
				 * @brief Метод очистки чёрного списка
				 *
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearBlacklist(const string & domain) noexcept;
				/**
				 * @brief Метод очистки чёрного списка
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearBlacklist(const event::family_t family, const string & domain) noexcept;
			public:
				/**
				 * @brief Метод удаления IP-адреса из чёрного списока
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInBlacklist(const string & domain, const string & ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списока
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInBlacklist(const string & domain, const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод удаления IP-адреса из чёрного списока
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInBlacklist(const event::family_t family, const string & domain, const string & ip) noexcept;
			public:
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void addToBlacklist(const string & domain, const string & ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void addToBlacklist(const string & domain, const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод добавления IP-адреса в чёрный список
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void addToBlacklist(const event::family_t family, const string & domain, const string & ip) noexcept;
			public:
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
				 */
				bool hasInBlacklist(const string & domain, const string & ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
				 */
				bool hasInBlacklist(const string & domain, const unique_ptr <net::addr_t> & ip) const noexcept;
				/**
				 * @brief Метод проверки наличия IP-адреса в чёрном списке
				 *
				 * @param family семейстов IP-адресов IPv4/IPv6
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для проверки наличия в чёрном списке
				 * @return       результат проверки наличия IP-адреса в чёрном списке
				 */
				bool hasInBlacklist(const event::family_t family, const string & domain, const string & ip) const noexcept;
			public:
				/**
				 * @brief Метод установки фдреса файла локальных хостов
				 *
				 * @param filename адрес файла для установки
				 */
				void setHostsFilename(const string & filename) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 */
				void setPrefixSnvironment(const string & prefix) noexcept;
			public:
				/**
				 * @brief Метод добавления сервера DNS
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param server адрес DNS-сервера
				 */
				void addServer(const event::id_t eid, const string & server) noexcept;
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
				void addServer(const event::id_t eid, const event::family_t family, const string & server) noexcept;
			public:
				/**
				 * @brief Метод установки серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param servers адреса DNS-серверов
				 */
				void setServers(const event::id_t eid, const vector <string> & servers) noexcept;
				/**
				 * @brief Метод установки серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param servers адреса DNS-серверов
				 */
				void setServers(const event::id_t eid, const vector <unique_ptr <net::addr_t>> & servers) noexcept;
				/**
				 * @brief Метод установки серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param family  семейстов IP-адресов IPv4/IPv6
				 * @param servers адреса DNS-серверов
				 */
				void setServers(const event::id_t eid, const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод замены существующих серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param servers адреса DNS-серверов
				 */
				void replaceServers(const event::id_t eid, const vector <string> & servers) noexcept;
				/**
				 * @brief Метод замены существующих серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param servers адреса DNS-серверов
				 */
				void replaceServers(const event::id_t eid, const vector <unique_ptr <net::addr_t>> & servers) noexcept;
				/**
				 * @brief Метод замены существующих серверов DNS
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param family  семейстов IP-адресов IPv4/IPv6
				 * @param servers адреса DNS-серверов
				 */
				void replaceServers(const event::id_t eid, const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса сети с которого будет выполняться запрос
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param source адрес сети для выполнения запроса
				 */
				void addSource(const event::id_t eid, const string & source) noexcept;
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
				void addSource(const event::id_t eid, const event::family_t family, const string & source) noexcept;
			public:
				/**
				 * @brief Метод установки адресов сети с которых будет выполняться запрос
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param sources адреса сети для выполнения запроса
				 */
				void setSources(const event::id_t eid, const vector <string> & sources) noexcept;
				/**
				 * @brief Метод установки адресов сети с которых будет выполняться запрос
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param sources адреса сети для выполнения запроса
				 */
				void setSources(const event::id_t eid, const vector <unique_ptr <net::addr_t>> & sources) noexcept;
				/**
				 * @brief Метод установки адресов сети с которых будет выполняться запрос
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param family  семейстов IP-адресов IPv4/IPv6
				 * @param sources адреса сети для выполнения запроса
				 */
				void setSources(const event::id_t eid, const event::family_t family, const vector <string> & sources) noexcept;
			public:
				/**
				 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    результат выполнения запроса
				 */
				bool reverse(const event::id_t eid, const string & ip) noexcept;
				/**
				 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    результат выполнения запроса
				 */
				bool reverse(const event::id_t eid, const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param ip     адрес для поиска доменного имени
				 * @return       результат выполнения запроса
				 */
				bool reverse(const event::id_t eid, const event::family_t family, const string & ip) noexcept;
			public:
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    список найденных доменных имён
				 */
				vector <string> search(const event::id_t eid, const string & ip) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid идентификатор события DNS-резолвера
				 * @param ip  адрес для поиска доменного имени
				 * @return    список найденных доменных имён
				 */
				vector <string> search(const event::id_t eid, const unique_ptr <net::addr_t> & ip) noexcept;
				/**
				 * @brief Метод поиска доменного имени соответствующего IP-адресу
				 *
				 * @param eid     идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param ip     адрес для поиска доменного имени
				 * @return       список найденных доменных имён
				 */
				vector <string> search(const event::id_t eid, const event::family_t family, const string & ip) noexcept;
			public:
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param domain доменное имя сервера
				 * @return       полученный IP-адрес
				 */
				bool request(const event::id_t eid, const string & domain) noexcept;
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param domain доменное имя сервера
				 * @return       полученный IP-адрес
				 */
				bool request(const event::id_t eid, const event::family_t family, const string & domain) noexcept;
			public:
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param domain доменное имя сервера
				 * @return       полученный IP-адрес
				 */
				string resolve(const event::id_t eid, const string & domain) noexcept;
				/**
				 * @brief Метод ресолвинга домена
				 *
				 * @param eid    идентификатор события DNS-резолвера
				 * @param family тип интернет-протокола IPv4/IPv6
				 * @param domain доменное имя сервера
				 * @return       полученный IP-адрес
				 */
				string resolve(const event::id_t eid, const event::family_t family, const string & domain) noexcept;
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
