/**
 * @file: unit.cpp
 * @date: 2026-02-20
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <atomic>
#include <type_traits>

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/unit.hpp>

/**
 * Если мы работаем с компилятором MinGW в MS Windows
 */
#if __MINGW32__ || __MINGW64__
	/**
	 * Если процессор принадлежит к x86_64
	 */
	#if __x86_64__ || _M_X64
		/**
		 * Подключаем заголовочный файл для процессора x86_64
		 */
		#include <emmintrin.h>

		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() _mm_pause()
	/**
	 * Если процессор принадлежит к x86
	 */
	#elif __i386__ || _M_IX86
		/**
		 * Подключаем заголовочный файл для процессора x86
		 */
		#include <emmintrin.h>

		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() _mm_pause()
	/**
	 * Если процессор принадлежит к ARM64
	 */
	#elif __aarch64__ || _M_ARM64
		/**
		 * Для компилятора GCC/Clang
		 */
		#if __clang__
			/**
			 * Формируем функцию паузы для CPU (Clang)
			 */
			#define CPU_PAUSE() __builtin_arm_yield()
		/**
		 * Для компилятора GCC (в GCC нет __builtin_arm_yield)
		 */
		#elif __GNUC__
			/**
			 * Формируем функцию паузы для CPU (GCC)
			 */
			#define CPU_PAUSE() __asm__ __volatile__("yield")
		/**
		 * Для компилятора MSVC на ARM:
		 */
		#else
			/**
			 * Формируем функцию паузы для CPU
			 */
			#define CPU_PAUSE() __yield()
		#endif
	/**
	 * Если процессор принадлежит к ARM/ARM64
	 */
	#elif __arm__ || _M_ARM
		/**
		 * Для компилятора GCC/Clang
		 */
		#if __clang__
			/**
			 * Формируем функцию паузы для CPU (Clang)
			 */
			#define CPU_PAUSE() __builtin_arm_yield()
		/**
		 * Для компилятора GCC (в GCC нет __builtin_arm_yield)
		 */
		#elif __GNUC__
			/**
			 * Формируем функцию паузы для CPU (GCC)
			 */
			#define CPU_PAUSE() __asm__ __volatile__("yield")
		/**
		 * Для компилятора MSVC на ARM:
		 */
		#else
			/**
			 * Формируем функцию паузы для CPU
			 */
			#define CPU_PAUSE() __yield()
		#endif
	/**
	 * Для остальных типов процессоров
	 */
	#else
		/**
		 * Стандартный заголовочный файл
		 */
		#include <thread>

		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() std::this_thread::yield()
	#endif
/**
 * Если мы работаем с другими компиляторами
 */
#else
	/**
	 * Если процессор принадлежит к x86/x86_64
	 */
	#if __x86_64__ || __i386__ || _M_X64 || _M_IX86
		/**
		 * Подключаем заголовочный файл для процессора x86/x86_64
		 */
		#include <emmintrin.h>

		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() _mm_pause()
	/**
	 * Если процессор принадлежит к ARM/ARM64
	 */
	#elif __aarch64__ || __arm__ || _M_ARM64 || _M_ARM
		/**
		 * Для компилятора GCC/Clang
		 */
		#if __clang__
			/**
			 * Формируем функцию паузы для CPU (Clang)
			 */
			#define CPU_PAUSE() __builtin_arm_yield()
		/**
		 * Для компилятора GCC (в GCC нет __builtin_arm_yield)
		 */
		#elif __GNUC__
			/**
			 * Формируем функцию паузы для CPU (GCC)
			 */
			#define CPU_PAUSE() __asm__ __volatile__("yield")
		/**
		 * Для компилятора MSVC на ARM:
		 */
		#else
			/**
			 * Формируем функцию паузы для CPU
			 */
			#define CPU_PAUSE() __yield()
		#endif
	/**
	 * Если процессор принадлежит к PowerPC
	 */
	#elif __powerpc__ || __ppc__
		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() __asm__ volatile("or 27,27,27")
	/**
	 * Если процессор принадлежит к MIPS
	 */
	#elif __mips__
		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() __asm__ volatile("pause")
	/**
	 * Для остальных типов процессоров
	 */
	#else
		/**
		 * Стандартный заголовочный файл
		 */
		#include <thread>

		/**
		 * Формируем функцию паузы для CPU
		 */
		#define CPU_PAUSE() std::this_thread::yield()
	#endif
