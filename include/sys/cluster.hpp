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
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_CLUSTER__
#define __AWH_CLUSTER__

/**
 * Стандартные библиотеки
 */
#include <map>
#include <ctime>
#include <mutex>
#include <queue>
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
#include <sys/screen.hpp>
#include <sys/buffer.hpp>
#include <net/socket.hpp>
#include <sys/events.hpp>

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
		private:
			/**
			 * Стейт входящего собщения
			 */
			enum class state_t : uint8_t {
				NONE = 0x00, // Стейт не установлен
				HEAD = 0x01, // Заголовок входящего сообщения
				DATA = 0x02  // Данные входящего сообщения
			};
		private:
			/**
			 * Message Структура межпроцессного сообщения
			 */
			typedef struct Message {
				bool quit;   // Флаг остановки работы процесса
				pid_t pid;   // Пид активного процесса
				size_t size; // Размер передаваемых данных
				/**
				 * Message Конструктор
				 */
				Message() noexcept : quit(false), pid(0), size(0) {}
			} __attribute__((packed)) mess_t;
			/**
			 * Payload Структура полезной нагрузки
			 */
			typedef struct Payload {
				SOCKET fd;                 // Файловый дескриптор для отправки сообщения
				size_t pos;                // Позиция в буфере
				size_t size;               // Размер буфера
				size_t offset;             // Смещение в бинарном буфере
				unique_ptr <char []> data; // Данные буфера
				/**
				 * Payload Конструктор
				 */
				Payload() noexcept : fd(-1), pos(0), size(0), offset(0), data(nullptr) {}
			} payload_t;
		public:
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
					/**
					 * Payload Структура буфера полезной нагрузки
					 */
					typedef struct Payload {
						// Стейт входящего сообщения
						state_t state;
						// Параметры входящего сообщения
						mess_t message;
						// Буфер полезной нагрузки
						buffer_t buffer;
						/**
						 * Payload Конструктор
						 */
						Payload() noexcept : state(state_t::HEAD), buffer(buffer_t::mode_t::COPY) {}
					} payload_t;
				public:
					// Мютекс для блокировки потока
					mutex mtx;
				public:
					// Идентификатор воркера
					uint16_t wid;
				public:
					// Флаг асинхронного режима обмена сообщениями
					bool async;
					// Флаг запуска работы
					bool working;
					// Флаг автоматического перезапуска
					bool restart;
				public:
					// Количество рабочих процессов
					uint16_t count;
				private:
					// Бинарный буфер полученных данных
					uint8_t _buffer[4096];
				public:
					// Родительский объект кластера
					Cluster * cluster;
				private:
					// Объект для работы с скрином
					screen_t <data_t> _screen;
				private:
					// Полезная нагрузка полученная в сообщениях
					map <pid_t, unique_ptr <payload_t>> _payload;
				private:
					// Объект для работы с логами
					const log_t * _log;
				public:
					/**
					 * Если операционной системой не является Windows
					 */
					#if !defined(_WIN32) && !defined(_WIN64)
						/**
						 * process Метод перезапуска упавшего процесса
						 * @param pid    идентификатор упавшего процесса
						 * @param status статус остановившегося процесса
						 */
						void process(const pid_t pid, const int status) noexcept;
						/**
						 * child Функция фильтр перехватчика сигналов
						 * @param signal номер сигнала полученного системой
						 * @param info   объект информации полученный системой
						 * @param ctx    передаваемый внутренний контекст
						 */
						static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
						/**
						 * message Функция обратного вызова получении сообщений
						 * @param fd    файловый дескриптор (сокет)
						 * @param event произошедшее событие
						 */
						void message(const SOCKET fd, const base_t::event_type_t event) noexcept;
					#endif
				private:
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
						 * prepare Метод извлечения данных из полученного буфера
						 * @param pid    идентификатор упавшего процесса
						 * @param family идентификатор семейства кластера
						 */
						void prepare(const pid_t pid, const family_t family) noexcept;
					#endif
				public:
					/**
					 * Worker Конструктор
					 * @param log объект для работы с логами
					 */
					Worker(const log_t * log) noexcept;
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept;
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
					bool end;          // Флаг завершения работы процессом
					pid_t pid;         // Пид активного процесса
					int mfds[2];       // Список файловых дескрипторов родительского процесса
					int cfds[2];       // Список файловых дескрипторов дочернего процесса
					time_t date;       // Время начала жизни процесса
					awh::event_t mess; // Объект события на получения сообщений
					awh::event_t send; // Объект события на отправку сообщений
					/**
					 * Broker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const fmk_t * fmk, const log_t * log) noexcept :
					 end(false), pid(::getpid()),
					 mfds{INVALID_SOCKET,INVALID_SOCKET},
					 cfds{INVALID_SOCKET,INVALID_SOCKET}, date(0),
					 mess(awh::event_t::type_t::EVENT, fmk, log),
					 send(awh::event_t::type_t::EVENT, fmk, log) {}
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
					pid_t pid;   // Пид активного процесса
					time_t date; // Время начала жизни процесса
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
			// Флаг отслеживания падения дочерних процессов
			bool _trackCrash;
		private:
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Объект перехвата сигнала
			struct sigaction _sa;
		private:
			// Список активных дочерних процессов
			map <pid_t, uint16_t> _pids;
			// Буферы отправляемой полезной нагрузки
			map <uint16_t, queue <payload_t>> _payloads;
			// Список активных воркеров
			map <uint16_t, unique_ptr <worker_t>> _workers;
			// Список дочерних брокеров
			map <uint16_t, vector <unique_ptr <broker_t>>> _brokers;
		private:
			// Объект работы с базой событий
			base_t * _base;
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
		private:
			/**
			 * send Метод асинхронной отправки буфера данных в сокет
			 * @param wid    идентификатор воркера
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param fd     идентификатор файлового дескриптора
			 */
			void send(const uint16_t wid, const char * buffer, const size_t size, const SOCKET fd) noexcept;
		private:
			/**
			 * emplace Метод добавления нового буфера полезной нагрузки
			 * @param wid    идентификатор воркера
			 * @param buffer бинарный буфер полезной нагрузки
			 * @param size   размер бинарного буфера полезной нагрузки
			 * @param fd     идентификатор файлового дескриптора
			 */
			void emplace(const uint16_t wid, const char * buffer, const size_t size, const SOCKET fd) noexcept;
		private:
			/**
			 * write Метод записи буфера данных в сокет
			 * @param wid   идентификатор воркера
			 * @param pid   идентификатор процесса для получения сообщения
			 * @param fd    идентификатор файлового дескриптора
			 * @param event возникшее событие
			 */
			void write(const uint16_t wid, const pid_t pid, const SOCKET fd, const base_t::event_type_t event) noexcept;
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
			void base(base_t * base) noexcept;
		public:
			/**
			 * trackCrash Метод отключения отслеживания падения дочерних процессов
			 * @param mode флаг отслеживания падения дочерних процессов
			 */
			void trackCrash(const bool mode) noexcept;
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
			 * asyncMessages Метод установки флага асинхронного режима обмена сообщениями
			 * @param wid  идентификатор воркера
			 * @param mode флаг асинхронного режима обмена сообщениями
			 */
			void asyncMessages(const uint16_t wid, const bool mode) noexcept;
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
			Cluster(const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _trackCrash(true), _callbacks(log),
			 _socket(fmk, log), _base(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * Cluster Конструктор
			 * @param base база событий
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Cluster(base_t * base, const fmk_t * fmk, const log_t * log) noexcept :
			 _pid(getpid()), _trackCrash(true), _callbacks(log),
			 _socket(fmk, log), _base(base), _fmk(fmk), _log(log) {}
			/**
			 * ~Cluster Деструктор
			 */
			~Cluster() noexcept {}
	} cluster_t;
};

#endif // __AWH_CLUSTER__
