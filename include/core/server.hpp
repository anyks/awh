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
			private:
				// Мютекс для блокировки основного потока
				mtx_t _mtx;
			private:
				// Идентификатор активного дочернего прцоесса
				size_t _pid;
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
				set <size_t> _locking;
			private:
				/**
				 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 * @param sid   идентификатор схемы сети
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void cluster(const size_t sid, const pid_t pid, const cluster_t::event_t event) noexcept;
			private:
				/**
				 * accept Функция подключения к серверу
				 * @param fd  файловый дескриптор (сокет) подключившегося клиента
				 * @param sid идентификатор схемы сети
				 */
				void accept(const int fd, const size_t sid) noexcept;
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
				void run(const size_t sid) noexcept;
				/**
				 * remove Метод удаления схемы сети
				 * @param sid идентификатор схемы сети
				 */
				void remove(const size_t sid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			private:
				/**
				 * timeout Функция обратного вызова при срабатывании таймаута
				 * @param aid идентификатор адъютанта
				 */
				void timeout(const size_t aid) noexcept;
				/**
				 * write Функция обратного вызова при записи данных в сокет
				 * @param method метод режима работы
				 * @param aid    идентификатор адъютанта
				 */
				void transfer(const engine_t::method_t method, const size_t aid) noexcept;
				/**
				 * resolving Метод получения IP адреса доменного имени
				 * @param sid    идентификатор схемы сети
				 * @param ip     адрес интернет-подключения
				 * @param family тип интернет-протокола AF_INET, AF_INET6
				 * @param did    идентификатор DNS запроса
				 */
				void resolving(const size_t sid, const string & ip, const int family, const size_t did) noexcept;
			public:
				/**
				 * bandWidth Метод установки пропускной способности сети
				 * @param aid   идентификатор адъютанта
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandWidth(const size_t aid, const string & read, const string & write) noexcept;
			public:
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const size_t size = 0) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param sid  идентификатор схемы сети
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const size_t sid, const bool mode) noexcept;
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
				void total(const size_t sid, const u_short total) noexcept;
				/**
				 * init Метод инициализации сервера
				 * @param sid  идентификатор схемы сети
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const size_t sid, const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * Core Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept;
		} core_t;
	};
};

#endif // __AWH_CORE_SERVER__
