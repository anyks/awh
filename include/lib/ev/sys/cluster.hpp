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
#include <ctime>
#include <mutex>
#include <vector>
#include <thread>
#include <string>
#include <cstring>
#include <csignal>
#include <functional>
#include <sys/types.h>
#include <libev/ev++.h>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/child.hpp>

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
			 * События работы кластера
			 */
			enum class event_t : uint8_t {
				NONE  = 0x00, // Событие не установлено
				STOP  = 0x02, // Событие остановки процесса
				START = 0x01  // Событие запуска процесса
			};
			/**
			 * Семейство кластера
			 */
			enum class family_t : uint8_t {
				NONE     = 0x00, // Воркер не установлено
				MASTER   = 0x02, // Воркер является мастером
				CHILDREN = 0x01  // Воркер является ребёнком
			};
		private:
			/**
			 * Worker Класс воркера
			 */
			typedef class Worker {
				private:
					/**
					 * Data Структура выводимых данных
					 */
					typedef struct Data {
						pid_t pid;            // Идентификатор процесса приславший данные
						vector <char> buffer; // Буфер передаваемых данных
						/**
						 * Data Конструктор
						 */
						Data() noexcept : pid(0) {}
					} data_t;
				public:
					mutex mtx;         // Мютекс для блокировки потока
					uint16_t wid;      // Идентификатор воркера
					bool async;        // Флаг асинхронного режима работы
					bool working;      // Флаг запуска работы
					bool restart;      // Флаг автоматического перезапуска
					uint16_t count;    // Количество рабочих процессов
					Cluster * cluster; // Родительский объект кластера
				private:
					// Общий буфер входящих данных
					vector <char> _buffer;
				private:
					// Объект для работы с дочерним потоком
					child_t <data_t> * _child;
				private:
					// Объект для работы с логами
					const log_t * _log;
				public:
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						/**
						 * callback Метод вывода функции обратного вызова
						 * @param data данные передаваемые процессом
						 */
						void callback(const data_t & data) noexcept;
						/**
						 * message Функция обратного вызова получении сообщений
						 * @param watcher объект события чтения
						 * @param revents идентификатор события
						 */
						void message(ev::io & watcher, int revents) noexcept;
						/**
						 * child Функция обратного вызова при завершении работы процесса
						 * @param watcher объект события дочернего процесса
						 * @param revents идентификатор события
						 */
						void child(ev::child & watcher, int revents) noexcept;
						/**
						 * process Метод перезапуска упавшего процесса
						 * @param pid    идентификатор упавшего процесса
						 * @param status статус остановившегося процесса
						 */
						void process(const pid_t pid, const int status) noexcept;
					#endif
				public:
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						/**
						 * stopChild Метод остановки работы дочернего процесса
						 */
						void stopChild() noexcept;
						/**
						 * startChild Метод запуска работы дочернего процесса
						 */
						void startChild() noexcept;
					#endif
				public:
					/**
					 * Worker Конструктор
					 * @param log объект для работы с логами
					 */
					Worker(const log_t * log) noexcept :
					 wid(0), async(false), working(false), restart(false),
					 count(1), cluster(nullptr), _child(nullptr), _log(log) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept;
			} worker_t;
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * Jack Структура работника
				 */
				typedef struct Jack {
					bool end;     // Флаг завершения работы процессом
					pid_t pid;    // Пид активного процесса
					int mfds[2];  // Список файловых дескрипторов родительского процесса
					int cfds[2];  // Список файловых дескрипторов дочернего процесса
					time_t date;  // Время начала жизни процесса
					ev::io mess;  // Объект события на получения сообщений
					ev::child cw; // Объект работы с дочерними процессами
					/**
					 * Jack Конструктор
					 */
					Jack() noexcept : end(false), pid(0), mfds{0,0}, cfds{0,0}, date(0) {}
				} jack_t;
				/**
				 * Message Структура межпроцессного сообщения
				 */
				typedef struct Message {
					bool end;             // Флаг получения последнего чанка
					bool quit;            // Флаг остановки работы процесса
					pid_t pid;            // Пид активного процесса
					size_t size;          // Размер передаваемых данных
					u_char payload[4082]; // Буфер полезной нагрузки
					/**
					 * Message Конструктор
					 */
					Message() noexcept : end(false), quit(false), pid(0), size(0), payload{0} {}
				} __attribute__((packed)) mess_t;
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
			// Флаг отслеживания падения дочерних процессов
			bool _trackCrash;
		private:
			// Хранилище функций обратного вызова
			fn_t _callback;
		private:
			// Список активных дочерних процессов
			map <pid_t, uint16_t> _pids;
			// Список активных воркеров
			map <uint16_t, unique_ptr <worker_t>> _workers;
			// Список дочерних работников
			map <uint16_t, vector <unique_ptr <jack_t>>> _jacks;
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
			void fork(const uint16_t wid, const uint16_t index = 0, const bool stop = false) noexcept;
		public:
			/**
			 * working Метод проверки на запуск работы кластера
			 * @param wid идентификатор воркера
			 * @return    результат работы проверки
			 */
			bool working(const uint16_t wid) const noexcept;
		public:
			/**
			 * send Метод отправки сообщения родительскому процессу
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void send(const uint16_t wid, const char * buffer, const size_t size) noexcept;
			/**
			 * send Метод отправки сообщения дочернему процессу
			 * @param wid    идентификатор воркера
			 * @param pid    идентификатор процесса для получения сообщения
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void send(const uint16_t wid, const pid_t pid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * broadcast Метод отправки сообщения всем дочерним процессам
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void broadcast(const uint16_t wid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * clear Метод очистки всех выделенных ресурсов
			 */
			void clear() noexcept;
		public:
			/**
			 * close Метод закрытия всех подключений
			 */
			void close() noexcept;
			/**
			 * close Метод закрытия всех подключений
			 * @param wid идентификатор воркера
			 */
			void close(const uint16_t wid) noexcept;
		public:
			/**
			 * stop Метод остановки кластера
			 * @param wid идентификатор воркера
			 */
			void stop(const uint16_t wid) noexcept;
			/**
			 * start Метод запуска кластера
			 * @param wid идентификатор воркера
			 */
			void start(const uint16_t wid) noexcept;
		public:
			/**
			 * restart Метод установки флага перезапуска процессов
			 * @param wid  идентификатор воркера
			 * @param mode флаг перезапуска процессов
			 */
			void restart(const uint16_t wid, const bool mode) noexcept;
		public:
			/**
			 * base Метод установки базы событий
			 * @param base база событий для установки
			 */
			void base(struct ev_loop * base) noexcept;
		public:
			/**
			 * trackCrash Метод отключения отслеживания падения дочерних процессов
			 * @param mode флаг отслеживания падения дочерних процессов
			 */
			void trackCrash(const bool mode) noexcept;
		public:
			/**
			 * async Метод установки флага асинхронного режима работы
			 * @param wid  идентификатор воркера
			 * @param mode флаг асинхронного режима работы
			 */
			void async(const uint16_t wid, const bool mode) noexcept;
		public:
			/**
			 * count Метод получения максимального количества процессов
			 * @param wid идентификатор воркера
			 * @return    максимальное количество процессов
			 */
			uint16_t count(const uint16_t wid) const noexcept;
			/**
			 * count Метод установки максимального количества процессов
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void count(const uint16_t wid, const uint16_t count) noexcept;
		public:
			/**
			 * init Метод инициализации воркера
			 * @param wid   идентификатор воркера
			 * @param count максимальное количество процессов
			 */
			void init(const uint16_t wid, const uint16_t count = 1) noexcept;
		public:
			/**
			 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint16_t, const pid_t, const event_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при получении сообщения
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint16_t, const pid_t, const char *, const size_t)> callback) noexcept;
		public:
			/**
			 * Cluster Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Cluster(const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _trackCrash(true), _callback(log), _base(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * Cluster Конструктор
			 * @param base база событий
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(struct ev_loop * base, const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _trackCrash(true), _callback(log), _base(base), _fmk(fmk), _log(log) {}
	} cluster_t;
};

#endif // __AWH_CLUSTER__
