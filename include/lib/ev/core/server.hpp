/**
 * @file: server.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <set>
#include <vector>

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <sys/cluster.hpp>
#include <scheme/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			private:
				/**
				 * Scheme Устанавливаем дружбу с схемой сети
				 */
				friend class Scheme;
			private:
				/**
				 * Mutex Объект основных мютексов
				 */
				typedef struct Mutex {
					recursive_mutex main;   // Для установки системных параметров
					recursive_mutex close;  // Для закрытия подключения
					recursive_mutex accept; // Для одобрения подключения
				} mtx_t;
			public:
				/**
				 * Host Параметры хоста сервера
				 */
				typedef struct Host {
					u_int port;  // Порт сервера
					string addr; // Адрес хоста сервера
					string sock; // Адрес unix-сокета
					/**
					 * Host Конструктор
					 */
					Host() noexcept : port(0), addr{""}, sock{""} {}
				} host_t;
			private:
				/**
				 * DTLS Класс проверки подключения для протокола UDP TLS
				 */
				typedef class DTLS {
					public:
						uint64_t bid;    // Идентификатор брокера
						Core * core;     // Объект ядра клиента
						ev::timer timer; // Объект события таймера
					public:
						/**
						 * callback Метод обратного вызова
						 * @param timer   объект события таймера
						 * @param revents идентификатор события
						 */
						void callback(ev::timer & timer, int revents) noexcept;
					public:
						/**
						 * DTLS Конструктор
						 */
						DTLS() noexcept : bid(0), core(nullptr) {}
				} dtls_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Идентификатор активного дочернего прцоесса
				pid_t _pid;
				// Параметры хоста сервера
				host_t _host;
				// Объект кластера
				cluster_t _cluster;
			private:
				// Флаг работы в режиме только IPv6
				bool _ipV6only;
			private:
				// Размер кластера
				uint16_t _clusterSize;
				// Флаг автоматического перезапуска упавших процессов
				bool _clusterAutoRestart;
			private:
				// Список блокированных объектов
				set <uint64_t> _locking;
				// Список серверов DTLS
				map <uint64_t, unique_ptr <dtls_t>> _dtls;
			private:
				/**
				 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 * @param sid   идентификатор схемы сети
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void cluster(const uint16_t sid, const pid_t pid, const cluster_t::event_t event) noexcept;
			private:
				/**
				 * accept Метод вызова при подключении к серверу
				 * @param fd  файловый дескриптор (сокет) подключившегося клиента
				 * @param sid идентификатор схемы сети
				 */
				void accept(const int fd, const uint16_t sid) noexcept;
			public:
				/**
				 * host Метод получения хоста сервера
				 * @return хост сервера
				 */
				const host_t & host() const noexcept;
			public:
				/**
				 * close Метод отключения всех брокеров
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех активных схем сети
				 */
				void remove() noexcept;
			public:
				/**
				 * run Метод запуска сервера
				 * @param sid идентификатор схемы сети
				 */
				void run(const uint16_t sid) noexcept;
				/**
				 * remove Метод удаления схемы сети
				 * @param sid идентификатор схемы сети
				 */
				void remove(const uint16_t sid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * read Метод чтения данных для брокера
				 * @param bid идентификатор брокера
				 */
				void read(const uint64_t bid) noexcept;
				/**
				 * write Метод записи буфера данных в сокет
				 * @param buffer буфер для записи данных
				 * @param size   размер записываемых данных
				 * @param bid    идентификатор брокера
				 */
				void write(const char * buffer, const size_t size, const uint64_t bid) noexcept;
			private:
				/**
				 * timeout Метод вызова при срабатывании таймаута
				 * @param bid идентификатор брокера
				 */
				void timeout(const uint64_t bid) noexcept;
			private:
				/**
				 * activation Метод активации параметров запуска сервера
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void activation(const uint16_t sid, const string & ip, const int family) noexcept;
			public:
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const uint64_t bid, const string & read, const string & write) noexcept;
			public:
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const uint16_t size = 0) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param sid  идентификатор схемы сети
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const uint16_t sid, const bool mode) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param sid   идентификатор схемы сети
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t sid, const u_short total) noexcept;
				/**
				 * init Метод инициализации сервера
				 * @param sid  идентификатор схемы сети
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint16_t sid, const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова при краше приложения
				 * @param callback функция обратного вызова для установки
				 */
				void on(function <void (const int)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова при запуске/остановки работы модуля
				 * @param callback функция обратного вызова для установки
				 */
				void on(function <void (const status_t, awh::core_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const log_t::flag_t, const error_t, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие запуска и остановки процессов кластера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const cluster_t::family_t, const uint16_t, const pid_t, const cluster_t::event_t, awh::core_t *)> callback) noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * Core Конструктор
				 * @param dns    объект DNS-резолвера
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				Core(const dns_t * dns, const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
