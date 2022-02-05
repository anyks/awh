/**
 * @file: dns.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_DNS_RESOLVER__
#define __AWH_DNS_RESOLVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <regex>
#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <unordered_map>
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

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * DNS Класс dns ресолвера
	 */
	typedef class DNS {
		private:
			/**
			 * Worker Структура воркера резолвинга
			 */
			typedef struct Worker {
				size_t id;                                       // Идентификатор объекта доменного имени
				int family;                                      // Тип протокола интернета AF_INET или AF_INET6
				string host;                                     // Название искомого домена
				void * context;                                  // Передаваемый внешний контекст
				const DNS * dns;                                 // Объект резолвера DNS
				const fmk_t * fmk;                               // Объект основного фреймворка
				const log_t * log;                               // Объект для работы с логами
				function <void (const string, void *)> callback; // Функция обратного вызова
				/**
				 * Worker Конструктор
				 */
				Worker() : id(0), family(AF_UNSPEC), host(""), context(nullptr), dns(nullptr), fmk(nullptr), log(nullptr), callback(nullptr) {}
			} worker_t;
		private:
			// Адреса серверов dns
			mutable vector <string> servers;
			// Список воркеров резолвинга доменов
			mutable map <size_t, worker_t> workers;
			// Список ранее полученых, IP адресов (горячий кэш)
			mutable unordered_multimap <string, string> cache;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаем объект сети
			const network_t * nwk = nullptr;
		private:
			// База событий
			struct event_base * base = nullptr;
			// База dns резолвера
			struct evdns_base * dbase = nullptr;
			// Объект dns запроса
			struct evdns_getaddrinfo_request * reply = nullptr;
		private:
			/**
			 * createBase Метод создания dns базы
			 */
			void createBase() noexcept;
			/**
			 * callback Событие срабатывающееся при получении данных с dns сервера
			 * @param error ошибка dns сервера
			 * @param addr  структура данных с dns сервера
			 * @param ctx   объект с данными для запроса
			 */
			static void callback(const int error, struct evutil_addrinfo * addr, void * ctx) noexcept;
		public:
			/**
			 * init Метод инициализации DNS резолвера
			 * @param host   хост сервера
			 * @param family тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
			 * @param base   объект базы событий
			 * @return       база DNS резолвера
			 */
			struct evdns_base * init(const string & host, const int family, struct event_base * base) const noexcept;
		public:
			/**
			 * reset Метод сброса параметров модуля DNS резолвера
			 */
			void reset() noexcept;
			/**
			 * clear Метод сброса кэша резолвера
			 */
			void clear() noexcept;
			/**
			 * flush Метод сброса кэша DNS резолвера
			 */
			void flush() noexcept;
		public:
			/**
			 * updateNameServers Метод обновления списка нейм-серверов
			 */
			void updateNameServers() noexcept;
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
			 * @param ctx      передаваемый контекст
			 * @param host     хост сервера
			 * @param family   тип интернет протокола AF_INET, AF_INET6 или AF_UNSPEC
			 * @param callback функция обратного вызова срабатывающая при получении данных
			 */
			void resolve(void * ctx, const string & host, const int family, function <void (const string, void *)> callback) noexcept;
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
