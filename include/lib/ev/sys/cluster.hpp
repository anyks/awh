/**
 * @file: cluster.hpp
 * @date: 2022-08-08
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

#ifndef __AWH_CLUSTER__
#define __AWH_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <vector>
#include <thread>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <libev/ev++.h>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Cluster Класс работы с кластером
	 */
	typedef class Cluster {
		public:
			/**
			 * События работы сервера
			 */
			enum class event_t : uint8_t {
				NONE  = 0x00, // Событие не установлено
				STOP  = 0x02, // Событие остановки процесса
				START = 0x01  // Событие запуска процесса
			};
		private:
			/**
			 * Worker Класс воркера
			 */
			typedef class Worker {
				public:
					size_t wid;        // Идентификатор воркера
					bool working;      // Флаг запуска работы
					bool restart;      // Флаг автоматического перезапуска
					uint16_t count;    // Количество рабочих процессов
					Cluster * cluster; // Родительский объект кластера
				public:
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						/**
						 * child Функция обратного вызова при завершении работы процесса
						 * @param watcher объект события дочернего процесса
						 * @param revents идентификатор события
						 */
						void child(ev::child & watcher, int revents) noexcept;
					#endif
				public:
					/**
					 * Worker Конструктор
					 */
					Worker() noexcept : wid(0), working(false), restart(false), count(1), cluster(nullptr) {}
			} worker_t;
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * Jack Структура работника
				 */
				typedef struct Jack {
					pid_t pid;    // Пид активного процесса
					ev::child cw; // Объект работы с дочерними процессами
					time_t date;  // Время начала жизни процесса
					/**
					 * Jack Конструктор
					 */
					Jack() noexcept : pid(0), date(0) {}
				} jack_t;
			/**
			 * Если операционной системой является Windows
			 */
			#else
				/**
				 * Jack Структура работника
				 */
				typedef struct Jack {
					pid_t pid;    // Пид активного процесса
					time_t date;  // Время начала жизни процесса
					/**
					 * Jack Конструктор
					 */
					Jack() noexcept : pid(0), date(0) {}
				} jack_t;
			#endif
		private:
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Список активных дочерних процессов
			map <pid_t, uint16_t> _pids;
			// Список активных воркеров
			map <size_t, worker_t> _workers;
			// Список дочерних работников
			map <size_t, vector <unique_ptr <jack_t>>> _jacks;
		private:
			// Функция обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
			function <void (const size_t, const pid_t, const event_t)> _fn;
		private:
			// Объект работы с базой событий
			struct ev_loop * _base;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * fork Метод отделения от основного процесса (создание дочерних процессов)
			 * @param wid   идентификатор воркера
			 * @param index индекс инициализированного процесса
			 * @param stop  флаг остановки итерации создания дочерних процессов
			 */
			void fork(const size_t wid, const uint16_t index = 0, const bool stop = false) noexcept;
		public:
			/**
			 * working Метод проверки на запуск работы кластера
			 * @param wid идентификатор воркера
			 * @return    результат работы проверки
			 */
			bool working(const size_t wid) const noexcept;
		public:
			/**
			 * clear Метод очистки всех выделенных ресурсов
			 */
			void clear() noexcept;
		public:
			/**
			 * stop Метод остановки кластера
			 * @param wid идентификатор воркера
			 */
			void stop(const size_t wid) noexcept;
			/**
			 * start Метод запуска кластера
			 * @param wid идентификатор воркера
			 */
			void start(const size_t wid) noexcept;
		public:
			/**
			 * restart Метод установки флага перезапуска процессов
			 * @param wid  идентификатор воркера
			 * @param mode флаг перезапуска процессов
			 */
			void restart(const size_t wid, const bool mode) noexcept;
		public:
			/**
			 * base Метод установки базы событий
			 * @param base база событий для установки
			 */
			void base(struct ev_loop * base) noexcept;
		public:
			/**
			 * count Метод получения максимального количества процессов
			 * @param wid идентификатор воркера
			 * @return    максимальное количество процессов
			 */
			uint16_t count(const size_t wid) const noexcept;
			/**
			 * count Метод установки максимального количества процессов
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void count(const size_t wid, const uint16_t count) noexcept;
		public:
			/**
			 * init Метод инициализации воркера
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void init(const size_t wid, const uint16_t count = 1) noexcept;
		public:
			/**
			 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const size_t, const pid_t, const event_t)> callback) noexcept;
		public:
			/**
			 * Cluster Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Cluster(const fmk_t * fmk, const log_t * log) noexcept : _pid(getpid()), _fmk(fmk), _log(log) {}
			/**
			 * Cluster Конструктор
			 * @param base база событий
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(struct ev_loop * base, const fmk_t * fmk, const log_t * log) noexcept : _pid(getpid()), _fn(nullptr), _base(base), _fmk(fmk), _log(log) {}
	} cluster_t;
};

#endif // __AWH_CLUSTER__
