/**
 * @file: dns.hpp
 * @date: 2022-08-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
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
#include <libev/ev++.h>

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
/**
 * Если операционной системой является Nix-подобная
 */
#else
	#include <ctime>
	#include <netdb.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/nwk.hpp>
#include <net/socket.hpp>

// Устанавливаем область видимости
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Прототип класса DNS резолвера
	 */
	class DNS;
	/**
	 * DNS Класс DNS ресолвера
	 */
	typedef class DNS {
		public:
			/**
			 * Server Структура сервера
			 */
			typedef struct Server {
				u_int port;  // Порт сервера
				string host; // Хост сервера
				/**
				 * Server Конструктор
				 */
				Server() noexcept : port(53), host("") {}
			} serv_t;
		private:
			/**
			 * Статус работы DNS резолвера
			 */
			enum class status_t : uint8_t {
				NONE    = 0x00, // Событие не установлено
				CLEAR   = 0x01, // Событие отчистки параметров резолвера
				FLUSH   = 0x02, // Событие сброса кэша DNS серверов
				ZOMBIE  = 0x03, // Событие очистки зависших процессов
				CANCEL  = 0x04, // Событие отмены резолвинга домена
				RESOLVE = 0x05, // Событие резолвинга домена
				NS_SET  = 0x06, // Событие установки DNS сервера
				NSS_SET = 0x07, // Событие установки DNS серверов
				NSS_REP = 0x08  // Событие замены DNS серверов
			};
		private:
			/**
			 * Mutex Объект основных мютексов
			 */
			typedef struct Mutex {
				recursive_mutex hold;   // Для работы со стеком событий
				recursive_mutex cache;  // Для работы с кэшем DNS серверов 
				recursive_mutex black;  // Для работы с чёрным списком DNS серверов
				recursive_mutex worker; // Для работы с воркерами
			} mtx_t;
		private:
			/**
			 * Header Структура заголовка запроса DNS
			 */
			typedef struct Header {
				u_short id;
				u_char rd : 1;
				u_char tc : 1;
				u_char aa : 1;
				u_char opcode : 4;
				u_char qr : 1;
				u_char rcode : 4;
				u_char z : 3;
				u_char ra : 1;
				u_short qdcount;
				u_short ancount;
				u_short nscount;
				u_short arcount;
				/**
				 * Header Конструктор
				 */
				Header() noexcept : qdcount(0), ancount(0), nscount(0), arcount(0) {}
			} head_t;
			/**
			 * QFlags Структура флагов DNS запросов
			 */
			typedef struct QFlags {
				u_short qtype;  // Тип записи
				u_short qclass; // Класс записи
				/**
				 * QFlags Конструктор
				 */
				QFlags() noexcept : qtype(0), qclass(0) {}
			} qflags_t;
			/**
			 * RRFlags Структура флагов DNS RRs
			 */
			typedef struct RRFlags {
				u_short rtype;    // Тип записи
				u_short rclass;   // Класс записи
				u_int ttl;        // Время обновления
				u_short rdlength; // Длина записи
				/**
				 * RRFlags Конструктор
				 */
				RRFlags() noexcept : rtype(0), rclass(0), ttl(0), rdlength(0) {}
			} rrflags_t;
			/**
			 * Holder Класс холдера
			 */
			typedef class Holder {
				private:
					// Флаг холдирования
					bool _flag;
				private:
					// Объект статуса работы DNS резолвера
					stack <status_t> * _status;
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
					Holder(stack <status_t> * status) noexcept : _flag(false), _status(status) {}
					/**
					 * ~Holder Деструктор
					 */
					~Holder() noexcept;
			} hold_t;
			/**
			 * Worker Класс воркера резолвинга
			 */
			typedef class Worker {
				private:
					/**
					 * DNS Устанавливаем дружбу с классом DNS резолвера
					 */
					friend class DNS;
				private:
					// Идентификатор DNS запроса
					size_t _did;
				private:
					// Файловый дескриптор (сокет)
					int _fd;
					// Тип протокола интернета AF_INET или AF_INET6
					int _family;
				private:
					// Название искомого домена
					string _domain;
				private:
					// Объект работы резолвера
					ev::io _io;
					// Объект события таймера
					ev::timer _timer;
				private:
					// Объект DNS резолвера
					const DNS * _dns;
				private:
					// База событий
					struct ev_loop * _base;
				private:
					/**
					 * response Событие срабатывающееся при получении данных с DNS сервера
					 * @param io      объект события чтения
					 * @param revents идентификатор события
					 */
					void response(ev::io & io, int revents) noexcept;
					/**
					 * timeout Функция выполняемая по таймеру для чистки мусора
					 * @param timer   объект события таймера
					 * @param revents идентификатор события
					 */
					void timeout(ev::timer & timer, int revents) noexcept;
				private:
					/**
					 * join Метод восстановления доменного имени
					 * @param domain доменное имя для восстановления
					 * @return       восстановленное доменное имя
					 */
					string join(const string & domain) const noexcept;
					/**
					 * split Метод разбивки доменного имени
					 * @param domain доменное имя для разбивки
					 * @return       разбитое доменное имя
					 */
					string split(const string & domain) const noexcept;
					/**
					 * extract Метод извлечения записи из ответа DNS
					 * @param data буфер данных из которого нужно извлечь запись
					 * @param pos  позиция в буфере данных
					 * @return     запись в текстовом виде из ответа DNS
					 */
					string extract(u_char * data, const size_t pos) const noexcept;
				public:
					/**
					 * close Метод закрытия подключения
					 */
					void close() noexcept;
				public:
					/**
					 * request Метод выполнения запроса
					 * @param domain название искомого домена
					 * @return       результат выполнения запроса
					 */
					bool request(const string & domain) noexcept;
				public:
					/**
					 * Worker Конструктор
					 * @param did    идентификатор DNS запроса
					 * @param family тип протокола интернета AF_INET или AF_INET6
					 * @param base   база событий
					 * @param dns    объект DNS резолвера
					 */
					Worker(const size_t did, const int family, struct ev_loop * base, const DNS * dns) noexcept;
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept;
			} worker_t;
		private:
			// Таймаут ожидания выполнения запроса (в секундах)
			time_t _timeout;
		private:
			// Мютекс для блокировки основного потока
			mtx_t _mtx;
			// Статус работы DNS резолвера
			stack <status_t> _status;
		private:
			// Чёрный список IP адресов
			map <int, set <string>> _blacklist;
			// Адреса серверов DNS
			map <int, vector <serv_t>> _servers;
			// Список воркеров резолвинга доменов
			map <size_t, unique_ptr <worker_t>> _workers;
			// Список ранее полученых, IP адресов (горячий кэш)
			map <int, unordered_multimap <string, string>> _cache;
		private:
			// Список внутренних DNS резолверов
			map <size_t, unique_ptr <DNS>> _dns;
		private:
			// Функция обратного вызова
			function <void (const string &, const int, const size_t)> _fn;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
			// Создаем объект сети
			const network_t * _nwk;
		private:
			// База событий
			struct ev_loop * _base;
		private:
			/**
			 * clearZombie Метод очистки зомби-запросов
			 */
			void clearZombie() noexcept;
		private:
			/**
			 * resolving Метод получения IP адреса доменного имени
			 * @param ip     адрес интернет-подключения
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param did    идентификатор DNS запроса
			 */
			void resolving(const string & ip, const int family, const size_t did) noexcept;
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
		public:
			/**
			 * base Метод установки базы событий
			 * @param base объект базы событий
			 */
			void base(struct ev_loop * base) noexcept;
		public:
			/**
			 * cancel Метод отмены выполнения запроса
			 * @param did идентификатор DNS запроса
			 * @return    результат работы функции
			 */
			bool cancel(const size_t did = 0) noexcept;
		public:
			/**
			 * timeout Метод установки времени ожидания выполнения запроса
			 * @param sec интервал времени выполнения запроса в секундах
			 */
			void timeout(const time_t sec) noexcept;
		public:
			/**
			 * cache Метод получения IP адреса из кэша
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP адресу
			 * @return       IP адрес находящийся в кэше
			 */
			string cache(const int family, const string & domain) noexcept;
		public:
			/**
			 * clearCache Метод очистки кэша
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void clearCache(const int family) noexcept;
			/**
			 * setToCache Метод добавления IP адреса в кэш
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP адресу
			 * @param ip     адрес для добавления к кэш
			 */
			void setToCache(const int family, const string & domain, const string & ip) noexcept;
		public:
			/**
			 * clearBlackList Метод очистки чёрного списка
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void clearBlackList(const int family) noexcept;
			/**
			 * delInBlackList Метод удаления IP адреса из чёрного списока
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackList(const int family, const string & ip) noexcept;
			/**
			 * setToBlackList Метод добавления IP адреса в чёрный список
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для добавления в чёрный список
			 */
			void setToBlackList(const int family, const string & ip) noexcept;
		public:
			/**
			 * emptyBlackList Метод проверки заполненности чёрного списка
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       результат проверки чёрного списка
			 */
			bool emptyBlackList(const int family) const noexcept;
			/**
			 * isInBlackList Метод проверки наличия IP адреса в чёрном списке
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для проверки наличия в чёрном списке
			 * @return       результат проверки наличия IP адреса в чёрном списке
			 */
			bool isInBlackList(const int family, const string & ip) const noexcept;
		public:
			/**
			 * server Метод получения данных сервера имён
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       запрошенный сервер имён
			 */
			const serv_t & server(const int family) noexcept;
			/**
			 * server Метод добавления сервера DNS
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param server параметры DNS сервера
			 */
			void server(const int family, const serv_t & server) noexcept;
			/**
			 * servers Метод добавления серверов DNS
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers параметры DNS серверов
			 */
			void servers(const int family, const vector <serv_t> & servers) noexcept;
			/**
			 * replace Метод замены существующих серверов DNS
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers параметры DNS серверов
			 */
			void replace(const int family, const vector <serv_t> & servers = {}) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова для получения данных
			 * @param callback функция обратного вызова срабатывающая при получении данных
			 */
			void on(function <void (const string &, const int, const size_t)> callback) noexcept;
		public:
			/**
			 * resolve Метод ресолвинга домена
			 * @param host   хост сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       идентификатор DNS запроса
			 */
			size_t resolve(const string & host, const int family) noexcept;
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
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 * @param nwk  объект методов для работы с сетью
			 * @param base база событий
			 */
			DNS(const fmk_t * fmk, const log_t * log, const network_t * nwk, struct ev_loop * base) noexcept;
			/**
			 * ~DNS Деструктор
			 */
			~DNS() noexcept;
	} dns_t;
};

#endif // __AWH_DNS_RESOLVER__
