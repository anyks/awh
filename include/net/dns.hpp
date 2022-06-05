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
#include <set>
#include <map>
#include <stack>
#include <mutex>
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
#include <event2/event_struct.h>

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
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/nwk.hpp>

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
			 * Статус работы DNS резолвера
			 */
			enum class status_t : uint8_t {
				NONE,        // Событие не установлено
				CLEAR,       // Событие отчистки параметров резолвера
				FLUSH,       // Событие сброса кэша DNS серверов
				ZOMBIE,      // Событие очистки зависших процессов
				REMOVE,      // Событие удаления параметров резолвера
				CANCEL,      // Событие отмены резолвинга домена
				RESOLVE,     // Событие резолвинга домена
				SET_BASE,    // Событие установки базы событий
				NS_SET,      // Событие установки DNS сервера
				NSS_SET,     // Событие установки DNS серверов
				NSS_REP,     // Событие замены DNS серверов
				NSS_UPDATE,  // Событие обновления DNS серверов
				CREATE_EVDNS // Событие создания DNS базы событий
			};
		private:
			/**
			 * Mutex Объект основных мютексов
			 */
			typedef struct Mutex {
				recursive_mutex hold;      // Для работы со стеком событий
				recursive_mutex base;      // Для работы с базой событий
				recursive_mutex evdns;     // Для работы с базой событий evdns
				recursive_mutex cache;     // Для работы с кэшем DNS серверов 
				recursive_mutex worker;    // Для работы с воркерами
				recursive_mutex servers;   // Для работы со списком DNS серверов
				recursive_mutex blacklist; // Для работы с чёрным списком DNS серверов
			} mtx_t;
		private:
			/**
			 * Holder Класс холдера
			 */
			typedef class Holder {
				private:
					// Флаг холдирования
					bool flag;
				private:
					// Объект статуса работы DNS резолвера
					stack <status_t> * status;
				public:
					/**
					 * access Метод проверки на разрешение выполнения операции
					 * @param comp  статус сравнения
					 * @param hold  статус установки
					 * @param equal флаг эквивалентности
					 * @return      результат проверки
					 */
					bool access(const set <status_t> & comp, const status_t hold, const bool equal = true) noexcept;
				public:
					/**
					 * Holder Конструктор
					 * @param status объект статуса работы DNS резолвера
					 */
					Holder(stack <status_t> * status) noexcept : flag(false), status(status) {}
					/**
					 * ~Holder Деструктор
					 */
					~Holder() noexcept;
			} hold_t;
			/**
			 * Worker Класс воркера резолвинга
			 */
			typedef class Worker {
				public:
					// Идентификатор объекта доменного имени
					size_t did;
				public:
					// Тип протокола интернета AF_INET или AF_INET6
					int family;
				public:
					// Название искомого домена
					string host;
				public:
					// Событие таймаута ожидания
					struct event ev;
					// Структура таймаута ожидания
					struct timeval tv;
				public:
					// Передаваемый внешний контекст
					void * context;
				public:
					// Объект DNS данных полученных с сервера
					struct evutil_addrinfo hints;
				public:
					const DNS * dns;                          // Объект резолвера DNS
					struct evdns_getaddrinfo_request * reply; // Объект DNS запроса
				public:
					// Функция обратного вызова
					function <void (const string, void *)> callbackFn;
				public:
					/**
					 * Worker Конструктор
					 */
					Worker() noexcept : did(0), family(AF_UNSPEC), host(""), context(nullptr), dns(nullptr), reply(nullptr), callbackFn(nullptr) {
						// Заполняем структуру запроса нулями
						memset(&this->hints, 0, sizeof(this->hints));
					}
			} worker_t;
		private:
			// Мютекс для блокировки основного потока
			mutable mtx_t mtx;
			// Статус работы DNS резолвера
			mutable stack <status_t> status;
		private:
			// Чёрный список IP адресов
			mutable set <string> blacklist;
			// Адреса серверов dns
			mutable vector <string> servers;
			// Список ранее полученых, IP адресов (горячий кэш)
			mutable unordered_multimap <string, string> cache;
			// Список воркеров резолвинга доменов
			mutable map <size_t, unique_ptr <worker_t>> workers;
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
		private:
			/**
			 * createBase Метод создания dns базы
			 */
			void createBase() noexcept;
			/**
			 * clearZombie Метод очистки зомби-запросов
			 */
			void clearZombie() noexcept;
		private:
			/**
			 * garbage Функция выполняемая по таймеру для чистки мусора
			 * @param fd    файловый дескриптор (сокет)
			 * @param event произошедшее событие
			 * @param ctx   передаваемый контекст
			 */
			static void garbage(evutil_socket_t fd, short event, void * ctx) noexcept;
			/**
			 * callback Событие срабатывающееся при получении данных с dns сервера
			 * @param error ошибка dns сервера
			 * @param addr  структура данных с dns сервера
			 * @param ctx   объект с данными для запроса
			 */
			static void callback(const int error, struct evutil_addrinfo * addr, void * ctx) noexcept;
		public:
			/**
			 * clear Метод сброса кэша резолвера
			 * @return результат работы функции
			 */
			bool clear() noexcept;
			/**
			 * flush Метод сброса кэша DNS резолвера
			 * @return результат работы функции
			 */
			bool flush() noexcept;
			/**
			 * remove Метод удаления параметров модуля DNS резолвера
			 * @return результат работы функции
			 */
			bool remove() noexcept;
		public:
			/**
			 * cancel Метод отмены выполнения запроса
			 * @param did идентификатор DNS запроса
			 * @return    результат работы функции
			 */
			bool cancel(const size_t did = 0) noexcept;
		public:
			/**
			 * updateNameServers Метод обновления списка нейм-серверов
			 */
			void updateNameServers() noexcept;
		public:
			/**
			 * clearBlackList Метод очистки чёрного списка
			 */
			void clearBlackList() noexcept;
			/**
			 * delInBlackList Метод удаления IP адреса из чёрного списока
			 * @param ip адрес для удаления из чёрного списка
			 */
			void delInBlackList(const string & ip) noexcept;
			/**
			 * setToBlackList Метод добавления IP адреса в чёрный список
			 * @param ip адрес для добавления в чёрный список
			 */
			void setToBlackList(const string & ip) noexcept;
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
			 * @return         идентификатор DNS запроса
			 */
			size_t resolve(void * ctx, const string & host, const int family, function <void (const string, void *)> callback) noexcept;
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
