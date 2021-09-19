/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_DNS_RESOLVER__
#define __AWH_DNS_RESOLVER__

/**
 * Стандартная библиотека
 */
#include <regex>
#include <string>
#include <vector>
#include <random>
#include <functional>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
// Если - это Unix
#else
	#include <ctime>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>
#include <nwk.hpp>

// Устанавливаем область видимости
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * DNS Класс dns ресолвера
	 */
	typedef class DNS {
		private:
			/**
			 * Структура доменного имени
			 */
			typedef struct Domain {
				int family;                                // Тип протокола интернета AF_INET или AF_INET6
				string host;                               // Название искомого домена
				DNS * dns;                                 // Объект резолвера DNS
				const fmk_t * fmk;                         // Объект основного фреймворка
				const log_t * log;                         // Объект для работы с логами
				unordered_map <string, string> * ips;      // Список IP адресов полученных ранее
				function <void (const string &)> callback; // Функция обратного вызова
				/**
				 * Domain Конструктор
				 */
				Domain() : family(AF_UNSPEC), host(""), dns(nullptr), fmk(nullptr), log(nullptr), ips(nullptr), callback(nullptr) {}
			} domain_t;
		private:
			// Адреса серверов dns
			vector <string> servers;
			// Список ранее полученых, IP адресов
			unordered_map <string, string> ips;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
			// База событий
			struct event_base * base = nullptr;
			// База dns
			struct evdns_base * dnsbase = nullptr;
			// Объект dns запроса
			struct evdns_getaddrinfo_request * reply = nullptr;
		private:
			/**
			 * createBase Метод создания dns базы
			 */
			void createBase() noexcept;
			/**
			 * callback Событие срабатывающееся при получении данных с dns сервера
			 * @param errcode ошибка dns сервера
			 * @param addr    структура данных с dns сервера
			 * @param ctx     объект с данными для запроса
			 */
			static void callback(const int errcode, struct evutil_addrinfo * addr, void * ctx) noexcept;
		public:
			/**
			 * init Метод инициализации DNS резолвера
			 * @param host   хост сервера
			 * @param family тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
			 * @param base   объект базы событий
			 * @return       база DNS резолвера
			 */
			struct evdns_base * init(const string & host, const int family, struct event_base * base) const;
		public:
			/**
			 * setBase Метод установки базы событий
			 * @param base объект базы событий
			 */
			void setBase(struct event_base * base) noexcept;
			/**
			 * setNameServer Метод добавления сервера dns
			 * @param server ip адрес dns сервера
			 */
			void setNameServer(const string & server) noexcept;
			/**
			 * setNameServers Метод добавления серверов dns
			 * @param servers ip адреса dns серверов
			 */
			void setNameServers(const vector <string> & servers) noexcept;
			/**
			 * replaceServers Метод замены существующих серверов dns
			 * @param servers ip адреса dns серверов
			 */
			void replaceServers(const vector <string> & servers) noexcept;
			/**
			 * resolve Метод ресолвинга домена
			 * @param host     хост сервера
			 * @param family   тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
			 * @param callback функция обратного вызова срабатывающая при получении данных
			 */
			void resolve(const string & host, const int family, function <void (const string &)> callback);
		public:
			/**
			 * DNS Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param nwk объект методов для работы с сетью
			 */
			DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk) noexcept;
			/**
			 * DNS Конструктор
			 * @param fmk     объект фреймворка
			 * @param log     объект для работы с логами
			 * @param nwk     объект методов для работы с сетью
			 * @param base    база событий
			 * @param servers массив dns серверов
			 */
			DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk, struct event_base * base, const vector <string> & servers) noexcept;
			/**
			 * ~DNS Деструктор
			 */
			~DNS() noexcept;
	} dns_t;
};

#endif // __AWH_DNS_RESOLVER__
