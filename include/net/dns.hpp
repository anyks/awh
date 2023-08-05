/**
 * @file: dns.hpp
 * @date: 2023-08-05
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_DNS_RESOLVER__
#define __AWH_DNS_RESOLVER__

/**
 * Стандартная библиотека
 */
#include <set>
#include <map>
#include <ctime>
#include <stack>
#include <mutex>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>

/**
 * Если операционной системой является Nix-подобная
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#define SOCKET int
	#define INVALID_SOCKET -1
#endif

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
/**
 * Если операционной системой является Nix-подобная
 */
#else
	#include <netdb.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Если используется модуль IDN и операционная система не MS Windows
 */
#if defined(AWH_IDN) && !defined(_WIN32) && !defined(_WIN64)
	#include <idn2.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/timer.hpp>
#include <net/net.hpp>
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
				recursive_mutex hold;  // Для работы со стеком событий
				recursive_mutex cache; // Для работы с кэшем DNS серверов 
				recursive_mutex black; // Для работы с чёрным списком DNS серверов
			} mtx_t;
		private:
			/**
			 * Header Структура заголовка DNS
			 */
			typedef struct Header {
				u_short id;        // Идентификатор операции
				u_char rd : 1;     // Флаг выполнения желаемой рекурсии
				u_char tc : 1;     // Флаг усечения сообщения если оно слишком большое
				u_char aa : 1;     // Флаг авторитетного ответа сервера
				u_char opcode : 4; // Опкод операции
				u_char qr : 1;     // Тип запроса или ответа
				u_char rcode : 4;  // Код выполнения операции
				u_char z : 3;      // Зарезервированно для использования в будущем
				u_char ra : 1;     // Флаг активации рекурсивных запросов на сервере
				u_short qdcount;   // Количество записей в разделе запроса
				u_short ancount;   // Количество записей в разделе ответа
				u_short nscount;   // Номер имени записи ресурсов сервера
				u_short arcount;   // Количество записей ресурсов в разделе дополнительных записей
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
			} rr_flags_t;
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
					// Файловый дескриптор (сокет)
					SOCKET _fd;
				private:
					// Флаг запуска резолвера
					bool _mode;
				private:
					// Идентификатор активного таймера
					size_t _tid;
				private:
					// Тип протокола интернета AF_INET или AF_INET6
					int _family;
				private:
					// Название искомого домена
					string _domain;
				private:
					// Объект таймера
					timer_t _timer;
				private:
					// Объект для работы с сокетами
					socket_t _socket;
				private:
					// Размер объекта подключения
					socklen_t _socklen;
				private:
					// Параметры подключения сервера
					struct sockaddr_storage _addr;
				private:
					// Объект DNS резолвера
					const DNS * _self;
				private:
					/**
					 * timeout Метод таймаута ожидания получения данных
					 */
					void timeout() noexcept;
				private:
					/**
					 * join Метод восстановления доменного имени
					 * @param domain доменное имя для восстановления
					 * @return       восстановленное доменное имя
					 */
					string join(const vector <u_char> & domain) const noexcept;
					/**
					 * split Метод разбивки доменного имени
					 * @param domain доменное имя для разбивки
					 * @return       разбитое доменное имя
					 */
					vector <u_char> split(const string & domain) const noexcept;
				private:
					/**
					 * extract Метод извлечения записи из ответа DNS
					 * @param data буфер данных из которого нужно извлечь запись
					 * @param pos  позиция в буфере данных
					 * @return     запись в текстовом виде из ответа DNS
					 */
					vector <u_char> extract(u_char * data, const size_t pos) const noexcept;
				public:
					/**
					 * close Метод закрытия подключения
					 */
					void close() noexcept;
				public:
					/**
					 * cancel Метод отмены выполнения запроса
					 */
					void cancel() noexcept;
				public:
					/**
					 * request Метод выполнения запроса
					 * @param domain название искомого домена
					 * @return       полученный IP-адрес
					 */
					string request(const string & domain) noexcept;
				public:
					/**
					 * Worker Конструктор
					 * @param family тип протокола интернета AF_INET или AF_INET6
					 * @param self   объект DNS резолвера
					 */
					Worker(const int family, const DNS * self) noexcept :
					 _fd(INVALID_SOCKET), _mode(false), _family(family),
					 _domain(""), _socket(self->_log), _socklen(0), _self(self) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept;
			} worker_t;
		private:
			// Таймаут ожидания выполнения запроса (в секундах)
			uint8_t _timeout;
		private:
			// Мютекс для блокировки основного потока
			mtx_t _mtx;
			// Статус работы DNS резолвера
			stack <status_t> _status;
		private:
			// Создаём воркер для IPv4
			unique_ptr <worker_t> _workerIPv4;
			// Создаём воркер для IPv6
			unique_ptr <worker_t> _workerIPv6;
		private:
			// Список используемых адресов
			unordered_set <string> _using;
		private:
			// Чёрный список IP-адресов
			map <int, set <string>> _blacklist;
			// Адреса серверов DNS
			map <int, vector <serv_t>> _servers;
			// Список ранее полученых, IP-адресов (горячий кэш)
			map <int, unordered_multimap <string, string>> _cache;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
			// Создаем объект сети
			const net_t * _net;
		public:
			/**
			 * Если используется модуль IDN
			 */
			#if defined(AWH_IDN)
				/**
				 * idnEncode Метод кодирования интернационального доменного имени
				 * @param domain доменное имя для кодирования
				 * @return       результат работы кодирования
				 */
				string idnEncode(const string & domain) const noexcept;
				/**
				 * idnDecode Метод декодирования интернационального доменного имени
				 * @param domain доменное имя для декодирования
				 * @return       результат работы декодирования
				 */
				string idnDecode(const string & domain) const noexcept;
			#endif
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
			 * cancel Метод отмены выполнения запроса
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void cancel(const int family) noexcept;
		public:
			/**
			 * shuffle Метод пересортировки серверов DNS
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void shuffle(const int family) noexcept;
		public:
			/**
			 * timeout Метод установки времени ожидания выполнения запроса
			 * @param sec интервал времени выполнения запроса в секундах
			 */
			void timeout(const uint8_t sec) noexcept;
		public:
			/**
			 * cache Метод получения IP-адреса из кэша
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
			 * @return       IP-адрес находящийся в кэше
			 */
			string cache(const int family, const string & domain) noexcept;
		public:
			/**
			 * clearCache Метод очистки кэша
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void clearCache(const int family) noexcept;
			/**
			 * setToCache Метод добавления IP-адреса в кэш
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
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
			 * delInBlackList Метод удаления IP-адреса из чёрного списока
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackList(const int family, const string & ip) noexcept;
			/**
			 * setToBlackList Метод добавления IP-адреса в чёрный список
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
			 * isInBlackList Метод проверки наличия IP-адреса в чёрном списке
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для проверки наличия в чёрном списке
			 * @return       результат проверки наличия IP-адреса в чёрном списке
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
			 * resolve Метод ресолвинга домена
			 * @param host   хост сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       полученный IP-адрес
			 */
			string resolve(const string & host, const int family) noexcept;
		public:
			/**
			 * getHostByName Метод определение локального IP-адреса по имени домена
			 * @param host   хост сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       полученный IP-адрес
			 */
			string getHostByName(const string & host, const int family) noexcept;
		public:
			/**
			 * DNS Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param net объект методов для работы с сетью
			 */
			DNS(const fmk_t * fmk, const log_t * log, const net_t * net) noexcept;
			/**
			 * ~DNS Деструктор
			 */
			~DNS() noexcept;
	} dns_t;
};

#endif // __AWH_DNS_RESOLVER__
