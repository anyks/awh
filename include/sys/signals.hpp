/**
 * @file signals.hpp
 * @date 2026-01-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля обработки сигналов — класс Signals для перехвата SIGINT, SIGTERM, SIGSEGV, SIGBUS,
 *        SIGILL, SIGFPE и SIGABRT через sigaction на POSIX-системах и через signal() на MS Windows
 *
 * \~english
 * @brief Header file of the signal handling module — the Signals class for intercepting SIGINT, SIGTERM, SIGSEGV, SIGBUS,
 *        SIGILL, SIGFPE and SIGABRT through sigaction on POSIX systems and through signal() on MS Windows
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <csignal>
#include <functional>

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	/**
	 * Системные заголовочные файлы
	 */
	#include <setjmp.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include "locker.hpp"
#include "macro/global.hpp"

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Системный заголовочный файл
	 */
	#include <tchar.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Системный заголовочный файл для типов pid_t/uid_t
	 */
	#include <sys/types.h>
#endif

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс работы с сигналами
	 *
	 * \~english
	 * @brief Class for working with signals
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Signals {
		public:
			/**
			 * \~russian
			 * @brief Класс точки восстановления потока на сигнал SIGBUS
			 *
			 * @details Проекция файла в память отвечает сигналом SIGBUS, если файл усечён
			 *          за её пределами либо носитель отказал в чтении: прежде сбойный поток
			 *          приостанавливался до конца процесса. Класс ставит потоку точку
			 *          возврата — обработчик сигнала возвращает управление в неё, и чтение
			 *          из проекции завершается честным отказом, не трогая остальной процесс.
			 *          Точка ставится на время одного чтения из проекции; вложенные точки
			 *          одним потоком не поддерживаются. Под MS Windows проекция за концом
			 *          файла заполняется нулями и сигнала нет — класс там пуст
			 *
			 * \~english
			 * @brief Class of the thread recovery point for the SIGBUS signal
			 *
			 * @details A file mapped into memory answers with SIGBUS if the file is truncated
			 *          behind the mapping or the medium refuses to read: the faulting thread
			 *          used to be suspended until the end of the process. The class sets a
			 *          return point in the frame of the calling function — the signal handler returns control to it,
			 *          and the read from the mapping ends with an honest refusal without
			 *          touching the rest of the process. The point is set for the duration of
			 *          a single read from a mapping; nested points are not supported by one
			 *          thread. Under MS Windows the mapping beyond the end of the file is
			 *          zero-filled and there is no signal — the class is empty there
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Bus {
				private:
					// Флаг регистрации точки восстановления этим объектом
					bool _armed;
				public:
					/**
					 * \~russian
					 * @brief Конструктор: регистрирует буфер точки возврата кадра вызывающей функции
					 *
					 * @details Обработчик SIGBUS ставится и без запущенного наблюдателя
					 *          сигналов: чтение из проекции обязано отказывать честно и в
					 *          приложении, которое сигнальный модуль не завело. Ставится тот
					 *          же обработчик, что и наблюдателю, поэтому точка восстановления
					 *          работает и поверх него. Сам вызов sigsetjmp выполняет
					 *          вызывающий, буфер sigjmp_buf объявляется рядом и передаётся
					 *          сюда: прыжок из обработчика возвращается в живой кадр, а не в
					 *          мёртвый кадр конструктора-помощника. Повторное конструирование
					 *          после прыжка безвредно: регистрация идемпотентна
					 *
					 * @param point буфер точки возврата, объявленный в кадре вызывающей функции
					 *
					 * \~english
					 * @brief Constructor: registers the return point buffer of the calling function frame
					 *
					 * @details The SIGBUS handler is installed even without the started
					 *          signal watcher: a read from a mapping must refuse honestly in
					 *          an application that never started the signals module. The same
					 *          handler as the watcher's is installed, so the recovery point
					 *          works on top of it too. The sigsetjmp call itself is performed
					 *          by the caller, the sigjmp_buf is declared next to it and passed
					 *          here: the jump from the handler returns into a live frame, not
					 *          into the dead frame of a helper constructor. A repeated
					 *          construction after the jump is harmless: the registration is
					 *          idempotent
					 *
					 * @param point return point buffer declared in the frame of the calling function
					 *
					 * \~
					 */
					explicit Bus(sigjmp_buf & point) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор: снимает регистрацию точки восстановления
					 *
					 * \~english
					 * @brief Destructor: removes the recovery point registration
					 *
					 * \~
					 */
					~Bus() noexcept;
			} bus_t;
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * \~russian
				 * @brief Структура событий сигналов
				 *
				 * @details Для операционной системы не являющейся MS Windows используется структура sigaction для установки обработчика сигнала,
				 *          которая позволяет передать контекст в обработчик сигнала.
				 *
				 * \~english
				 * @brief Structure of the signal events
				 * @details For an operating system other than MS Windows the sigaction structure is used to set the signal handler,
				 *          which allows passing the context into the signal handler.
				 *
				 * \~
				 */
				typedef struct Events {
					// Перехватчик сигнала SIGINT
					struct sigaction sigint;
					// Перехватчик сигнала SIGFPE
					struct sigaction sigfpe;
					// Перехватчик сигнала SIGILL
					struct sigaction sigill;
					// Перехватчик сигнала SIGBUS
					struct sigaction sigbus;
					// Перехватчик сигнала SIGABRT
					struct sigaction sigabrt;
					// Перехватчик сигнала SIGTERM
					struct sigaction sigterm;
					// Перехватчик сигнала SIGSEGV
					struct sigaction sigsegv;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Events() noexcept = default;
				} events_t;
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				/**
				 * \~russian
				 * @brief Устанавливаем прототип функции обработчика сигнала
				 *
				 * \~english
				 * @brief Set the prototype of the signal handler function
				 *
				 * \~
				 */
				typedef void (* SignalHandlerPointer)(int32_t);

				/**
				 * \~russian
				 * @brief Структура событий сигналов
				 *
				 * @details Для операционной системы MS Windows используется функция signal() для установки обработчика сигнала,
				 *          которая возвращает указатель на предыдущий обработчик сигнала.
				 *
				 * @note Намеренное решение: под MS Windows перехват разведён надвое, и
				 *       обработчики эти - лишь одна его половина. Обработчики SIGFPE,
				 *       SIGILL и SIGSEGV у библиотеки времени исполнения MS Windows
				 *       **потоковые**: поставленный одним потоком, у другого он не
				 *       действует вовсе, и отказ, случившийся в рабочем потоке
				 *       приложения, валил бы процесс молча, не позвав функции обратного
				 *       вызова. Проверено опытом пробой вне библиотеки - SIGFPE,
				 *       поднятый в чужом потоке, завершал процесс с кодом 3
				 *
				 *       Оттого настоящие отказы оборудования ловятся не сигналами, а
				 *       перехватчиком структурных исключений, ставящимся на весь процесс
				 *       через AddVectoredExceptionHandler. Обработчики же signal
				 *       остаются: ими ловится поднятое самим приложением через raise,
				 *       чего перехватчик исключений не видит и видеть не должен -
				 *       raise отказом оборудования не является
				 *
				 * \~english
				 * @brief Structure of the signal events
				 * @details For the MS Windows operating system the signal() function is used to set the signal handler,
				 *          which returns a pointer to the previous signal handler.
				 * @note A deliberate decision: under MS Windows the interception is split in two, and
				 *       those handlers are only one half of it. The SIGFPE,
				 *       SIGILL and SIGSEGV handlers of the MS Windows runtime library
				 *       are **per-thread**: one set by one thread does not take effect
				 *       in another at all, and a fault that happened in a worker thread
				 *       of the application would bring the process down silently, without calling the callback
				 *       function. Checked by experience by a trial outside the library — SIGFPE
				 *       raised in a foreign thread terminated the process with code 3
				 *       That is why the real hardware faults are caught not by signals but by
				 *       the structured exception handler, set for the whole process
				 *       through AddVectoredExceptionHandler. The signal handlers, on the other hand,
				 *       remain: they catch what is raised by the application itself through raise,
				 *       which the exception handler does not see and must not see —
				 *       raise is not a hardware fault
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Events {
					// Перехватчик сигнала SIGINT
					SignalHandlerPointer sigint;
					// Перехватчик сигнала SIGFPE
					SignalHandlerPointer sigfpe;
					// Перехватчик сигнала SIGILL
					SignalHandlerPointer sigill;
					// Перехватчик сигнала SIGABRT
					SignalHandlerPointer sigabrt;
					// Перехватчик сигнала SIGTERM
					SignalHandlerPointer sigterm;
					// Перехватчик сигнала SIGSEGV
					SignalHandlerPointer sigsegv;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Events() noexcept;
				} events_t;
			#endif
		private:
			// Объект работы с событиями сигналов
			events_t _events;
		private:
			// Флаг запуска отслеживания сигналов
			atomic_bool _mode;
			// Флаг запроса остановки рабочего потока
			atomic_bool _exit;
		private:
			// Рабочий поток для асинхронной обработки полученных сигналов
			std::thread _worker;
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Дескрипторы самопайпа: [0] - чтение, [1] - запись
				int32_t _pipe[2];
				// Запасной стек обработчика сбоев, нужный при срыве основного
				stack_t _stack;
				/**
				 * Прежнее расположение сигнала SIGPIPE, сохранённое при запуске
				 *
				 * @note Настройка эта общая на весь процесс и переживала бы остановку,
				 *       навязывая чужому коду изменённое поведение. Оттого прежнее
				 *       расположение запоминается запуском и возвращается снятием
				 */
				struct sigaction _pipesig;
			#endif
		private:
			/**
			 * @brief Мьютекс защиты операций запуска/останова/установки колбэка
			 *
			 * @note Замок берётся состоянием блокировок фреймворка, а не голым
			 *       `std::mutex`: фреймворк ветвится, а голый мьютекс достаётся
			 *       потомку в том виде, в каком его застало ветвление, - захваченный
			 *       чужим потоком, он остаётся захваченным навсегда. Состояние
			 *       блокировок помнит номер процесса и заводит мьютекс заново
			 *
			 * @note Отключать блокировки здесь НЕЛЬЗЯ, и переключателя безопасности
			 *       потоков у модуля нет намеренно: перехватчик сигналов и рабочий
			 *       поток - разные потоки по самому устройству модуля, а не по выбору
			 *       потребителя
			 *
			 */
			lock_state_t <std::mutex> _mtx;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова при получении сигнала
			 *
			 * @param sig номер полученного сигнала
			 *
			 * \~english
			 * @brief Callback function on the receipt of a signal
			 * @param sig number of the received signal
			 *
			 * \~
			 */
			function <void (const int32_t)> _callback;
		private:
			/**
			 * \~russian
			 * @brief Метод восстановления обработчиков сигналов по умолчанию
			 *
			 * \~english
			 * @brief Method of restoring the default signal handlers
			 *
			 * \~
			 */
			void disarm() noexcept;
			/**
			 * \~russian
			 * @brief Метод рабочего потока асинхронной обработки сигналов
			 *
			 * \~english
			 * @brief Method of the worker thread of the asynchronous signal handling
			 *
			 * \~
			 */
			void worker() noexcept;
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * \~russian
				 * @brief Метод обработки полученного сигнала вне контекста обработчика
				 *
				 * @param sig  номер полученного сигнала
				 * @param pid  идентификатор процесса-отправителя
				 * @param uid  идентификатор пользователя-отправителя
				 * @param addr адрес обращения, вызвавшего сбой
				 *
				 * \~english
				 * @brief Method of handling a received signal outside the context of the handler
				 * @param sig  number of the received signal
				 * @param pid  identifier of the sending process
				 * @param uid  identifier of the sending user
				 * @param addr address of the access that caused the fault
				 *
				 * \~
				 */
				void process(const int32_t sig, const pid_t pid, const uid_t uid, void * addr) noexcept;
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				/**
				 * \~russian
				 * @brief Метод обработки полученного сигнала вне контекста обработчика
				 *
				 * @param sig номер полученного сигнала
				 *
				 * \~english
				 * @brief Method of handling a received signal outside the context of the handler
				 * @param sig number of the received signal
				 *
				 * \~
				 */
				void process(const int32_t sig) noexcept;
			#endif
		public:
			/**
			 * \~russian
			 * @brief Метод остановки обработки сигналов
			 *
			 * @warning Останавливается перехват СЕМИ сигналов, восстанавливаемых по
			 *          умолчанию. Заглушение `SIGPIPE`, выставленное запуском, остановкой
			 *          НЕ снимается и переживает её: настройка эта общая на весь процесс
			 *
			 *
			 * \~english
			 * @brief Method of stopping the signal handling
			 *
			 * \~
			 */
			void stop() noexcept;
			/**
			 * \~russian
			 * @brief Метод запуска обработки сигналов
			 *
			 * \~english
			 * @brief Method of starting the signal handling
			 *
			 * \~
			 */
			void start() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова, которая должна сработать при получении сигнала
			 *
			 * @param callback функция обратного вызова
			 *
			 * \~english
			 * @brief Method of setting the callback function that must fire on the receipt of a signal
			 * @param callback callback function
			 *
			 * \~
			 */
			void on(function <void (const int32_t)> callback) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit Signals() noexcept;
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
			~Signals() noexcept;
	} signals_t;
};
