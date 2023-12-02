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
			private:
				// Идентификатор активного дочернего прцоесса
				pid_t _pid;
			private:
				// Размер кластера
				uint16_t _size;
			private:
				// Флаг автоматического перезапуска упавших процессов
				bool _autoRestart;
			private:
				// Объект кластера
				cluster_t _cluster;
			private:
				// Объект для работы с логами
				const log_t * _log;
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
				void cluster(const uint16_t wid, const pid_t pid, const cluster_t::event_t event) const noexcept;
			private:
				/**
				 * message Метод получения сообщения
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void message(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * master Метод проверки является ли процесс дочерним
				 * @return результат проверки
				 */
				bool master() const noexcept;
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
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const fn_t & callback) noexcept;
			public:
				/**
				 * async Метод установки флага асинхронного режима работы
				 * @param wid  идентификатор воркера
				 * @param mode флаг асинхронного режима работы
				 */
				void async(const bool mode) noexcept;
				/**
				 * size Метод установки количества процессов кластера
				 * @param size количество рабочих процессов
				 */
				void size(const uint16_t size = 0) noexcept;
				/**
				 * autoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void autoRestart(const bool mode) noexcept;
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
