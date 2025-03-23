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
#include <map>
#include <mutex>

/**
 * Наши модули
 */
#include <sys/screen.hpp>
#include <events/evpipe.hpp>

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
				// Порт на который нужно отправить
				uint32_t port;
				// Время задержки работы таймера
				uint64_t delay;
				/**
				 * Data Конструктор
				 */
				Data() noexcept : fd(INVALID_SOCKET), port(0), delay(0) {}
			} __attribute__((packed)) data_t;
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Объект работы с пайпом
				evpipe_t _evpipe;
			/**
			 * Методы для всех остальных операционных систем
			 */
			#else
				// Объект работы с пайпом
				evpipe_t _evpipe;
			#endif
		private:
			// Мютекс для блокировки потока
			recursive_mutex _mtx;
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <data_t> _screen;
		private:
			// Список существующих файловых дескрипторов
			map <SOCKET, uint32_t> _fds;
			// Список активных таймеров
			multimap <uint64_t, SOCKET> _timers;
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
			 * set Метод установки таймера
			 * @param fd    файловый дескриптор таймера
			 * @param delay задержка времени в миллисекундах
			 * @param port  порт сервера на который нужно отправить ответ
			 */
			void set(const SOCKET fd, const uint32_t delay, const uint32_t port = 0) noexcept;
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
