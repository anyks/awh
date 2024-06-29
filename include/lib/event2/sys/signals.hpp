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
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <lib/event2/sys/events.hpp>

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
					event_t sigint;  // Перехватчик сигнала SIGINT
					event_t sigfpe;  // Перехватчик сигнала SIGFPE
					event_t sigill;  // Перехватчик сигнала SIGILL
					event_t sigbus;  // Перехватчик сигнала SIGBUS
					event_t sigterm; // Перехватчик сигнала SIGTERM
					event_t sigabrt; // Перехватчик сигнала SIGABRT
					event_t sigsegv; // Перехватчик сигнала SIGSEGV
					/**
					 * Events Конструктор
					 * @param log объект для работы с логами
					 */
					Events(const log_t * log) noexcept :
					 sigint(event_t::type_t::SIGNAL, log),
					 sigfpe(event_t::type_t::SIGNAL, log),
					 sigill(event_t::type_t::SIGNAL, log),
					 sigbus(event_t::type_t::SIGNAL, log),
					 sigterm(event_t::type_t::SIGNAL, log),
					 sigabrt(event_t::type_t::SIGNAL, log),
					 sigsegv(event_t::type_t::SIGNAL, log) {}
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
					/**
					 * Events Конструктор
					 * @param log объект для работы с логами
					 */
					Events(const log_t * log) noexcept {}
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
				 */
				void intCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * fpeCallback Функция обработки информационных сигналов SIGFPE
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void fpeCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * illCallback Функция обработки информационных сигналов SIGILL
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void illCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * busCallback Функция обработки информационных сигналов SIGBUS
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void busCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * termCallback Функция обработки информационных сигналов SIGTERM
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void termCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * abrtCallback Функция обработки информационных сигналов SIGABRT
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void abrtCallback(evutil_socket_t fd, short event) noexcept;
				/**
				 * segvCallback Функция обработки информационных сигналов SIGSEGV
				 * @param fd    файловый дескриптор (сокет)
				 * @param event возникшее событие
				 */
				void segvCallback(evutil_socket_t fd, short event) noexcept;
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
			 * @param log  объект для работы с логами
			 */
			Signals(struct event_base * base, const log_t * log) noexcept :
			 _mode(false), _ev(log), _fn(nullptr), _base(base) {}
			/**
			 * ~Signals Деструктор
			 */
			~Signals() noexcept;
	} sig_t;
};

#endif // __AWH_SIGNALS__
