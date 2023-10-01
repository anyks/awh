/**
 * @file: server.hpp
 * @date: 2022-09-08
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
				/**
				 * DTLS Класс проверки подключения для протокола UDP TLS
				 */
				typedef class DTLS {
					public:
						uint64_t aid;  // Идентификатор адъютанта
						Core * core;   // Объект ядра клиента
						event_t event; // Объект события таймера
					public:
						/**
						 * callback Метод обратного вызова
						 * @param fd    файловый дескриптор (сокет)
						 * @param event произошедшее событие
						 */
						void callback(const evutil_socket_t fd, const short event) noexcept;
					public:
						/**
						 * DTLS Конструктор
						 * @param log объект для работы с логами
						 */
						DTLS(const log_t * log) noexcept : aid(0), core(nullptr), event(event_t::type_t::TIMER, log) {}
						/**
						 * ~DTLS Деструктор
						 */
						~DTLS() noexcept;
				} dtls_t;
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Идентификатор активного дочернего прцоесса
				pid_t _pid;
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
				 * close Метод отключения всех адъютантов
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
				 * close Метод закрытия подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void close(const uint64_t aid) noexcept;
			private:
				/**
				 * timeout Метод вызова при срабатывании таймаута
				 * @param aid идентификатор адъютанта
				 */
				void timeout(const uint64_t aid) noexcept;
				/**
				 * transfer Метед передачи данных между клиентом и сервером
				 * @param method метод режима работы
				 * @param aid    идентификатор адъютанта
				 */
				void transfer(const engine_t::method_t method, const uint64_t aid) noexcept;
				/**
				 * resolving Метод получения IP адреса доменного имени
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 */
				void resolving(const uint16_t sid, const string & ip, const int family) noexcept;
			public:
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const uint64_t aid, const string & read, const string & write) noexcept;
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
				 * Core Конструктор
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				Core(const fmk_t * fmk, const log_t * log, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
