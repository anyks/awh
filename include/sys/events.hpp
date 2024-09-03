/**
 * @file: events.hpp
 * @date: 2024-06-26
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

#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартные модули
 */
#include <map>
#include <set>
#include <cmath>
#include <ctime>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <future>
#include <functional>

/**
 * Если это Linux
 */
#if __linux__
	// Подключаем модуль EPoll
	#include <sys/epoll.h>
	#include <sys/timerfd.h>
/**
 * Если это FreeBSD или MacOS X
 */
#elif __APPLE__ || __MACH__ || __FreeBSD__
	// Подключаем модуль Kqueue
	#include <sys/event.h>
#endif

/**
 * Наши модули
 */
#include <sys/timeout.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Event Прототип класса события AWHEvent
	 */
	class Event;
	/**
	 * Base Класс базы событий
	 */
	typedef class Base {
		private:
			/**
			 * Event Устанавливаем дружбу с модулем события
			 */
			friend class Event;
		private:
			/**
			 * Экшен выполняемого запроса
			 */
			enum class action_t : uint8_t {
				NONE   = 0x00, // Экшен для вызова метода не установлен
				DEL    = 0x01, // Экшен вызова метода "del"
				ADD    = 0x02, // Экшен вызова метода "add"
				MODE   = 0x03, // Экшен вызова метода "mode"
				KICK   = 0x04, // Экшен вызова метода "kick"
				STOP   = 0x05, // Экшен вызова метода "stop"
				START  = 0x06, // Экшен вызова метода "start"
				CLEAR  = 0x07, // Экшен вызова метода "clear"
				REBASE = 0x08  // Экшен вызова метода "rebase"
			};
		public:
			/**
			 * Тип работы базы событий
			 */
			enum class mode_t : uint8_t {
				MAIN  = 0x00, // Работа базы событий в основном потоке
				CHILD = 0x01  // Работа базы событий в дочернем потоке
			};
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
			static constexpr const uint32_t MAX_COUNT_FDS = 20480;
		public:
			/**
			 * Создаём тип функции обратного вызова
			 */
			typedef std::function <void (const SOCKET, const event_type_t)> callback_t;
		private:
			/**
			 * FileDescriptor Класс работы с файловым дескриптором
			 */
			typedef class FileDescriptor {
				private:
					// Файловый дескриптор в виде значения
					SOCKET _value1;
					// Файловый дескриптор в виде указателя
					SOCKET * _value2;
				public:
					/**
					 * Оператор присвоения значения сокета
					 * @param value значение сокета для присвоения
					 * @return      текущий объект
					 */
					FileDescriptor & operator = (SOCKET value) noexcept {
						// Выполняем присвоение значения сокета
						this->_value1 = value;
						// Выводим значение текущего объекта
						return (* this);
					}
					/**
					 * Оператор присвоения указателя на сокет
					 * @param value указатель на сокет для присвоения
					 * @return      текущий объект
					 */
					FileDescriptor & operator = (SOCKET * value) noexcept {
						// Выполняем присвоение указателя на сокет
						this->_value2 = value;
						// Выводим значение текущего объекта
						return (* this);
					}
				public:
					/**
					 * Оператор извлечения установленного сокета
					 * @return SOCKET значение сокета для извлечения
					 */
					operator SOCKET() noexcept {
						// Если сокет установлен как указатель
						if(this->_value2 != nullptr)
							// Выводим значение сокета
							return (* this->_value2);
						// Выводим значение сокета как оно есть
						else return this->_value1;
					}
					/**
					 * Оператор извлечения установленного указателя на сокет
					 * @return SOCKET указатель на сокет для извлечения
					 */
					operator SOCKET * () noexcept {
						// Если сокет установлен как указатель
						if(this->_value2 != nullptr)
							// Выводим указатель на сокет
							return this->_value2;
						// Выводим указатель на сокет
						else return &this->_value1;
					}
				public:
					/**
					 * FileDescriptor Конструктор
					 */
					FileDescriptor() noexcept : _value1(0), _value2(nullptr) {}
					/**
					 * FileDescriptor Конструктор
					 * @param value значение сокета для присвоения
					 */
					FileDescriptor(SOCKET value) noexcept : _value1(value), _value2(nullptr) {}
					/**
					 * FileDescriptor Конструктор
					 * @param value указатель на сокет для присвоения
					 */
					FileDescriptor(SOCKET * value) noexcept : _value1(0), _value2(value) {}
			} __attribute__((packed)) fd_t;
			/**
			 * Message Структура сообщения передаваемая между потоками
			 */
			typedef struct Message {
				// Объект файлового дескриптора
				fd_t fd;
				// Идентификатор записи
				uint64_t id;
				// Флаг активации серийного таймера
				bool series;
				// Задержка времени таймера
				time_t delay;
				// Экшен выполняемого метода
				action_t action;
				// Тип отслеживаемого события
				event_type_t type;
				// Флаг режима работы модуля
				event_mode_t mode;
				// Функция обратного вызова
				callback_t callback;
				/**
				 * Message Конструктор
				 */
				Message() noexcept :
				 id(0), series(false), delay(0), action(action_t::NONE),
				 type(event_type_t::NONE), mode(event_mode_t::DISABLED), callback(nullptr) {}
			} message_t;
			/**
			 * Item Структура данных участника
			 */
			typedef struct Item {
				// Отслеживаемый файловый дескриптор
				SOCKET fd;
				// Идентификатор записи
				uint64_t id;
				// Флаг активации серийного таймера
				bool series;
				// Задержка времени таймера
				time_t delay;
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Объект работы с пайпом
					shared_ptr <pipe_t> pipe;
				/**
				 * Если это Linux
				 */
				#elif __linux__
					// Параметры таймера
					struct itimerspec timer;
				/**
				 * Если это FreeBSD или MacOS X
				 */
				#elif __APPLE__ || __MACH__ || __FreeBSD__
					// Объект работы с пайпом
					shared_ptr <pipe_t> pipe;
					// Файловые дескрипторы таймеров
					SOCKET timer = INVALID_SOCKET;
				#endif
				// Функция обратного вызова
				callback_t callback;
				// Список соответствия типов событий режиму работы
				std::map <event_type_t, event_mode_t> mode;
				/**
				 * Item Конструктор
				 */
				Item() noexcept : fd(INVALID_SOCKET), id(0), series(false), delay(0), callback(nullptr) {}
			} item_t;
		private:
			// Идентификатор модуля
			uint64_t _id;
		private:
			// Флаг простого чтения базы событий
			bool _easily;
			// Флаг блокировки опроса базы событий
			bool _locker;
			// Флаг статуса результата отправки события
			bool _sending;
			// Флаг запуска работы базы событий
			bool _started;
			// Флаг запущенного опроса базы событий
			bool _launched;
		private:
			// Режим работы базы событий
			mode_t _mode;
		private:
			// Таймаут времени блокировки базы событий
			int32_t _baseDelay;
		private:
			// Максимальное количество обрабатываемых сокетов
			uint32_t _maxCount;
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Объект данных запроса
				WSADATA _wsaData;
				// Флаг инициализации WinSocksAPI
				bool _winSockInit;
				// Список активных файловых дескрипторов
				std::vector <WSAPOLLFD> _fds;
			/**
			 * Если это Linux
			 */
			#elif __linux__
				// Идентификатор активного EPoll
				int32_t _efd;
				// Список активных изменений событий
				std::vector <struct epoll_event> _change;
				// Список активных событий
				std::vector <struct epoll_event> _events;
			/**
			 * Если это FreeBSD или MacOS X
			 */
			#elif __APPLE__ || __MACH__ || __FreeBSD__
				// Идентификатор активного kqueue
				int32_t _kq;
				// Список активных изменений событий
				std::vector <struct kevent> _change;
				// Список активных событий
				std::vector <struct kevent> _events;
			#endif
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Объект работы с таймерами скрина
			timeout_t _timeout;
		private:
			// Объект обещания биндинга базы событий
			std::promise <void> _base;
			// Объект обещания на получение события
			std::promise <bool> _event;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <message_t> _screen;
		private:
			// Список отслеживаемых участников
			std::map <SOCKET, item_t> _items;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * id Метод получения идентификатора потока
			 * @return идентификатор потока для получения
			 */
			uint64_t id() const noexcept;
		public:
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
			void init(const bool mode) noexcept;
		private:
			/**
			 * process Метод обработки процесса выполнения экшенов внутри дочернего потока
			 * @param message сообщение экшена для выполнения
			 */
			void process(message_t message) noexcept;
		private:
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
			bool add(const uint64_t id, SOCKET & fd, callback_t callback = nullptr, const time_t delay = 0, const bool series = false) noexcept;
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
		public:
			/**
			 * rebase Метод пересоздания базы событий
			 */
			void rebase() noexcept;
			/**
			 * mode Метод установки режима работы базы событий
			 * @param mode режим работы базы событий
			 */
			void mode(const mode_t mode) noexcept;
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
			// Мютекс для блокировки основного потока
			mutex _mtx;
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
			time_t _delay;
		private:
			// Функция обратного вызова
			base_t::callback_t _callback;
		private:
			// База данных событий
			base_t * _base;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
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
			void timeout(const time_t delay, const bool series = false) noexcept;
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
			Event & operator = (const time_t delay) noexcept;
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

#endif // __AWH_EVENTS__
