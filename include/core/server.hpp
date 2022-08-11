/**
 * @file: server.hpp
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
#include <worker/server.hpp>

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
				 * Worker Устанавливаем дружбу с классом сетевого ядра
				 */
				friend class Worker;
			private:
				/**
				 * События работы сервера
				 */
				enum class event_t : uint8_t {
					NONE       = 0x00, // Флаг не установлен
					ONLINE     = 0x01, // Флаг подключения дочернего процесса
					MESSAGE    = 0x02, // Флаг передачи внутреннего сообщения
					SELECT     = 0x03, // Флаг выбора работника
					UNSELECT   = 0x04, // Флаг снятия выбора с работника
					CONNECT    = 0x05, // Флаг подключения
					DISCONNECT = 0x06  // Флаг отключения
				};
			private:
				/**
				 * Message Структура межпроцессного сообщения
				 */
				typedef struct Data {
					bool fin;            // Сообщение является финальным
					size_t aid;          // Идентификатор адъютанта
					size_t size;         // Размер полезной нагрузки
					size_t count;        // Количество подключений
					event_t event;       // Активное событие
					u_char buffer[4063]; // Буфер полезной нагрузки
					/**
					 * Data Конструктор
					 */
					Data() noexcept : fin(true), aid(0), size(0), count(0), event(event_t::NONE) {}
				} __attribute__((packed)) data_t;
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
				// Флаг активации перехвата подключения
				bool _interception;
			private:
				// Размер кластера
				uint16_t _clusterSize;
			private:
				// Список блокированных объектов
				set <size_t> _locking;
			private:
				// Список адъютантов
				map <size_t, pid_t> _adjutants;
				// Нагрузка на дочерние процессы
				map <size_t, map <pid_t, size_t>> _burden;
				// Буферы сообщений дочерних процессов
				map <pid_t, map <size_t, vector <char>>> _messages;
			private:
				/**
				 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 * @param wid   идентификатор воркера
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void cluster(const size_t wid, const pid_t pid, const cluster_t::event_t event) noexcept;
				/**
				 * message Метод получения сообщения от родительского или дочернего процесса
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param buffer буфер получаемых данных
				 * @param size   размер получаемых данных
				 */
				void message(const size_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * sendEvent Метод отправки события родительскому и дочернему процессу
				 * @param wid   идентификатор воркера
				 * @param aid   идентификатор адъютанта
				 * @param pid   идентификатор процесса
				 * @param event активное событие на сервере
				 */
				void sendEvent(const size_t wid, const size_t aid, const pid_t pid, const event_t event) noexcept;
			public:
				/**
				 * sendMessage Метод отправки сообщения родительскому процессу
				 * @param wid    идентификатор воркера
				 * @param aid    идентификатор адъютанта
				 * @param pid    идентификатор процесса
				 * @param buffer буфер передаваемых данных
				 * @param size   размер передаваемых данных
				 */
				void sendMessage(const size_t wid, const size_t aid, const pid_t pid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * resolver Функция выполнения резолвинга домена
				 * @param ip  полученный IP адрес
				 * @param wrk объект воркера
				 */
				void resolver(const string & ip, worker_t * wrk) noexcept;
			private:
				/**
				 * accept Функция подключения к серверу
				 * @param fd  файловый дескриптор (сокет) подключившегося клиента
				 * @param wid идентификатор воркера
				 */
				void accept(const int fd, const size_t wid) noexcept;
			public:
				/**
				 * close Метод отключения всех воркеров
				 */
				void close() noexcept;
				/**
				 * remove Метод удаления всех воркеров
				 */
				void remove() noexcept;
			public:
				/**
				 * run Метод запуска сервера воркером
				 * @param wid идентификатор воркера
				 */
				void run(const size_t wid) noexcept;
				/**
				 * remove Метод удаления воркера
				 * @param wid идентификатор воркера
				 */
				void remove(const size_t wid) noexcept;
			public:
				/**
				 * close Метод закрытия подключения воркера
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
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const size_t size = 0) noexcept;
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param wid   идентификатор воркера
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const size_t wid, const u_short total) noexcept;
				/**
				 * init Метод инициализации сервера
				 * @param wid  идентификатор воркера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const size_t wid, const u_int port, const string & host = "") noexcept;
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
