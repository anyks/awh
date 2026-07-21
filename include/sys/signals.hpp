/**
 * @file: signals.hpp
 * @date: 2026-01-26
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с сигналами
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Signals {
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * @brief Структура событий сигналов
				 *
				 * @details Для операционной системы не являющейся MS Windows используется структура sigaction для установки обработчика сигнала,
				 *          которая позволяет передать контекст в обработчик сигнала.
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
					 * @brief Конструктор
					 *
					 */
					explicit Events() noexcept = default;
				} events_t;
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				/**
				 * @brief Устанавливаем прототип функции обработчика сигнала
				 *
				 */
				typedef void (* SignalHandlerPointer)(int32_t);

				/**
				 * @brief Структура событий сигналов
				 *
				 * @details Для операционной системы MS Windows используется функция signal() для установки обработчика сигнала,
				 *          которая возвращает указатель на предыдущий обработчик сигнала.
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
					 * @brief Конструктор
					 *
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
			#endif
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Функция обратного вызова при получении сигнала
			 *
			 * @param sig номер полученного сигнала
			 */
			function <void (const int32_t)> _callback;
		private:
			/**
			 * @brief Метод восстановления обработчиков сигналов по умолчанию
			 *
			 */
			void disarm() noexcept;
			/**
			 * @brief Метод рабочего потока асинхронной обработки сигналов
			 *
			 */
			void worker() noexcept;
		private:
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * @brief Метод обработки полученного сигнала вне контекста обработчика
				 *
				 * @param sig номер полученного сигнала
				 * @param pid идентификатор процесса-отправителя
				 * @param uid идентификатор пользователя-отправителя
				 */
				void process(const int32_t sig, const pid_t pid, const uid_t uid) noexcept;
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				/**
				 * @brief Метод обработки полученного сигнала вне контекста обработчика
				 *
				 * @param sig номер полученного сигнала
				 */
				void process(const int32_t sig) noexcept;
			#endif
		public:
			/**
			 * @brief Метод остановки обработки сигналов
			 *
			 */
			void stop() noexcept;
			/**
			 * @brief Метод запуска обработки сигналов
			 *
			 */
			void start() noexcept;
		public:
			/**
			 * @brief Метод установки функции обратного вызова, которая должна сработать при получении сигнала
			 *
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const int32_t)> callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Signals(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Signals() noexcept;
	} signals_t;
};

#endif // __AWH_SIGNALS__
