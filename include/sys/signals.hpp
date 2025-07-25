/**
 * @file: signals.hpp
 * @date: 2024-07-06
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

#ifndef __AWH_SIGNALS__
#define __AWH_SIGNALS__

/**
 * Стандартные модули
 */
#include <string>
#include <thread>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <iostream>
#include <functional>

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	#include <windows.h>
	#include <tchar.h>
#endif

/**
 * Подключаем наши модули
 */
#include "fmk.hpp"
#include "log.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Signals Класс работы с сигналами
	 */
	typedef class AWHSHARED_EXPORT Signals {
		private:
			/**
			 * Для операционной системы не являющейся OS Windows
			 */
			#if !_WIN32 && !_WIN64
				/**
				 * Event Структура событий сигналов
				 */
				typedef struct Events {
					struct sigaction sigInt;  // Перехватчик сигнала SIGINT
					struct sigaction sigFpe;  // Перехватчик сигнала SIGFPE
					struct sigaction sigIll;  // Перехватчик сигнала SIGILL
					struct sigaction sigBus;  // Перехватчик сигнала SIGBUS
					struct sigaction sigAbrt; // Перехватчик сигнала SIGABRT
					struct sigaction sigTerm; // Перехватчик сигнала SIGTERM
					struct sigaction sigSegv; // Перехватчик сигнала SIGSEGV
					/**
					 * Events Конструктор
					 */
					Events() noexcept {}
				} ev_t;
			/**
			 * Для операционной системы OS Windows
			 */
			#else
				// Устанавливаем прототип функции обработчика сигнала
				typedef void (* SignalHandlerPointer)(int32_t);
				/**
				 * Events Структура событий сигналов
				 */
				typedef struct Events {
					SignalHandlerPointer sigInt;  // Перехватчик сигнала SIGINT
					SignalHandlerPointer sigFpe;  // Перехватчик сигнала SIGFPE
					SignalHandlerPointer sigIll;  // Перехватчик сигнала SIGILL
					SignalHandlerPointer sigAbrt; // Перехватчик сигнала SIGABRT
					SignalHandlerPointer sigTerm; // Перехватчик сигнала SIGTERM
					SignalHandlerPointer sigSegv; // Перехватчик сигнала SIGSEGV
					/**
					 * Events Конструктор
					 */
					Events() noexcept {}
				} ev_t;
			#endif
		private:
			// Объект работы с сигналами
			ev_t _ev;
		private:
			// Флаг запуска отслежиявания сигналов
			bool _mode;
		private:
			// Функция обратного вызова при получении сигнала
			function <void (const int32_t)> _callback;
		private:
			/**
			 * callback Функция обратного вызова
			 * @param sig идентификатор сигнала
			 */
			void callback(const int32_t sig) noexcept;
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
			 * on Метод установки функции обратного вызова, которая должна сработать при получении сигнала
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const int32_t)> callback) noexcept;
		public:
			/**
			 * Signals Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Signals(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Signals Деструктор
			 */
			~Signals() noexcept;
	} sig_t;
};

#endif // __AWH_SIGNALS__
