/**
 * @file: cluster.hpp
 * @date: 2023-07-01
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

#ifndef __AWH_CORE_CLUSTER__
#define __AWH_CORE_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <set>

/**
 * Модули AWH
 */
#include <core/core.hpp>

/**
 * Наши модули
 */
#include <sys/cluster.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * cluster пространство имён
	 */
	namespace cluster {
		/**
		 * Core Класс клиентского ядра биндинга TCP/IP
		 */
		typedef class AWHSHARED_EXPORT Core : public awh::core_t {
			public:
				/**
				 * Коды ошибок клиента
				 */
				enum class error_t : uint8_t {
					NONE      = 0x00, // Ошибка не установлена
					START     = 0x01, // Ошибка запуска приложения
					CLUSTER   = 0x02, // Ошибка работы кластера
					OS_BROKEN = 0x03  // Ошибка неподдерживаемой ОС
				};
			private:
				// Размер кластера
				uint16_t _size;
			private:
				// Объект кластера
				cluster_t _cluster;
			private:
				/**
				 * active Метод вывода статуса работы сетевого ядра
				 * @param status флаг запуска сетевого ядра
				 */
				void active(const status_t status) noexcept;
			private:
				/**
				 * rebase Метод события пересоздании процесса
				 * @param wid  идентификатор воркера
				 * @param pid  идентификатор процесса
				 * @param opid идентификатор старого процесса
				 */
				void rebase(const uint16_t wid, const pid_t pid, const pid_t opid) const noexcept;
			private:
				/**
				 * exit Метод события завершения работы процесса
				 * @param wid    идентификатор воркера
				 * @param pid    идентификатор процесса
				 * @param status статус остановки работы процесса
				 */
				void exit(const uint16_t wid, const pid_t pid, const int32_t status) const noexcept;
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
				 * master Метод проверки является ли процесс родительским
				 * @return результат проверки
				 */
				bool master() const noexcept;
			public:
				/**
				 * pids Метод получения списка дочерних процессов
				 * @return список дочерних процессов
				 */
				set <pid_t> pids() const noexcept;
			public:
				/**
				 * emplace Метод размещения нового воркера
				 */
				void emplace() noexcept;
				/**
				 * erase Метод удаления активного процесса
				 * @param pid идентификатор процесса
				 */
				void erase(const pid_t pid) noexcept;
			public:
				/**
				 * send Метод отправки сообщение родительскому процессу
				 */
				void send() const noexcept;
				/**
				 * send Метод отправки сообщение родительскому процессу
				 * @param buffer буфер бинарных данных
				 * @param size   размер буфера бинарных данных
				 */
				void send(const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * send Метод отправки сообщение процессу
				 * @param pid идентификатор процесса для отправки
				 */
				void send(const pid_t pid) const noexcept;
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
				 */
				void broadcast() const noexcept;
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
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
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
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Core(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Core Деструктор
				 */
				~Core() noexcept {}
		} core_t;
	};
};

#endif // __AWH_CORE_CLUSTER__
