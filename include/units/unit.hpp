/**
 * @file: unit.hpp
 * @date: 2026-02-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл базового класса модулей — класс unit::Unit,
 *        задающий общий контракт всех модулей библиотеки: привязка к движку ввода-вывода, идентификация события,
 *        управление состоянием и подписка на обратные вызовы
 *
 * \~english
 * @brief Header file of the base class of the modules — the unit::Unit class,
 *        which defines the common contract of all the modules of the library: binding to the input-output engine, event identification,
 *        state management and subscription to the callbacks
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT__
#define __AWH_UNIT__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../net/io.hpp"
#include "../sys/signals.hpp"
#include "../sys/callback.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс базового модуля
		 *
		 * \~english
		 * @brief Base module class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Unit {
			protected:
				// Идентификатор процесса
				pid_t _pid;
			private:
				// Таймаут опроса базы событий
				int32_t _timeout;
			private:
				/**
				 * \~russian
				 * Признак просьбы об остановке, поступившей прежде запуска цикла
				 *
				 * @details Обращение может отказать ещё до входа в цикл событий - скажем,
				 *          рассылка не уходит в неисправную сеть, - и отказ этот приходит
				 *          вызывающему сразу же, из той самой функции, которой обращение
				 *          и начато. Просьба остановиться поступает тогда, когда цикла
				 *          ещё нет, и остановить ей нечего
				 *
				 *          Признак этот просьбу и запоминает: запуск сверяется с ним и в
				 *          цикл не входит вовсе. Без него запуск, следующий за отказом,
				 *          уходил бы в ожидание навсегда - остановить его было бы уже
				 *          некому
				 *
				 * \~english
				 * Flag of a stop request that has arrived before the loop has been started
				 * @details A call may fail even before entering the event loop — say,
				 *          a broadcast does not go out into a faulty network — and that refusal
				 *          reaches the caller immediately, from the very function by which the call
				 *          has been started. The request to stop then arrives at a moment when the loop
				 *          does not exist yet, and there is nothing for it to stop
				 *          This flag is what remembers the request: the launch checks it and does not
				 *          enter the loop at all. Without it, a launch following a refusal
				 *          would go into waiting forever — there would be no one left
				 *          to stop it
				 *
				 * \~
				 */
				bool _stopped;
			private:
				// Флаг активации перехвата сигналов
				event::mode_t _intercep;
			protected:
				// Статус работы модуля
				event::status_t _status;
			private:
				// Объект работы с сигналами
				signals_t _signals;
			protected:
				// Хранилище функций обратного вызова
				callback_t _callback;
			protected:
				// Объект асинхронного сетевого движка
				engine::io_t * _io;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * \~russian
				 * @brief Метод вывода полученного сигнала
				 *
				 * @param sig идентификатор сигнала
				 *
				 * \~english
				 * @brief Method of printing the received signal
				 * @param sig signal identifier
				 *
				 * \~
				 */
				void signal(const int32_t sig) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод принудительного пинка базе событий
				 *
				 * @return результат выполнения операции
				 *
				 * \~english
				 * @brief Method of forcibly kicking the event base
				 * @return result of performing the operation
				 *
				 * \~
				 */
				bool kick() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения мастер-процесса
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of determining the master process
				 * @return result of the check
				 *
				 * \~
				 */
				bool master() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки на запуск работы
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking whether the work has been started
				 * @return result of the check
				 *
				 * \~
				 */
				bool working() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки базы событий
				 *
				 * \~english
				 * @brief Method of clearing the event base
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод реинициализации базы событий
				 *
				 * \~english
				 * @brief Method of reinitializing the event base
				 *
				 * \~
				 */
				void reinit() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества событий в базе событий
				 *
				 * @return количество событий
				 *
				 * \~english
				 * @brief Method of getting the number of events in the event base
				 * @return number of events
				 *
				 * \~
				 */
				size_t events() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод остановки юнита
				 *
				 * @note База событий едина на весь процесс. Реально цикл событий останавливает только
				 *       юнит-лаунчер (тот, что его запустил): он сбрасывает флаг работы и будит базу.
				 *       Остальные юниты лишь переводят себя в статус DESTROYED, не затрагивая общий цикл.
				 *
				 * \~english
				 * @brief Method of stopping the unit
				 * @note The event base is common to the whole process. The event loop is actually stopped only by
				 *       the launcher unit (the one that has started it): it resets the working flag and wakes the base up.
				 *       The rest of the units merely move themselves into the DESTROYED status without touching the common loop.
				 *
				 * \~
				 */
				virtual void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска юнита
				 *
				 * @note База событий едина на весь процесс. Запустить цикл событий может только один
				 *       юнит-лаунчер: захват выполняется атомарно (compare_exchange), и для него вызов
				 *       блокирующий — поток крутится в цикле опроса до остановки. Остальные юниты,
				 *       обнаружив, что база уже работает, цикл не запускают, не блокируются и лишь
				 *       переводят себя в статус LAUNCHED.
				 *
				 * \~english
				 * @brief Method of launching the unit
				 * @note The event base is common to the whole process. The event loop can be started by only one
				 *       launcher unit: the capture is performed atomically (compare_exchange), and for it the call is
				 *       blocking — the thread spins in the polling loop until the stop. The rest of the units,
				 *       having discovered that the base is already working, do not start the loop, do not block and merely
				 *       move themselves into the LAUNCHED status.
				 *
				 * \~
				 */
				virtual void start() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением события
				 *
				 * @note Дескриптор пересоздаётся, а само событие (его идентификатор, коллбэки,
				 *       параметры) сохраняется - подмена дескриптора приложению незаметна
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения перестройки
				 *
				 * \~english
				 * @brief Method of rebuilding an event: recreation of the underlying descriptor while preserving the event
				 * @note The descriptor is recreated, while the event itself (its identifier, callbacks,
				 *       parameters) is preserved — the substitution of the descriptor is invisible to the application
				 * @param eid event identifier
				 * @return    result of performing the rebuild
				 *
				 * \~
				 */
				bool rebuild(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа события
				 *
				 * @param eid идентификатор события
				 * @return    тип события
				 *
				 * \~english
				 * @brief Method of getting the event type
				 * @param eid event identifier
				 * @return    event type
				 *
				 * \~
				 */
				event::type_t type(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения типа узла события
				 *
				 * @param eid идентификатор события
				 * @return    тип узла события
				 *
				 * \~english
				 * @brief Method of getting the unit type of the event
				 * @param eid event identifier
				 * @return    unit type of the event
				 *
				 * \~
				 */
				event::node_t node(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения семейства события
				 *
				 * @param eid идентификатор события
				 * @return    семейство адресов
				 *
				 * \~english
				 * @brief Method of getting the family of the event
				 * @param eid event identifier
				 * @return    address family
				 *
				 * \~
				 */
				event::family_t family(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения статуса события
				 *
				 * @param eid идентификатор события
				 * @return    статус события
				 *
				 * \~english
				 * @brief Method of getting the event status
				 * @param eid event identifier
				 * @return    event status
				 *
				 * \~
				 */
				event::status_t status(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения протокола события
				 *
				 * @param id идентификатор события
				 * @return   протокол события
				 *
				 * \~english
				 * @brief Method of getting the event protocol
				 * @param id event identifier
				 * @return   event protocol
				 *
				 * \~
				 */
				event::protocol_t protocol(const event::id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа внутренних таймеров
				 *
				 * @return тип таймера для событий сетевого движка
				 *
				 * \~english
				 * @brief Method of getting the type of the internal timers
				 * @return timer type for the events of the network engine
				 *
				 * \~
				 */
				event::timer_t getInternalTimer() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа внутренних таймеров
				 *
				 * @param timer тип таймера для событий сетевого движка
				 *
				 * \~english
				 * @brief Method of setting the type of the internal timers
				 * @param timer timer type for the events of the network engine
				 *
				 * \~
				 */
				void setInternalTimer(const event::timer_t timer) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки пропускной способности события
				 *
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 *
				 * \~english
				 * @brief Method of setting the bandwidth of the event
				 * @param limiting  mode of limiting the bandwidth of the event (egress or ingress)
				 * @param bandwidth bandwidth of the event to be set (for example, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" or "auto")
				 *
				 * \~
				 */
				void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки времени блокировки базы событий в ожидании событий
				 *
				 * @param timeout время ожидания событий в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the blocking time of the event base while waiting for events
				 * @param timeout time of waiting for events in milliseconds
				 *
				 * \~
				 */
				void rate(const int32_t timeout = -1) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions
				 *
				 * \~
				 */
				virtual void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 * \~english
				 * @brief Template of the method of connecting a callback function
				 * @tparam T    callback function type
				 * @tparam Args callback function arguments
				 *
				 * \~
				 */
				template <typename T, class... Args>
				/**
				 * \~russian
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 * \~english
				 * @brief Method of connecting a callback function
				 * @param name callback function identifier
				 * @param args callback function arguments
				 * @return     identifier of the added callback function
				 *
				 * \~
				 */
				auto on(const char * name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * \~russian
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 * \~english
				 * @brief Template of the method of connecting a callback function
				 * @tparam T    callback function type
				 * @tparam Args callback function arguments
				 *
				 * \~
				 */
				template <typename T, class... Args>
				/**
				 * \~russian
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 * \~english
				 * @brief Method of connecting a callback function
				 * @param name callback function identifier
				 * @param args callback function arguments
				 * @return     identifier of the added callback function
				 *
				 * \~
				 */
				auto on(string_view name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * \~russian
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 * \~english
				 * @brief Template of the method of connecting a callback function
				 * @tparam T    callback function type
				 * @tparam Args callback function arguments
				 *
				 * \~
				 */
				template <typename T, class... Args>
				/**
				 * \~russian
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 * \~english
				 * @brief Method of connecting a callback function
				 * @param name callback function identifier
				 * @param args callback function arguments
				 * @return     identifier of the added callback function
				 *
				 * \~
				 */
				auto on(const string & name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * \~russian
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 * \~english
				 * @brief Template of the method of connecting a callback function
				 * @tparam T    callback function type
				 * @tparam Args callback function arguments
				 *
				 * \~
				 */
				template <typename T, class... Args>
				/**
				 * \~russian
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 * \~english
				 * @brief Method of connecting a callback function
				 * @param fid  callback function identifier
				 * @param args callback function arguments
				 * @return     identifier of the added callback function
				 *
				 * \~
				 */
				auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
					// Если мы получили идентификатор функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (fid, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * \~russian
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam A    тип идентификатора функции
				 * @tparam B    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 * \~english
				 * @brief Template of the method of connecting a callback function
				 * @tparam A    function identifier type
				 * @tparam B    callback function type
				 * @tparam Args callback function arguments
				 *
				 * \~
				 */
				template <typename A, typename B, class... Args>
				/**
				 * \~russian
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 * \~english
				 * @brief Method of connecting a callback function
				 * @param fid  callback function identifier
				 * @param args callback function arguments
				 * @return     identifier of the added callback function
				 *
				 * \~
				 */
				auto on(const A fid, Args... args) noexcept -> uint32_t {
					// Если мы получили на вход число
					if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации перехвата сигналов
				 *
				 * @param mode флаг активации
				 *
				 * \~english
				 * @brief Method of activating/deactivating the interception of signals
				 * @param mode activation flag
				 *
				 * \~
				 */
				void interception(const event::mode_t mode) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				Unit(const Unit &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				Unit & operator = (const Unit &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Unit(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				virtual ~Unit() noexcept;
		} unit_t;
	};
};

#endif // __AWH_UNIT__
