/**
 * @file: cluster.hpp
 * @date: 2024-07-14
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

#ifndef __AWH_CLUSTER__
#define __AWH_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <string>
#include <cstring>
#include <csignal>
#include <functional>
#include <sys/types.h>

/**
 * Если операционной системой не является Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Стандартные библиотеки
	 */
	#include <sys/wait.h>
#endif

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/cmp.hpp>
#include <net/socket.hpp>
#include <core/core.hpp>
#include <events/evbase.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Cluster Класс работы с кластером
	 */
	typedef class AWHSHARED_EXPORT Cluster {
		private:
			// Максимальный размер отправляемой полезной нагрузки
			static constexpr size_t MAX_PAYLOAD = 0xFF2;
			// Максимальный размер одного сообщения
			static constexpr size_t MAX_MESSAGE = 0x3B9ACA00;
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
		public:
			/**
			 * Worker Класс воркера
			 */
			typedef class AWHSHARED_EXPORT Worker {
				private:
					// Устанавливаем дружбу с родительским классом
					friend class Cluster;
				private:
					// Флаг запуска работы
					bool _working;
					// Флаг автоматического перезапуска
					bool _restart;
				private:
					// Идентификатор воркера
					uint16_t _wid;
				private:
					// Количество рабочих процессов
					uint16_t _count;
				private:
					// Бинарный буфер полученных данных
					uint8_t _buffer[4096];
				private:
					// Список объектов работы с протоколом кластера
					map <pid_t, unique_ptr <cmp::decoder_t>> _cmp;
				private:
					// Объект для работы с логами
					const log_t * _log;
					// Родительский объект кластера
					const Cluster * _ctx;
				public:
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						/**
						 * message Функция обратного вызова получении сообщений
						 * @param fd    файловый дескриптор (сокет)
						 * @param event произошедшее событие
						 */
						void message(const SOCKET fd, const base_t::event_type_t event) noexcept;
					#endif
				public:
					/**
					 * Worker Конструктор
					 * @param wid идентификатор воркера
					 * @param ctx родительский объект кластера
					 * @param log объект для работы с логами
					 */
					Worker(const uint16_t wid, const Cluster * ctx, const log_t * log) noexcept :
					 _working(false), _restart(false), _wid(wid), _count(1), _buffer{0}, _log(log), _ctx(ctx) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept {}
			} worker_t;
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * Broker Структура брокера
				 */
				typedef struct Broker {
					bool end;        // Флаг завершения работы процессом
					pid_t pid;       // Идентификатор активного процесса
					uint64_t date;   // Время начала жизни процесса
					SOCKET mfds[2];  // Список файловых дескрипторов родительского процесса
					SOCKET cfds[2];  // Список файловых дескрипторов дочернего процесса
					awh::event_t ev; // Объект события на получения сообщений
					/**
					 * Broker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const fmk_t * fmk, const log_t * log) noexcept :
					 end(false), pid(::getpid()), date(0),
					 mfds{INVALID_SOCKET, INVALID_SOCKET},
					 cfds{INVALID_SOCKET, INVALID_SOCKET},
					 ev(awh::event_t::type_t::EVENT, fmk, log) {}
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept {}
				} broker_t;
			/**
			 * Если операционной системой является Windows
			 */
			#else
				/**
				 * Broker Структура брокера
				 */
				typedef struct Broker {
					pid_t pid;     // Идентификатор активного процесса
					uint64_t date; // Время начала жизни процесса
					/**
					 * Broker Конструктор
					 */
					Broker() noexcept : pid(0), date(0) {}
				} broker_t;
			#endif
		private:
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Объект перехвата сигнала
				struct sigaction _sa;
			#endif
		private:
			// Список активных дочерних процессов
			map <pid_t, uint16_t> _pids;
			// Список активных воркеров
			map <uint16_t, unique_ptr <worker_t>> _workers;
			// Список объектов работы с протоколом кластера
			map <uint16_t, unique_ptr <cmp::encoder_t>> _cmp;
			// Список дочерних брокеров
			map <uint16_t, vector <unique_ptr <broker_t>>> _brokers;
		private:
			// Объект сетевого ядра
			core_t * _core;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * process Метод перезапуска упавшего процесса
				 * @param pid    идентификатор упавшего процесса
				 * @param status статус остановившегося процесса
				 */
				void process(const pid_t pid, const int32_t status) noexcept;
				/**
				 * child Функция фильтр перехватчика сигналов
				 * @param signal номер сигнала полученного системой
				 * @param info   объект информации полученный системой
				 * @param ctx    передаваемый внутренний контекст
				 */
				static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
			#endif
		private:
			/**
			 * write Метод записи буфера данных в сокет
			 * @param wid идентификатор воркера
			 * @param fd  идентификатор файлового дескриптора
			 */
			void write(const uint16_t wid, const SOCKET fd) noexcept;
		private:
			/**
			 * emplace Метод размещения нового дочернего процесса
			 * @param wid идентификатор воркера
			 * @param pid идентификатор предыдущего процесса
			 */
			void emplace(const uint16_t wid, const pid_t pid) noexcept;
			/**
			 * create Метод создания дочерних процессов при запуске кластера
			 * @param wid   идентификатор воркера
			 * @param index индекс инициализированного процесса
			 */
			void create(const uint16_t wid, const uint16_t index = 0) noexcept;
		public:
			/**
			 * master Метод проверки является ли процесс родительским
			 * @return результат проверки
			 */
			bool master() const noexcept;
		public:
			/**
			 * working Метод проверки на запуск работы кластера
			 * @param wid идентификатор воркера
			 * @return    результат работы проверки
			 */
			bool working(const uint16_t wid) const noexcept;
		public:
			/**
			 * pids Метод получения списка дочерних процессов
			 * @param wid идентификатор воркера
			 * @return    список дочерних процессов
			 */
			set <pid_t> pids(const uint16_t wid) const noexcept;
		public:
			/**
			 * send Метод отправки сообщения родительскому процессу
			 * @param wid идентификатор воркера
			 */
			void send(const uint16_t wid) noexcept;
			/**
			 * send Метод отправки сообщения родительскому процессу
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер для отправки сообщения
			 * @param size   размер бинарного буфера для отправки сообщения
			 */
			void send(const uint16_t wid, const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * send Метод отправки сообщения дочернему процессу
			 * @param wid идентификатор воркера
			 * @param pid идентификатор процесса для получения сообщения
			 */
			void send(const uint16_t wid, const pid_t pid) noexcept;
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
			 * @param wid идентификатор воркера
			 */
			void broadcast(const uint16_t wid) noexcept;
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
			 * base Метод установки сетевого ядра
			 * @param core сетевое ядро для установки
			 */
			void core(core_t * core) noexcept;
		public:
			/**
			 * emplace Метод размещения нового дочернего процесса
			 * @param wid идентификатор воркера
			 */
			void emplace(const uint16_t wid) noexcept;
			/**
			 * erase Метод удаления активного процесса
			 * @param wid идентификатор воркера
			 * @param pid идентификатор процесса
			 */
			void erase(const uint16_t wid, const pid_t pid) noexcept;
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
			 * callbacks Метод установки функций обратного вызова
			 * @param callbacks функции обратного вызова
			 */
			void callbacks(const fn_t & callbacks) noexcept;
		public:
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @param fn  функция обратного вызова для установки
			 */
			void callback(const uint64_t idw, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if((idw > 0) && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (idw, fn);
			}
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова для установки
			 */
			void callback(const string & name, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if(!name.empty() && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (name, fn);
			}
		public:
			/**
			 * Cluster Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Cluster(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * Cluster Конструктор
			 * @param core объект сетевого ядра
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Cluster Деструктор
			 */
			~Cluster() noexcept;
	} cluster_t;
};

#endif // __AWH_CLUSTER__
