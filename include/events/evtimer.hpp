/**
 * @file: evtimer.hpp
 * @date: 2024-07-03
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

#ifndef __AWH_EVENT_TIMER__
#define __AWH_EVENT_TIMER__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <mutex>

/**
 * Наши модули
 */
#include "evpipe.hpp"
#include "../sys/screen.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * EventTimer Класс для работы с таймером в экране
	 */
	typedef class AWHSHARED_EXPORT EventTimer {
		private:
			/**
			 * Data структура обмена данными
			 */
			typedef struct Data {
				// Файловый дескрипторв (сокет)
				SOCKET fd;
				// Время задержки работы таймера
				uint64_t delay;
				/**
				 * Data Конструктор
				 */
				Data() noexcept : fd(INVALID_SOCKET), delay(0) {}
			} __attribute__((packed)) data_t;
		private:
			// Объект работы с пайпом
			evpipe_t _evpipe;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <data_t> _screen;
		private:
			// Список существующих файловых дескрипторов
			std::set <SOCKET> _fds;
			// Список активных таймеров
			std::multimap <uint64_t, SOCKET> _timers;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * trigger Метод обработки событий триггера
			 */
			void trigger() noexcept;
			/**
			 * process Метод обработки процесса добавления таймеров
			 * @param data данные таймера для добавления
			 */
			void process(const data_t data) noexcept;
		public:
			/**
			 * stop Метод остановки работы таймера
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска работы таймера
			 */
			void start() noexcept;
		public:
			/**
			 * del Метод удаления таймера
			 * @param fd файловый дескриптор таймера
			 */
			void del(const SOCKET fd) noexcept;
			/**
			 * add Метод добавления таймера
			 * @param fd    файловый дескриптор таймера
			 * @param delay задержка времени в миллисекундах
			 */
			void add(const SOCKET fd, const uint32_t delay) noexcept;
		public:
			/**
			 * EventTimer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			EventTimer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~EventTimer Деструктор
			 */
			~EventTimer() noexcept;
	} evtimer_t;
};

#endif // __AWH_EVENT_TIMER__
