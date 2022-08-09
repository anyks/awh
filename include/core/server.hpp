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
 * Стандартная библиотека
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
					SELECT     = 0x01, // Флаг выбора работника
					UNSELECT   = 0x02, // Флаг снятия выбора с работника
					CONNECT    = 0x03, // Флаг подключения
					DISCONNECT = 0x04  // Флаг отключения
				};
			private:
				/**
				 * Message Структура межпроцессного сообщения
				 */
				typedef struct Data {
					bool fin;            // Сообщение является финальным
					size_t count;        // Количество подключений
					event_t event;       // Активное событие
					u_char buffer[4079]; // Буфер полезной нагрузки
					/**
					 * Data Конструктор
					 */
					Data() noexcept : fin(true), count(0), event(event_t::NONE) {}
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
				// Индекс работника в списке
				size_t _index;
				// Активное событие
				// event_t _event;
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
				// Нагрузка на дочерние процессы
				map <int16_t, size_t> burden; // ++++++++++++++++++= Реализовать поддержку IDW и очистку объекта, при остановки процесса
			private:
				/**
				 * message Метод получения сообщения от родительского или дочернего процесса
				 * @param wid  идентификатор воркера
				 * @param mess объект полученного сообщения
				 */
				void message(const size_t wid, const cluster_t::mess_t & mess) noexcept;
				/**
				 * cluster Метод события ЗАПУСКА/ОСТАНОВКИ кластера
				 * @param wid   идентификатор воркера
				 * @param event идентификатор события
				 * @param index индекс процесса
				 * @param pid   идентификатор процесса
				 */
				void cluster(const size_t wid, const cluster_t::event_t event, const int16_t index) noexcept;
			private:
				/**
				 * sendToProccess Метод отправки сообщения дочернему процессу
				 * @param wid   идентификатор воркера
				 * @param index индекс процесса для получения сообщения
				 * @param event активное событие на сервере
				 */
				void sendToProccess(const size_t wid, const int16_t index, const event_t event) noexcept;
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