#endif

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Флаг запуска работы базы событий
	 *
	 */
	atomic_bool __awh_working__ = false;
	/**
	 * @brief Идентификатор юнита который запустил работу цикла событий
	 *
	 */
	atomic_uint64_t __awh_launcher__ = 0;
	/**
	 * @brief Количество ссылок юнитов на базу событий
	 *
	 */
	atomic_uint16_t __awh_count_refs__ = 0;
	/**
	 * @brief Объект рабочей базы событий
	 *
	 */
	unique_ptr <awh::engine::io_t> __awh_event_base__ = nullptr;
};

/**
 * @brief Метод вывода полученного сигнала
 *
 * @param sig идентификатор сигнала
 */
void awh::unit::Unit::signal(const int32_t sig) const noexcept {
	// Если процесс является дочерним
	if(this->_pid != ::getpid()){
		/**
		 * Определяем тип сигнала
		 */
		switch(sig){
			// Если возникает сигнал ручной остановкой процесса
			case SIGINT:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] has been terminated, goodbye!", log_t::flag_t::INFO, ::getpid());
				// Выходим из приложения
				::exit(0);
			break;
			// Если возникает сигнал ошибки выполнения арифметической операции
			case SIGFPE:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGFPE");
			break;
			// Если возникает сигнал выполнения неверной инструкции
			case SIGILL:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGILL");
			break;
			// Если возникает сигнал запроса принудительного завершения процесса
			case SIGTERM:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGTERM");
			break;
			// Если возникает сигнал сегментации памяти (обращение к несуществующему адресу памяти)
			case SIGSEGV:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGSEGV");
			break;
			// Если возникает сигнал запроса принудительное закрытие приложения из кода программы
			case SIGABRT:
				// Записываем в лог сообщение об завершении работы процесса
				this->_log->print("Child process [%u] was terminated by [%s] signal", log_t::flag_t::WARNING, ::getpid(), "SIGABRT");
			break;
		}
		// Выходим принудительно из приложения
		::exit(EXIT_FAILURE);
	// Если процесс является родительским
	} else {
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("crash");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const int32_t)> (fid, sig);
		// Выходим из приложения
		else ::exit(sig);
	}
}
/**
 * @brief Метод принудительного пинка базе событий
 *
 * @return результат выполнения операции
 */
bool awh::unit::Unit::kick() noexcept {
	// Выполняем принудительный пинок базе событий
	return ::__awh_event_base__->kick();
}
/**
 * @brief Метод определения мастер-процесса
 *
 * @return результат проверки
 */
bool awh::unit::Unit::master() const noexcept {
	// Возвращаем результат проверки
	return (this->_pid == ::getpid());
}
/**
 * @brief Метод проверки на запуск работы
 *
 * @return результат проверки
 */
bool awh::unit::Unit::working() const noexcept {
	// Выполняем проверку запущена ли работа базы событий
	return (this->_status == event::status_t::LAUNCHED);
}
/**
 * @brief Метод очистки базы событий
 *
 */
void awh::unit::Unit::clear() noexcept {
	// Выполняем очистку базы событий
	::__awh_event_base__->clear();
}
/**
 * @brief Метод реинициализации базы событий
 *
 */
