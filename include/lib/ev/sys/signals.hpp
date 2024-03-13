/**
 * @file: signals.hpp
 * @date: 2022-08-07
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

#ifndef __AWH_SIGNALS__
#define __AWH_SIGNALS__

/**
 * Стандартные модули
 */
#include <string>
#include <thread>
#include <cstdlib>
#include <csignal>

/**
 * Для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
	#include <tchar.h>
#endif

/**
 * Стандартные модули
 */
#include <libev/ev++.h>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Signals Класс работы с сигналами
	 */
	typedef class Signals {
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * Event Структура событий сигналов
				 */
				typedef struct Events {
					ev::sig sigint;  // Перехватчик сигнала SIGINT
					ev::sig sigfpe;  // Перехватчик сигнала SIGFPE
					ev::sig sigill;  // Перехватчик сигнала SIGILL
					ev::sig sigterm; // Перехватчик сигнала SIGTERM
					ev::sig sigabrt; // Перехватчик сигнала SIGABRT
					ev::sig sigsegv; // Перехватчик сигнала SIGSEGV
				} ev_t;
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Устанавливаем прототип функции обработчика сигнала
				typedef void (* SignalHandlerPointer)(int);
				/**
				 * Events Структура событий сигналов
				 */
				typedef struct Events {
					SignalHandlerPointer sigint;  // Перехватчик сигнала SIGINT
					SignalHandlerPointer sigfpe;  // Перехватчик сигнала SIGFPE
					SignalHandlerPointer sigill;  // Перехватчик сигнала SIGILL
					SignalHandlerPointer sigterm; // Перехватчик сигнала SIGTERM
					SignalHandlerPointer sigabrt; // Перехватчик сигнала SIGABRT
					SignalHandlerPointer sigsegv; // Перехватчик сигнала SIGSEGV
				} ev_t;
			#endif
		private:
			// Флаг запуска отслежиявания сигналов
			bool _mode;
		private:
			// Объект работы с сигналами
			ev_t _ev;
		private:
			// Функция обратного вызова при получении сигнала
			function <void (const int)> _fn;
		private:
			// Объект работы с базой событий
			struct ev_loop * _base;
		private:
			/**
			 * callback Функция обратного вызова
			 * @param sig идентификатор сигнала
			 */
			void callback(const int sig) noexcept;
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * signal Функция обратного вызова при возникновении сигнала
				 * @param watcher объект события сигнала
				 * @param revents идентификатор события
				 */
				void signal(ev::sig & watcher, int revents) noexcept;
			#endif
		public:
			/**
			 * stop Метод остановки обработки сигналов
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска обработки сигналов
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
			 * on Метод установки функции обратного вызова, которая должна сработать при получении сигнала
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const int)> callback) noexcept;
		public:
			/**
			 * Signals Конструктор
			 * @param base база событий
			 */
			Signals(struct ev_loop * base) noexcept :
			 _mode(false), _fn(nullptr), _base(base) {}
	} sig_t;
};

#endif // __AWH_SIGNALS__
