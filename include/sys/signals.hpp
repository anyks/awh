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
 * Стандартные модули
 */
#include <atomic>
#include <cstdlib>
#include <csignal>
#include <functional>

/**
 * Подключаем наши модули
 */
#include "fmk.hpp"
#include "log.hpp"

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системные модули
	 */
	#include <tchar.h>
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
				 */
				typedef struct Events {
					struct sigaction sigint;  // Перехватчик сигнала SIGINT
					struct sigaction sigfpe;  // Перехватчик сигнала SIGFPE
					struct sigaction sigill;  // Перехватчик сигнала SIGILL
					struct sigaction sigbus;  // Перехватчик сигнала SIGBUS
					struct sigaction sigabrt; // Перехватчик сигнала SIGABRT
					struct sigaction sigterm; // Перехватчик сигнала SIGTERM
					struct sigaction sigsegv; // Перехватчик сигнала SIGSEGV
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Events() noexcept {}
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
				 */
				typedef struct Events {
					SignalHandlerPointer sigint;  // Перехватчик сигнала SIGINT
					SignalHandlerPointer sigfpe;  // Перехватчик сигнала SIGFPE
					SignalHandlerPointer sigill;  // Перехватчик сигнала SIGILL
					SignalHandlerPointer sigabrt; // Перехватчик сигнала SIGABRT
					SignalHandlerPointer sigterm; // Перехватчик сигнала SIGTERM
					SignalHandlerPointer sigsegv; // Перехватчик сигнала SIGSEGV
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Events() noexcept {}
				} events_t;
			#endif
		private:
			// Объект работы с событиями сигналов
			events_t _events;
		private:
			// Флаг запуска отслежиявания сигналов
			atomic_bool _mode;
		private:
			/**
			 * Функция обратного вызова при получении сигнала
			 */
			function <void (const int32_t)> _callback;
		private:
			/**
			 * @brief Функция обратного вызова
			 *
			 * @param sid идентификатор сигнала
			 */
			void callback(const int32_t sid) noexcept;
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
