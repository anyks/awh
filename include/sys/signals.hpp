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
#ifndef __AWH_SIGNALS__
#define __AWH_SIGNALS__

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
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"
#include "log.hpp"

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
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
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
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
			// Мьютекс защиты операций запуска/останова/установки колбэка
			std::mutex _mtx;
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
			#if !_WIN32 && !_WIN64
				// Дескрипторы самопайпа: [0] - чтение, [1] - запись
				int32_t _pipe[2];
				// Запасной стек обработчика сбоев, нужный при срыве основного
				stack_t _stack;
			#endif
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
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
			#if !_WIN32 && !_WIN64
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
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Signals(const fmk_t * fmk, const log_t * log) noexcept;
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

#endif // __AWH_SIGNALS__