void awh::unit::Unit::reinit() noexcept {
	// Выполняем реинициализацию базы событий, если реинициализация не выполнена
	if(!::__awh_event_base__->reinitialize()){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("Event database reinitialization failed: %s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("Event database reinitialization failed: %s", log_t::flag_t::CRITICAL, ::strerror(errno));
		#endif
	}
}
/**
 * @brief Метод получения количества событий в базе событий
 *
 * @return количество событий
 */
size_t awh::unit::Unit::events() const noexcept {
	// Возвращаем количество событий в базе событий
	return ::__awh_event_base__->eventsCount();
}
/**
 * @brief Метод остановки юнита
 *
 * @note База событий едина на весь процесс. Реально цикл событий останавливает только
 *       юнит-лаунчер (тот, что его запустил): он сбрасывает флаг работы и будит базу.
 *       Остальные юниты лишь переводят себя в статус DESTROYED, не затрагивая общий цикл.
 */
void awh::unit::Unit::stop() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если работа цикла событий базы событий запущена
		if(this->_status == event::status_t::LAUNCHED){
			// Если данный юнит запустил работу базы событий
			if(::__awh_launcher__.load(std::memory_order_acquire) == static_cast <uint64_t> (reinterpret_cast <uintptr_t> (this))){
				// Выполняем остановку работы цикла базы событий
				::__awh_working__.store(false, std::memory_order_release);
				// Разблокируем работу базы событий
				::__awh_event_base__->kick();
			// Если базу событий запустил не этот юнит
			} else {
				// Устанавливаем флаг статуса остановки работы юнита
				this->_status = event::status_t::DESTROYED;
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> ("status", this->_status);
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод запуска юнита
 *
 * @note База событий едина на весь процесс. Запустить цикл событий может только один
 *       юнит-лаунчер: захват выполняется атомарно (compare_exchange), и для него вызов
 *       блокирующий — поток крутится в цикле опроса до остановки. Остальные юниты,
 *       обнаружив, что база уже работает, цикл не запускают, не блокируются и лишь
 *       переводят себя в статус LAUNCHED.
 */
void awh::unit::Unit::start() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Ожидаемое состояние работы базы событий (база ещё не запущена)
		bool expected = false;
		// Если работа базы событий не запущена, атомарно захватываем её запуск (защита от гонки check-then-act)
		if(::__awh_working__.compare_exchange_strong(expected, true, std::memory_order_acq_rel)){
			// Запоминаем идентификатор юнита запустивщего работу базы событий
			::__awh_launcher__.store(static_cast <uint64_t> (reinterpret_cast <uintptr_t> (this)), std::memory_order_release);
			// Устанавливаем флаг статуса запуска работы юнита
			this->_status = event::status_t::LAUNCHED;
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("status", this->_status);
			/**
			 * Выполняем запуск работы цикла базы событий
			 */
			while(::__awh_working__.load(std::memory_order_acquire)){
				/**
				 * Выполняем опрос базы событий
				 */
				if(!::__awh_event_base__->poll(this->_timeout)){
					// Если таймаут установлен очень низкий
					if((this->_timeout > -1) && (this->_timeout <= 10))
						/**
						 * Замедляем работу процессора
						 */
						CPU_PAUSE();
				}
			}
			// Сбрасываем идентификатор юнита запустившего работу базы событий
			::__awh_launcher__.store(0, std::memory_order_release);
			// Устанавливаем флаг статуса остановки работы юнита
			this->_status = event::status_t::DESTROYED;
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("status", this->_status);
		// Если запуск работы цикла событий ещё не выполнен
		} else if(this->_status != event::status_t::LAUNCHED) {
			// Устанавливаем флаг статуса запуска работы юнита
			this->_status = event::status_t::LAUNCHED;
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("status", this->_status);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением события
 *
 * @note Дескриптор пересоздаётся, а само событие (его идентификатор, коллбэки,
 *       параметры) сохраняется - подмена дескриптора приложению незаметна
 *
 * @param eid идентификатор события
 * @return    результат выполнения перестройки
 */
bool awh::unit::Unit::rebuild(const event::id_t eid) noexcept {
	// Пересоздаём дескриптор события, сохраняя само событие (его идентификатор, коллбэки, параметры)
	return ::__awh_event_base__->rebuild(eid);
}
/**
 * @brief Метод получения типа события
 *
 * @param eid идентификатор события
 * @return    тип события
 */
awh::event::type_t awh::unit::Unit::type(const event::id_t eid) const noexcept {
	// Возвращаем тип события
	return ::__awh_event_base__->type(eid);
}
/**
 * @brief Метод получения типа узла события
 *
 * @param eid идентификатор события
 * @return    тип узла события
 */
awh::event::node_t awh::unit::Unit::node(const event::id_t eid) const noexcept {
	// Возвращаем тип узла события
	return ::__awh_event_base__->node(eid);
}
/**
 * @brief Метод получения семейства события
 *
 * @param eid идентификатор события
 * @return    семейство адресов
 */
awh::event::family_t awh::unit::Unit::family(const event::id_t eid) const noexcept {
	// Возвращаем семейство адресов
	return ::__awh_event_base__->family(eid);
}
/**
 * @brief Метод получения статуса события
 *
 * @param eid идентификатор события
 * @return    статус события
 */
awh::event::status_t awh::unit::Unit::status(const event::id_t eid) const noexcept {
	// Возвращаем статус события
	return ::__awh_event_base__->status(eid);
}
/**
 * @brief Метод получения протокола события
 *
 * @param id идентификатор события
 * @return   протокол события
 */
awh::event::protocol_t awh::unit::Unit::protocol(const event::id_t id) const noexcept {
	// Возвращаем протокол события
	return ::__awh_event_base__->protocol(id);
}
/**
 * @brief Метод получения типа внутренних таймеров
 *
 * @return тип таймера для событий сетевого движка
 */
awh::event::timer_t awh::unit::Unit::getInternalTimer() const noexcept {
	// Возвращаем тип таймера для событий сетевого движка
	return ::__awh_event_base__->getInternalTimer();
}
/**
 * @brief Метод установки типа внутренних таймеров
 *
 * @param timer тип таймера для событий сетевого движка
 */
void awh::unit::Unit::setInternalTimer(const event::timer_t timer) noexcept {
	// Устанавливаем тип таймера для событий сетевого движка
	::__awh_event_base__->setInternalTimer(timer);
}
/**
 * @brief Метод установки пропускной способности события
 *
 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 */
void awh::unit::Unit::bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Выполняем установку пропускной способности события для события
	::__awh_event_base__->bandwidth(limiting, bandwidth);
}
/**
 * @brief Метод установки времени блокировки базы событий в ожидании событий
 *
 * @param timeout время ожидания событий в миллисекундах
 */
void awh::unit::Unit::rate(const int32_t timeout) noexcept {
	// Устанавливаем время блокировки базы событий
	this->_timeout = timeout;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 */
void awh::unit::Unit::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова при краше приложения
	this->_callback.set("crash", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова при запуске/остановки работы модуля
	this->_callback.set("status", callback);
}
/**
 * @brief Метод активации/деактивации перехвата сигналов
 *
 * @param mode флаг активации
 */
void awh::unit::Unit::interception(const event::mode_t mode) noexcept {
	// Если флаг перехвата сигналов не соответствует
	if(this->_intercep != mode){
		// Активируем/деактивируем флаг перехвата сигналов
		this->_intercep = mode;
		/**
		 * Определяем флаг активации
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если передан флаг активации перехвата сигналов
			case static_cast <uint8_t> (event::mode_t::ENABLED):
				// Устанавливаем функцию обработки сигналов
				this->_signals.on(std::bind(&unit_t::signal, this, _1));
				// Выполняем запуск отслеживания сигналов
				this->_signals.start();
			break;
			// Если передан флаг деактивации перехвата сигналов
			case static_cast <uint8_t> (event::mode_t::DISABLED):
				// Выполняем остановку отслеживания сигналов
				this->_signals.stop();
			break;
		}
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::Unit::Unit(const fmk_t * fmk, const log_t * log) noexcept :
 _pid(::getpid()), _timeout(-1),
 _intercep(event::mode_t::DISABLED),
 _status(event::status_t::NONE), _signals(fmk, log),
 _callback(fmk, log), _io(nullptr), _fmk(fmk), _log(log) {
	// Если база событий не инициализированна
	if(::__awh_event_base__ == nullptr){
		// Выполняем создание базы событий
		::__awh_event_base__ = make_unique <engine::io_t> (fmk, log);
		// Инициализируем базу событий, если база событий не инициализированна
		if(!::__awh_event_base__->initialize()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("Event database could not be initialized: %s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, ::strerror(errno));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("Event database could not be initialized: %s", log_t::flag_t::CRITICAL, ::strerror(errno));
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
	// Увеличиваем количество ссылок на базу событий
	::__awh_count_refs__.fetch_add(1, memory_order_relaxed);
	// Запоминаем объект базы событий
	this->_io = ::__awh_event_base__.get();
}
/**
 * @brief Деструктор
 *
 */
awh::unit::Unit::~Unit() noexcept {
	/**
	 * Уменьшаем количество ссылок на базу событий и проверяем, был ли это последний юнит
	 * (fetch_sub возвращает предыдущее значение; acq_rel синхронизирует декременты других потоков)
	 */
	if(::__awh_count_refs__.fetch_sub(1, std::memory_order_acq_rel) == 1){
		// Останавливаем работу цикла обработки базы событий
		this->stop();
		// Выполняем деинициализацию базы событий
		::__awh_event_base__->deinitialize();
		// Выполняем удаление базы событий
		::__awh_event_base__.reset(nullptr);
	}
}
