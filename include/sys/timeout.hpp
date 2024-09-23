/**
 * @file: timeout.hpp
 * @date: 2024-07-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_TIMEOUT__
#define __AWH_TIMEOUT__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>

/**
 * Наши модули
 */
#include <sys/pipe.hpp>
#include <sys/screen.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timeout Класс для работы с таймером в экране
	 */
	typedef class AWHSHARED_EXPORT Timeout {
		private:
			/**
			 * Data структура обмена данными
			 */
			typedef struct Data {
				// Файловый дескрипторв (сокет)
				SOCKET fd;
				// Время задержки работы таймера
				time_t delay;
				// Порт на который нужно отправить
				uint32_t port;
				/**
				 * Data Конструктор
				 */
				Data() noexcept : fd(INVALID_SOCKET), delay(0), port(0) {}
			} __attribute__((packed)) data_t;
		private:
			// Мютекс для блокировки потока
			mutex _mtx;
		private:
			// Контрольная точка даты
			time_t _date;
		private:
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Объект работы с пайпом
				pipe_t _pipe;
			/**
			 * Методы для всех остальных операционных систем
			 */
			#else
				// Объект работы с пайпом
				pipe_t _pipe;
			#endif
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <data_t> _screen;
		private:
			// Список существующих файловых дескрипторов
			std::map <SOCKET, uint32_t> _fds;
			// Список активных таймеров
			multimap <time_t, SOCKET> _timers;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
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
			 * @param delay задержка времени в наносекундах
			 * @param port  порт сервера на который нужно отправить ответ
			 */
			void set(const SOCKET fd, const time_t delay, const uint32_t port = 0) noexcept;
		public:
			/**
			 * Timeout Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Timeout(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Timeout Деструктор
			 */
			~Timeout() noexcept;
	} timeout_t;
};

#endif // __AWH_TIMEOUT__
