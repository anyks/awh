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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_DNS_RESOLVER__
#define __AWH_DNS_RESOLVER__

/**
 * Стандартные модули
 */
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
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	#define SOCKET int32_t
	#define INVALID_SOCKET -1
#endif

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	#include <winsock2.h>
	#include <ws2tcpip.h>
/**
 * Для операционной системы не являющейся OS Windows
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
#if AWH_IDN && !_WIN32 && !_WIN64
	#include <idn2.h>
#endif

/**
 * Наши модули
 */
#include "net.hpp"
#include "socket.hpp"
#include "../sys/fs.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/hold.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс DNS ресолвера
	 *
	 */
	typedef class AWHSHARED_EXPORT DNS {
		private:
			/**
			 * Статус работы DNS-резолвера
			 */
			enum class status_t : uint8_t {
				NONE    = 0x00, // Событие не установлено
				CLEAR   = 0x01, // Событие отчистки параметров резолвера
				FLUSH   = 0x02, // Событие сброса кэша DNS-серверов
				RESOLVE = 0x03, // Событие резолвинга домена
				NS_SET  = 0x04, // Событие установки DNS-сервера
				NSS_SET = 0x05, // Событие установки DNS-серверов
				NSS_REP = 0x06, // Событие замены DNS-серверов
				NET_SET = 0x07  // Событие установки параметров сети
			};
		private:
			/**
			 * @brief Шаблон формата данных DNS-сервера
			 *
			 * @tparam T размерность буфера DNS-сервера
			 */
			template <uint8_t T>
			/**
			 * @brief Структура сервера имён
			 *
			 */
			struct Server {
				uint32_t port;  // Порт сервера
				uint32_t ip[T]; // Буфер IP-адреса
				/**
				 * @brief Конструктор
				 *
				 */
				Server() noexcept : port(53), ip{0} {}
			} __attribute__((packed));
			/**
			 * @brief Шаблон формата данных DNS-кэша
			 *
			 * @tparam T размерность буфера DNS-кэша
			 */
			template <uint8_t T>
			/**
			 * @brief Структура кэша DNS
			 *
			 */
			struct Cache {
				uint32_t ttl;    // Время жизни кэша
				uint64_t create; // Время создания кэша
				bool localhost;  // Флаг локального адреса
				bool forbidden;  // Флаг запрещённого адреса
				uint32_t ip[T];  // Буфер IP-адреса
				/**
				 * @brief Конструктор
				 *
				 */
				Cache() noexcept :
				 ttl(0), create(0),
				 localhost(false),
				 forbidden(false), ip{0} {}
			} __attribute__((packed));
			/**
			 * Шаблон формата данных DNS-кэша
			 * @tclass T размерность буфера DNS-кэша
			 */
			template <uint8_t T>
			// Создаём тип данных работы с DNS-кэшем
			using cache_t = Cache <T>;
			/**
			 * @brief Шаблон формата данных DNS-сервера
			 *
			 * @tparam T размерность буфера DNS-сервера
			 */
			template <uint8_t T>
			// Создаём тип данных работы с DNS-серверами
			using server_t = Server <T>;
		private:
			/**
			 * @brief Структура извлекаемой записи
			 *
			 */
			typedef struct Item {
				uint32_t ttl;          // Время жизни записи в секундах
				uint32_t type;         // Тип полученной записи
				string record;         // Данные полученной записи
				vector <string> items; // Составные части доменного имени
				/**
				 * @brief Конструктор
				 *
				 */
				Item() noexcept : ttl(0), type(0), record{""} {}
			} item_t;
			/**
			 * @brief Структура подключения
			 *
			 */
			typedef struct Peer {
				socklen_t size;                 // Размер объекта подключения
				struct sockaddr_storage client; // Параметры подключения клиента
				struct sockaddr_storage server; // Параметры подключения сервера
				/**
				 * @brief Конструктор
				 *
				 */
				Peer() noexcept : size(0), client{}, server{} {}
			} peer_t;
			/**
			 * @brief Класс бинарного буфера данных
			 *
			 */
			typedef class AWHSHARED_EXPORT Buffer {
				public:
					/**
					 * Тип бинарного буфера
					 */
					enum class type_t : uint8_t {
						NONE = 0x00, // Буфер не определён
						ADDR = 0x01, // Буфер является IP-адресом
						DATA = 0x02  // Буфер является обменником
					};
				private:
					// Буфер для обмена данными с DNS-сервером
					uint8_t _data[AWH_DATA_SIZE];
					// Буфер для извлечения IP-адреса
					uint8_t _addr[INET6_ADDRSTRLEN];
				public:
					/**
					 * @brief Метод получения бинарных данных
					 *
					 * @param type тип бинарного буфера данных
					 * @return     бинарные данные буфера
					 */
					uint8_t * get(const type_t type) noexcept;
				public:
					/**
					 * @brief Метод очистки бинарного буфера данных
					 *
					 * @param type   тип бинарного буфера данных
					 * @param family тип интернет-протокола AF_INET, AF_INET6
					 */
					void clear(const type_t type, const int32_t family = AF_INET6) noexcept;
				public:
					/**
					 * @brief Метод получения размера буфера
					 *
					 * @param type   тип бинарного буфера данных
					 * @param family тип интернет-протокола AF_INET, AF_INET6
					 * @return       размер бинарного буфера данных
					 */
					size_t size(const type_t type, const int32_t family = AF_INET6) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Buffer() noexcept : _data{0}, _addr{0} {}
			} __attribute__((packed)) buffer_t;
			/**
			 * @brief Структура заголовка DNS
			 *
			 */
			typedef struct Header {
				uint16_t id;        // Идентификатор операции
				uint8_t rd : 1;     // Флаг выполнения желаемой рекурсии
				uint8_t tc : 1;     // Флаг усечения сообщения если оно слишком большое
				uint8_t aa : 1;     // Флаг авторитетного ответа сервера
				uint8_t opcode : 4; // Опкод операции
				uint8_t qr : 1;     // Тип запроса или ответа
				uint8_t rcode : 4;  // Код выполнения операции
				uint8_t z : 3;      // Зарезервированно для использования в будущем
				uint8_t ra : 1;     // Флаг активации рекурсивных запросов на сервере
				uint16_t qdcount;   // Количество записей в разделе запроса
				uint16_t ancount;   // Количество записей в разделе ответа
				uint16_t nscount;   // Номер имени записи ресурсов сервера
				uint16_t arcount;   // Количество записей ресурсов в разделе дополнительных записей
				/**
				 * @brief Конструктор
				 *
				 */
				Header() noexcept :
				 qdcount(0), ancount(0),
				 nscount(0), arcount(0) {}
			} __attribute__((packed)) head_t;
			/**
			 * @brief Структура флагов DNS запросов
			 *
			 */
			typedef struct QFlags {
				uint16_t type; // Тип записи
				uint16_t cls;  // Класс записи
				/**
				 * @brief Конструктор
				 *
				 */
				QFlags() noexcept : type(0), cls(0) {}
			} __attribute__((packed)) q_flags_t;
			/**
			 * @brief Структура флагов DNS RRs
			 *
			 */
			typedef struct RRFlags {
				uint16_t type;   // Тип записи
				uint16_t cls;    // Класс записи
				uint32_t ttl;    // Время обновления
				uint16_t length; // Длина записи
				/**
				 * @brief Конструктор
				 *
				 */
				RRFlags() noexcept : type(0), cls(0), ttl(0), length(0) {}
			} rr_flags_t;
			/**
			 * @brief Класс воркера резолвинга
			 *
			 */
			typedef class AWHSHARED_EXPORT Worker {
				private:
					/**
					 * @brief Устанавливаем дружбу с классом DNS-резолвера
					 *
					 */
					friend class DNS;
				private:
					/**
					 * Тип запроса
					 */
					enum class q_type_t : uint8_t {
						IP  = 0x01, // Тип запроса IP-адрес
						PTR = 0x02  // Тип запроса PTR-адрес
					};
				private:
					// Сетевой сокет
					SOCKET _sock;
				private:
					// Флаг запуска резолвера
					bool _mode;
				private:
					// Тип протокола интернета AF_INET или AF_INET6
					int32_t _family;
				private:
					// Тип DNS-запроса
					q_type_t _qtype;
				private:
					// Объект для работы с подключениями
					peer_t _peer;
					// Объект для работы с сокетами
					socket_t _socket;
				private:
					// Список сетевых интерфейсов
					vector <string> _network;
				private:
					// Объект DNS-резолвера
					const DNS * _self;
				private:
					/**
					 * @brief Метод извлечения хоста компьютера
					 *
					 * @return хост компьютера с которого производится запрос
					 */
					string host() const noexcept;
				private:
					/**
					 * @brief Метод разбивки доменного имени
					 *
					 * @param domain доменное имя для разбивки
					 * @return       разбитое доменное имя
					 */
					vector <uint8_t> split(const string & domain) const noexcept;
				private:
					/**
					 * @brief Метод извлечения записи из ответа DNS
					 *
					 * @param data буфер данных из которого нужно извлечь запись
					 * @param pos  позиция в буфере данных
					 * @return     запись в текстовом виде из ответа DNS
					 */
					string extract(const uint8_t * data, const size_t pos) const noexcept;
					/**
					 * @brief Метод восстановления доменного имени
					 *
					 * @param buffer буфер бинарных данных записи
					 * @param size   размер буфера бинарных данных
					 * @return       восстановленное доменное имя
					 */
					string join(const uint8_t * buffer, const size_t size) const noexcept;
				private:
					/**
					 * @brief Метод извлечения частей доменного имени
					 *
					 * @param buffer буфер бинарных данных записи
					 * @param size   размер буфера бинарных данных
					 * @return       восстановленное доменное имя
					 */
					vector <string> items(const uint8_t * buffer, const size_t size) const noexcept;
				public:
					/**
					 * @brief Метод закрытия подключения
					 *
					 */
					void close() noexcept;
				public:
					/**
					 * @brief Метод отмены выполнения запроса
					 *
					 */
					void cancel() noexcept;
				public:
					/**
					 * @brief Метод выполнения запроса
					 *
					 * @param domain название искомого домена
					 * @return       полученный IP-адрес
					 */
					string request(const string & domain) noexcept;
				private:
					/**
					 * Метод отправки запроса на удалённый сервер DNS
					 * @param fqdn полное доменное имя для которого выполняется отправка запроса
					 * @param from адрес компьютера с которого выполняется запрос
					 * @param to   адрес DNS-сервера на который выполняется запрос
					 * @return     полученный IP-адрес
					 */
					string send(const string & fqdn, const string & from, const string & to) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param family тип протокола интернета AF_INET или AF_INET6
					 * @param self   объект DNS-резолвера
					 */
					Worker(const int32_t family, const DNS * self) noexcept :
					 _sock(INVALID_SOCKET), _mode(false), _family(family),
					 _qtype(q_type_t::IP), _socket(self->_fmk, self->_log), _self(self) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Worker() noexcept;
			} worker_t;
		private:
			// Объект IP-адресов
			net_t _net;
		private:
			// Таймаут ожидания выполнения запроса (в секундах)
			uint8_t _timeout;
		private:
			// Префикс переменной окружения
			string _prefix;
		private:
			// Буфер бинарных данных
			buffer_t _buffer;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Выполняем инициализацию генератора
			std::random_device _randev;
		private:
			// Статус работы DNS-резолвера
			std::stack <status_t> _status;
		private:
			// Список используемых адресов
			std::unordered_set <string> _using;
		private:
			// Создаём воркер для IPv4
			std::unique_ptr <worker_t> _workerIPv4;
			// Создаём воркер для IPv6
			std::unique_ptr <worker_t> _workerIPv6;
		private:
			// Адреса серверов имён DNS для IPv4
			vector <server_t <1>> _serversIPv4;
			// Адреса серверов имён DNS для IPv6
			vector <server_t <4>> _serversIPv6;
		private:
			// Список кэша полученных IPv4-адресов
			std::unordered_multimap <string, cache_t <1>> _cacheIPv4;
			// Список кэша полученных IPv6-адресов
			std::unordered_multimap <string, cache_t <4>> _cacheIPv6;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
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
			 * @brief Метод отмены выполнения запроса
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void cancel(const int32_t family) noexcept;
		public:
			/**
			 * @brief Метод пересортировки серверов DNS
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void shuffle(const int32_t family) noexcept;
		public:
			/**
			 * @brief Метод установки времени ожидания выполнения запроса
			 *
			 * @param sec интервал времени выполнения запроса в секундах
			 */
			void timeout(const uint8_t sec) noexcept;
		public:
			/**
			 * @brief Метод получения IP-адреса из кэша
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
			 * @return       IP-адрес находящийся в кэше
			 */
			string cache(const int32_t family, const string & domain) noexcept;
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
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя для которого выполняется очистка кэша
			 */
			void clearCache(const int32_t family, const string & domain) noexcept;
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
			 * @param family    тип интернет-протокола AF_INET, AF_INET6
			 * @param localhost флаг обозначающий добавление локального адреса
			 */
			void clearCache(const int32_t family, const bool localhost = false) noexcept;
		public:
			/**
			 * @brief Метод добавления IP-адреса в кэш
			 *
			 * @param domain    доменное имя соответствующее IP-адресу
			 * @param ip        адрес для добавления к кэш
			 * @param ttl       время жизни кэша доменного имени
			 * @param localhost флаг обозначающий добавление локального адреса
			 */
			void setToCache(const string & domain, const string & ip, const uint32_t ttl, const bool localhost = false) noexcept;
			/**
			 * @brief Метод добавления IP-адреса в кэш
			 *
			 * @param family    тип интернет-протокола AF_INET, AF_INET6
			 * @param domain    доменное имя соответствующее IP-адресу
			 * @param ip        адрес для добавления к кэш
			 * @param ttl       время жизни кэша доменного имени
			 * @param localhost флаг обозначающий добавление локального адреса
			 */
			void setToCache(const int32_t family, const string & domain, const string & ip, const uint32_t ttl, const bool localhost = false) noexcept;
		public:
			/**
			 * @brief Метод очистки чёрного списка
			 *
			 * @param domain доменное имя для которого очищается чёрный список
			 */
			void clearBlackList(const string & domain) noexcept;
			/**
			 * @brief Метод очистки чёрного списка
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя для которого очищается чёрный список
			 */
			void clearBlackList(const int32_t family, const string & domain) noexcept;
		public:
			/**
			 * @brief Метод удаления IP-адреса из чёрного списока
			 *
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackList(const string & domain, const string & ip) noexcept;
			/**
			 * @brief Метод удаления IP-адреса из чёрного списока
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для удаления из чёрного списка
			 */
			void delInBlackList(const int32_t family, const string & domain, const string & ip) noexcept;
		public:
			/**
			 * @brief Метод добавления IP-адреса в чёрный список
			 *
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для добавления в чёрный список
			 */
			void setToBlackList(const string & domain, const string & ip) noexcept;
			/**
			 * @brief Метод добавления IP-адреса в чёрный список
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для добавления в чёрный список
			 */
			void setToBlackList(const int32_t family, const string & domain, const string & ip) noexcept;
		public:
			/**
			 * @brief Метод проверки наличия IP-адреса в чёрном списке
			 *
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для проверки наличия в чёрном списке
			 * @return       результат проверки наличия IP-адреса в чёрном списке
			 */
			bool isInBlackList(const string & domain, const string & ip) const noexcept;
			/**
			 * @brief Метод проверки наличия IP-адреса в чёрном списке
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param domain доменное имя соответствующее IP-адресу
			 * @param ip     адрес для проверки наличия в чёрном списке
			 * @return       результат проверки наличия IP-адреса в чёрном списке
			 */
			bool isInBlackList(const int32_t family, const string & domain, const string & ip) const noexcept;
		public:
			/**
			 * @brief Метод получения данных сервера имён
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       запрошенный сервер имён
			 */
			string server(const int32_t family) noexcept;
		public:
			/**
			 * @brief Метод добавления сервера DNS
			 *
			 * @param server адрес DNS-сервера
			 */
			void server(const string & server) noexcept;
			/**
			 * @brief Метод добавления сервера DNS
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param server адрес DNS-сервера
			 */
			void server(const int32_t family, const string & server) noexcept;
		public:
			/**
			 * @brief Метод добавления серверов DNS
			 *
			 * @param servers адреса DNS-серверов
			 */
			void servers(const vector <string> & servers) noexcept;
			/**
			 * @brief Метод добавления серверов DNS
			 *
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers адреса DNS-серверов
			 */
			void servers(const int32_t family, const vector <string> & servers) noexcept;
		public:
			/**
			 * @brief Метод замены существующих серверов DNS
			 *
			 * @param servers адреса DNS-серверов
			 */
			void replace(const vector <string> & servers = {}) noexcept;
			/**
			 * @brief Метод замены существующих серверов DNS
			 *
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers адреса DNS-серверов
			 */
			void replace(const int32_t family, const vector <string> & servers = {}) noexcept;
		public:
			/**
			 * @brief Метод установки адреса сетевых плат, с которых нужно выполнять запросы
			 *
			 * @param network IP-адреса сетевых плат
			 */
			void network(const vector <string> & network) noexcept;
			/**
			 * @brief Метод установки адреса сетевых плат, с которых нужно выполнять запросы
			 *
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param network IP-адреса сетевых плат
			 */
			void network(const int32_t family, const vector <string> & network) noexcept;
		public:
			/**
			 * @brief Метод установки префикса переменной окружения
			 *
			 * @param prefix префикс переменной окружения для установки
			 */
			void prefix(const string & prefix) noexcept;
		public:
			/**
			 * @brief Метод загрузки файла со списком хостов
			 *
			 * @param filename адрес файла для загрузки
			 */
			void hosts(const string & filename) noexcept;
		public:
			/**
			 * @brief Метод определение локального IP-адреса по имени домена
			 *
			 * @param name название сервера
			 * @return     полученный IP-адрес
			 */
			string host(const string & name) noexcept;
			/**
			 * @brief Метод определение локального IP-адреса по имени домена
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param name   название сервера
			 * @return       полученный IP-адрес
			 */
			string host(const int32_t family, const string & name) noexcept;
		public:
			/**
			 * @brief Метод ресолвинга домена
			 *
			 * @param host хост сервера
			 * @return     полученный IP-адрес
			 */
			string resolve(const string & host) noexcept;
			/**
			 * @brief Метод ресолвинга домена
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param host   хост сервера
			 * @return       полученный IP-адрес
			 */
			string resolve(const int32_t family, const string & host) noexcept;
		public:
			/**
			 * @brief Метод поиска доменного имени соответствующего IP-адресу
			 *
			 * @param ip адрес для поиска доменного имени
			 * @return   список найденных доменных имён
			 */
			vector <string> search(const string & ip) noexcept;
			/**
			 * @brief Метод поиска доменного имени соответствующего IP-адресу
			 *
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для поиска доменного имени
			 * @return       список найденных доменных имён
			 */
			vector <string> search(const int32_t family, const string & ip) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			DNS(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~DNS() noexcept;
	} dns_t;
};

#endif // __AWH_DNS_RESOLVER__
