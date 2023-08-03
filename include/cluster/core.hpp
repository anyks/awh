/**
 * @file: core.hpp
 * @date: 2023-07-01
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

#ifndef __AWH_CORE_CLUSTER__
#define __AWH_CORE_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <set>
#include <vector>

/**
 * Модули AWH
 */
#include <core/core.hpp>

/**
 * Наши модули
 */
#include <sys/cluster.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * cluster пространство имён
	 */
	namespace cluster {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class Core : public awh::core_t {
			public:
				/**
				 * Тип воркера
				 */
				enum class worker_t : uint8_t {
					NONE     = 0x00, // Воркер не установлено
					MASTER   = 0x02, // Воркер является мастером
					CHILDREN = 0x01  // Воркер является ребёнком
				};
			private:
				// Идентификатор активного дочернего прцоесса
				size_t _pid;
				// Объект кластера
				cluster_t _cluster;
			private:
				// Размер кластера
				uint16_t _clusterSize;
				// Флаг автоматического перезапуска упавших процессов
				bool _clusterAutoRestart;
			private:
				// Функция обратного вызова при запуске/остановке модуля
				function <void (const status_t, awh::core_t *)> _activeClusterFn;
			private:
				// Объект для работы с логами
				const log_t * _log;
			private:
				// Функция обратного вызова при получении события
				function <void (const worker_t, const pid_t, const cluster_t::event_t, Core *)> _eventsFn;
				// Функция обратного вызова при получении сообщения
				function <void (const worker_t, const pid_t, const char *, const size_t, Core *)> _messageFn;
			private:
				/**
				 * active Метод вывода статуса работы сетевого ядра
				 * @param status флаг запуска сетевого ядра
				 * @param core   объект сетевого ядра
				 */
				void active(const status_t status, awh::core_t * core) noexcept;
			private:
				/**
				 * cluster Метод информирования о статусе кластера
				 * @param wid   идентификатор воркера
				 * @param pid   идентификатор процесса
				 * @param event идентификатор события
				 */
				void cluster(const size_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept;
			private:
				/**
				 * message Метод получения сообщения
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void message(const size_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * isMaster Метод проверки является ли процесс дочерним
				 * @return результат проверки
				 */
				bool isMaster() const noexcept;
			public:
				/**
				 * send Метод отправки сообщение родительскому процессу
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void send(const char * buffer, const size_t size) const noexcept;
				/**
				 * send Метод отправки сообщение процессу
				 * @param pid    идентификатор процесса для отправки
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void send(const pid_t pid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * broadcast Метод отправки сообщения всем дочерним процессам
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 */
				void broadcast(const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
				/**
				 * close Метод закрытия всех подключений
				 */
				void close() noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова при получении события
				 * @param callback функция обратного вызова для установки
				 */
				void on(function <void (const worker_t, const pid_t, const cluster_t::event_t, Core *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова при получении сообщения
				 * @param callback функция обратного вызова для установки
				 */
				void on(function <void (const worker_t, const pid_t, const char *, const size_t, Core *)> callback) noexcept;
			public:
				/**
				 * clusterAsync Метод установки флага асинхронного режима работы
				 * @param wid  идентификатор воркера
				 * @param mode флаг асинхронного режима работы
				 */
				void clusterAsync(const bool mode) noexcept;
				/**
				 * clusterSize Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void clusterSize(const size_t size = 0) noexcept;
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
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

#endif // __AWH_CORE_CLUSTER__
