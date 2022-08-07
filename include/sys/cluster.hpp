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
 * Стандартная библиотека
 */
#include <thread>
#include <string>
#include <unistd.h>
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
		public:
			/**
			 * Message Структура межпроцессного сообщения
			 */
			typedef struct Message {
				pid_t pid;            // Пид активного процесса
				size_t index;         // Индекс работника в списке
				u_char payload[4080]; // Буфер полезной нагрузки
				/**
				 * Message Конструктор
				 */
				Message() noexcept : pid(0), index(0) {}
			} __attribute__((packed)) mess_t;
		private:
			/**
			 * Jack Структура работника
			 */
			typedef struct Jack {
				pid_t pid;    // Пид активного процесса
				int mfds[2];  // Список файловых дескрипторов родительского процесса
				int cfds[2];  // Список файловых дескрипторов дочернего процесса
				ev::io read;  // Объект события на чтение
				ev::io write; // Объект события на запись
				ev::child cw; // Объект работы с дочерними процессами
				size_t wid;   // Идентификатор основного воркера
				time_t date;  // Время начала жизни процесса
				size_t index; // Индекс работника в списке
				size_t count; // Количество подключений
				/**
				 * Jack Конструктор
				 */
				Jack() noexcept : pid(0), wid(0), date(0), index(0), count(0) {}
			} jack_t;
			/**
			 * Callback Структура функций обратного вызова
			 */
			typedef struct Callback {
				// Функция обратного вызова при получении сообщения
				function <void (const mess_t &)> message;
				// Функция обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
				function <void (const event_t, const size_t, const pid_t)> process;
				/**
				 * Callback Конструктор
				 */
				Callback() noexcept : message(nullptr), process(nullptr) {}
			} fn_t;
		private:
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Индекс работника в списке
			uint16_t _index;
		private:
			// Максимальное количество процессов
			uint16_t _max;
			// Количество рабочих процессов
			uint16_t _count;
		private:
			// Объявляем функции обратного вызова
			fn_t callback;
		private:
			// Список дочерних работников
			vector <unique_ptr <jack_t>> _jacks;
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
			 * read Функция обратного вызова при чтении данных с сокета
			 * @param watcher объект события чтения
			 * @param revents идентификатор события
			 */
			void read(ev::io & watcher, int revents) noexcept;
			/**
			 * write Функция обратного вызова при записи данных в сокет
			 * @param watcher объект события записи
			 * @param revents идентификатор события
			 */
			void write(ev::io & watcher, int revents) noexcept;
		private:
			/**
			 * child Функция обратного вызова при завершении работы процесса
			 * @param watcher объект события дочернего процесса
			 * @param revents идентификатор события
			 */
			void child(ev::child & watcher, int revents) noexcept;
		private:
			/**
			 * fork Метод отделения от основного процесса (создание дочерних процессов)
			 * @param index индекс инициализированного процесса
			 * @param stop  флаг остановки итерации создания дочерних процессов
			 */
			void fork(const size_t index = 0, const size_t stop = false) noexcept;
		public:
			/**
			 * send Метод отправки сообщения родительскому объекту
			 * @param mess отправляемое сообщение
			 */
			void send(const mess_t & mess) noexcept;
			/**
			 * send Метод отправки сообщения родительскому объекту
			 * @param index индекс процесса для получения сообщения
			 * @param mess  отправляемое сообщение
			 */
			void send(const size_t index, const mess_t & mess) noexcept;
		public:
			/**
			 * stop Метод остановки кластера
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска кластера
			 */
			void start() noexcept;
		public:
			/**
			 * base Метод установки базы событий
			 * @param base база событий для установки
			 */
			void base(struct ev_loop * base) noexcept;
		public:
			/**
			 * count Метод установки максимального количества процессов
			 * @param count максимальное количество процессов
			 */
			void count(const uint16_t count) noexcept;
		public:
			/**
			 * onMessage Метод установки функции обратного вызова при получении сообщения
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const mess_t &)> callback) noexcept;
			/**
			 * onMessage Метод установки функции обратного вызова при ЗАПУСКЕ/ОСТАНОВКИ процесса
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const event_t, const size_t, const pid_t)> callback) noexcept;
		public:
			/**
			 * Cluster Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Cluster(const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _index(0), _max(1), _count(0), _fmk(fmk), _log(log) {}
			/**
			 * Cluster Конструктор
			 * @param base база событий
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(struct ev_loop * base, const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _index(0), _max(1), _count(0), _base(base), _fmk(fmk), _log(log) {}
	} cluster_t;
};

#endif // __AWH_CLUSTER__
