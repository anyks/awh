/**
 * @file: evbase.hpp
 * @date: 2024-06-26
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

#ifndef __AWH_EVENT_BASE__
#define __AWH_EVENT_BASE__

/**
 * Стандартные модули
 */
#include <map>
#include <cmath>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <future>
#include <functional>

/**
 * Для операционной системы Linux
 */
#if __linux__
	// Подключаем модуль EPoll
	#include <sys/epoll.h>
/**
 * Для операционной системы Sun Solaris
 */
#elif __sun__
	// Подключаем модуль /dev/poll
	#include <sys/devpoll.h>
/**
 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
	// Подключаем модуль Kqueue
	#include <sys/event.h>
#endif

/**
 * Наши модули
 */
#include "evtimer.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Event Прототип класса события AWHEvent
	 */
	class Event;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Base Класс базы событий
	 */
	typedef class AWHSHARED_EXPORT Base {
		private:
			/**
			 * Event Устанавливаем дружбу с модулем события
			 */
			friend class Event;
		public:
			/**
			 * Тип режима получения события
			 */
			enum class event_mode_t : uint8_t {
				ENABLED  = 0x01, // Разрешено получение события
				DISABLED = 0x00  // Запрещено получение события 
			};
			/**
			 * Тип активного события
			 */
			enum class event_type_t : uint8_t {
				NONE  = 0x00, // Тип активного события не установлено
				CLOSE = 0x01, // Активное событие закрытия подключения
				READ  = 0x02, // Активное событие доступности данных на чтение
				WRITE = 0x03, // Активное событие доступности сокета на запись
				TIMER = 0x04  // Активное событие таймера в миллисекундах
			};
		private:
			// Максимальное количество отслеживаемых сокетов
			static constexpr const uint32_t MAX_COUNT_FDS = 0x5000;
		public:
			/**
			 * Создаём тип функции обратного вызова
			 */
			typedef function <void (const SOCKET, const event_type_t)> callback_t;
		private:
			/**
			 * Upstream Структура работы вышестоящего потока
			 */
			typedef struct Upstream {
				// Файловые дескрипторы для чтения и записи
				SOCKET read, write;
				// Объект работы с пайпом
				std::shared_ptr <evpipe_t> pipe;
				// Функция обратного вызова
				function <void (const uint64_t)> callback;
				/**
				 * Upstream Конструктор
				 */
				Upstream() noexcept :
				 read(INVALID_SOCKET), write(INVALID_SOCKET),
				 pipe(nullptr), callback(nullptr) {}
			} upstream_t;
			/**
			 * Peer Структура участника
			 */
			typedef struct Peer {
				// Отслеживаемый файловый дескриптор
				SOCKET fd;
				// Файловые дескрипторы таймеров
				SOCKET timer;
				// Идентификатор записи
				uint64_t id;
				// Флаг активации серийного таймера
				bool series;
				// Задержка времени таймера
				uint32_t delay;
				// Функция обратного вызова
				callback_t callback;
				// Объект работы с пайпом
				std::shared_ptr <evpipe_t> pipe;
				// Список соответствия типов событий режиму работы
				std::map <event_type_t, event_mode_t> mode;
				/**
				 * Peer Конструктор
				 */
				Peer() noexcept :
				 fd(INVALID_SOCKET), timer(INVALID_SOCKET), id(0),
				 series(false), delay(0), callback(nullptr), pipe(nullptr) {}
			} peer_t;
		private:
			// Идентификатор модуля
			uint64_t _id;
		private:
			// Флаг простого чтения базы событий
			bool _easily;
			// Флаг блокировки опроса базы событий
			bool _locker;
			// Флаг запуска работы базы событий
			bool _started;
			// Флаг запущенного опроса базы событий
			bool _launched;
		private:
			// Таймаут времени блокировки базы событий
			int32_t _baseDelay;
		private:
			// Максимальное количество обрабатываемых сокетов
			uint32_t _maxCount;
		private:
			/**
			 * Для операционной системы OS Windows
			 */
			#if _WIN32 || _WIN64
				// Объект данных запроса
				WSADATA _wsaData;
				// Флаг инициализации WinSocksAPI
				bool _winSockInit;
				// Список активных файловых дескрипторов
				vector <WSAPOLLFD> _fds;
			/**
			 * Для операционной системы Sun Solaris
			 */
			#elif __sun__
				// Идентификатор активного /dev/poll
				int32_t _wfd;
				// Список активных событий
				struct dvpoll _dopoll;
				// Список активных файловых дескрипторов
				vector <struct pollfd> _fds;
			/**
			 * Для операционной системы Linux
			 */
			#elif __linux__
				// Идентификатор активного EPoll
				int32_t _efd;
				// Список активных изменений событий
				vector <struct epoll_event> _change;
				// Список активных событий
				vector <struct epoll_event> _events;
			/**
			 * Для операционной системы FreeBSD, NetBSD, OpenBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
				// Идентификатор активного kqueue
				int32_t _kq;
				// Список активных изменений событий
				vector <struct kevent> _change;
				// Список активных событий
				vector <struct kevent> _events;
			#endif
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Объект работы с таймером событий
			evtimer_t _evtimer;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Список существующих таймеров
			std::set <SOCKET> _timers;
			// Список отслеживаемых участников
			std::map <SOCKET, peer_t> _peers;
			// Спиоск активных верхнеуровневых потоков
			std::map <uint64_t, upstream_t> _upstreams;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * id Метод получения идентификатора потока
			 * @return идентификатор потока для получения
			 */
			uint64_t id() const noexcept;
		private:
			/**
			 * stream Метод проверки запущен ли модуль в дочернем потоке
			 * @return результат проверки
			 */
			bool stream() const noexcept;
		private:
			/**
			 * init Метод инициализации базы событий
			 * @param mode флаг инициализации
			 */
			void init(const event_mode_t mode) noexcept;
		private:
			/**
			 * upstream Метод получения событий верхнеуровневых потоков
			 * @param sid  идентификатор верхнеуровневого потока
			 * @param fd   файловый дескриптор верхнеуровневого потока
			 * @param type тип отслеживаемого события
			 */
			void upstream(const uint64_t sid, const SOCKET fd, const event_type_t type) noexcept;
		private:
			/**
			 * del Метод удаления файлового дескриптора из базы событий
			 * @param fd файловый дескриптор для удаления
			 * @return   результат работы функции
			 */
			bool del(const SOCKET fd) noexcept;
			/**
			 * del Метод удаления файлового дескриптора из базы событий для всех событий
			 * @param id идентификатор записи
			 * @param fd файловый дескриптор для удаления
			 * @return   результат работы функции
			 */
			bool del(const uint64_t id, const SOCKET fd) noexcept;
			/**
			 * del Метод удаления файлового дескриптора из базы событий для указанного события
			 * @param id   идентификатор записи
			 * @param fd   файловый дескриптор для удаления
			 * @param type тип отслеживаемого события
			 * @return     результат работы функции
			 */
			bool del(const uint64_t id, const SOCKET fd, const event_type_t type) noexcept;
		private:
			/**
			 * add Метод добавления файлового дескриптора в базу событий
			 * @param id       идентификатор записи
			 * @param fd       файловый дескриптор для добавления
			 * @param callback функция обратного вызова при получении события
			 * @param delay    задержка времени для создания таймеров
			 * @param series   флаг серийного таймаута
			 * @return         результат работы функции
			 */
			bool add(const uint64_t id, SOCKET & fd, callback_t callback = nullptr, const uint32_t delay = 0, const bool series = false) noexcept;
		private:
			/**
			 * mode Метод установки режима работы модуля
			 * @param id   идентификатор записи
			 * @param fd   файловый дескриптор для установки режима работы
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const uint64_t id, const SOCKET fd, const event_type_t type, const event_mode_t mode) noexcept;
		public:
			/**
			 * launched Метод проверки запущена ли в данный момент база событий
			 * @return результат проверки запущена ли база событий
			 */
			bool launched() const noexcept;
		public:
			/**
			 * clear Метод очистки списка событий
			 */
			void clear() noexcept;
		public:
			/**
			 * kick Метод отправки пинка
			 */
			void kick() noexcept;
			/**
			 * stop Метод остановки чтения базы событий
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска чтения базы событий
			 */
			void start() noexcept;
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
		public:
			/**
			 * freeze Метод заморозки чтения данных
			 * @param mode флаг активации
			 */
			void freeze(const bool mode) noexcept;
			/**
			 * easily Метод активации простого режима чтения базы событий
			 * @param mode флаг активации
			 */
			void easily(const bool mode) noexcept;
		public:
			/**
			 * frequency Метод установки частоты обновления базы событий
			 * @param msec частота обновления базы событий в миллисекундах
			 */
			void frequency(const uint32_t msec = 10) noexcept;
		public:
			/**
			 * eraseUpstream Метод удаления верхнеуровневого потока
			 * @param sid идентификатор верхнеуровневого потока
			 */
			void eraseUpstream(const uint64_t sid) noexcept;
			/**
			 * launchUpstream Метод запуска верхнеуровневого потока
			 * @param sid идентификатор верхнеуровневого потока
			 * @param tid идентификатор трансферной передачи
			 */
			void launchUpstream(const uint64_t sid, const uint64_t tid = 0) noexcept;
			/**
			 * emplaceUpstream Метод создания верхнеуровневого потока
			 * @param callback функция обратного вызова
			 * @return         идентификатор верхнеуровневого потока
			 */
			uint64_t emplaceUpstream(function <void (const uint64_t)> callback) noexcept;
		public:
			/**
			 * Base Конструктор
			 * @param fmk   объект фреймворка
			 * @param log   объект для работы с логами
			 * @param count максимальное количество обрабатываемых сокетов
			 */
			Base(const fmk_t * fmk, const log_t * log, const uint32_t count = MAX_COUNT_FDS) noexcept;
			/**
			 * ~Base Деструктор
			 */
			~Base() noexcept;
	} base_t;
	/**
	 * Event Класс события AWHEvent
	 */
	typedef class Event {
		public:
			/**
			 * Типы события
			 */
			enum class type_t : uint8_t {
				NONE  = 0x00, // Тип события не установлен
				EVENT = 0x01, // Тип события обычное
				TIMER = 0x02  // Тип события таймер
			};
		private:
			// Режим активации события
			bool _mode;
		private:
			// Флаг активации серийного таймера
			bool _series;
		private:
			// Файловый дескриптор
			SOCKET _fd;
		private:
			// Идентификатор события
			uint64_t _id;
		private:
			// Тип события таймера
			type_t _type;
		private:
			// Задержка времени таймера
			uint32_t _delay;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Функция обратного вызова
			base_t::callback_t _callback;
		private:
			// База данных событий
			base_t * _base;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * type Метод получения типа события
			 * @return установленный тип события
			 */
			type_t type() const noexcept;
		public:
			/**
			 * set Метод установки базы событий
			 * @param base база событий для установки
			 */
			void set(base_t * base) noexcept;
			/**
			 * set Метод установки файлового дескриптора
			 * @param fd файловый дескриптор для установки
			 */
			void set(const SOCKET fd) noexcept;
			/**
			 * set Метод установки функции обратного вызова
			 * @param callback функция обратного вызова
			 */
			void set(base_t::callback_t callback) noexcept;
		public:
			/**
			 * Метод удаления типа события
			 * @param type тип события для удаления
			 */
			void del(const base_t::event_type_t type) noexcept;
		public:
			/**
			 * timeout Метод установки задержки времени таймера
			 * @param delay  задержка времени в миллисекундах
			 * @param series флаг серийного таймаута
			 */
			void timeout(const uint32_t delay, const bool series = false) noexcept;
		public:
			/**
			 * mode Метод установки режима работы модуля
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const base_t::event_type_t type, const base_t::event_mode_t mode) noexcept;
		public:
			/**
			 * stop Метод остановки работы события
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска работы события
			 */
			void start() noexcept;
		public:
			/**
			 * Оператор [=] для установки базы событий
			 * @param base база событий для установки
			 * @return     текущий объект
			 */
			Event & operator = (base_t * base) noexcept;
			/**
			 * Оператор [=] для установки файлового дескриптора
			 * @param fd файловый дескриптор для установки
			 * @return   текущий объект
			 */
			Event & operator = (const SOCKET fd) noexcept;
			/**
			 * Оператор [=] для установки задержки времени таймера
			 * @param delay задержка времени в миллисекундах
			 * @return      текущий объект
			 */
			Event & operator = (const uint32_t delay) noexcept;
			/**
			 * Оператор [=] для установки функции обратного вызова
			 * @param callback функция обратного вызова
			 * @return         текущий объект
			 */
			Event & operator = (base_t::callback_t callback) noexcept;
		public:
			/**
			 * Event Конструктор
			 * @param type тип события
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Event(const type_t type, const fmk_t * fmk, const log_t * log) noexcept :
			 _mode(false), _series(false), _fd(INVALID_SOCKET), _id(0), _type(type),
			 _delay(0), _callback(nullptr), _base(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~Event Деструктор
			 */
			~Event() noexcept;
	} event_t;
};

#endif // __AWH_EVENT_BASE__
