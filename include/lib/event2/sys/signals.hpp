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
 * Стандартная библиотека
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
 * Стандартная библиотека
 */
#include <event2/event.h>

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
					struct event * sint;  // Перехватчик сигнала SIGINT
					struct event * sfpe;  // Перехватчик сигнала SIGFPE
					struct event * sill;  // Перехватчик сигнала SIGILL
					struct event * sterm; // Перехватчик сигнала SIGTERM
					struct event * sabrt; // Перехватчик сигнала SIGABRT
					struct event * ssegv; // Перехватчик сигнала SIGSEGV
					/**
					 * Events Конструктор
					 */
					Events() noexcept : sint(nullptr), sfpe(nullptr), sill(nullptr), sterm(nullptr), sabrt(nullptr), ssegv(nullptr) {}
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
					SignalHandlerPointer sint;  // Перехватчик сигнала SIGINT
					SignalHandlerPointer sfpe;  // Перехватчик сигнала SIGFPE
					SignalHandlerPointer sill;  // Перехватчик сигнала SIGILL
					SignalHandlerPointer sabrt; // Перехватчик сигнала SIGABRT
					SignalHandlerPointer sterm; // Перехватчик сигнала SIGTERM
					SignalHandlerPointer ssegv; // Перехватчик сигнала SIGSEGV
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
			struct event_base * _base;
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
				 * intCallback Функция обработки информационных сигналов SIGINT
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void intCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * fpeCallback Функция обработки информационных сигналов SIGFPE
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void fpeCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * illCallback Функция обработки информационных сигналов SIGILL
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void illCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * termCallback Функция обработки информационных сигналов SIGTERM
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void termCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * abrtCallback Функция обработки информационных сигналов SIGABRT
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void abrtCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
				/**
				 * segvCallback Функция обработки информационных сигналов SIGSEGV
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 * @param ctx   объект сервера
				 */
				static void segvCallback(evutil_socket_t fd, short event, void * ctx) noexcept;
			#endif
		private:
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				/**
				 * clear Метод очистки сигнала
				 * @param signal сигнал для очистки
				 */
				void clear(struct event ** signal) noexcept;
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
			void base(struct event_base * base) noexcept;
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
			Signals(struct event_base * base) noexcept : _mode(false), _fn(nullptr), _base(base) {}
			/**
			 * ~Signals Деструктор
			 */
			~Signals() noexcept;
	} sig_t;
};

#endif // __AWH_SIGNALS__
