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
#include <set>
#include <mutex>

/**
 * Наши модули
 */
#include <sys/screen.hpp>
#include <net/socket.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Timeout Класс для работы с таймером в экране
	 */
	typedef class Timeout {
		private:
			/**
			 * Data структура обмена данными
			 */
			typedef struct Data {
				// Файловый дескрипторв (сокет)
				SOCKET fd;
				// Время задержки работы таймера
				time_t delay;
				/**
				 * Data Конструктор
				 */
				Data() noexcept : fd(INVALID_SOCKET), delay(0) {}
			} __attribute__((packed)) data_t;
		private:
			// Мютекс для блокировки потока
			mutex _mtx;
		private:
			// Контрольная точка даты
			time_t _date;
		private:
			// Объект для работы с сокетами
			socket_t _socket;
		private:
			// Объект экрана для работы в дочернем потоке
			screen_t <data_t> _screen;
		private:
			// Список существующих таймеров
			std::set <SOCKET> _ids;
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
			 */
			void set(const SOCKET fd, const time_t delay) noexcept;
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
