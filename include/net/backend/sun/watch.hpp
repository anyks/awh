/**
 * @file: watch.hpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля системных часов цикла событий — класс Watch,
 *        обеспечивающий отсчёт таймеров в отдельном потоке для платформ,
 *        где нативный механизм таймеров цикла событий недоступен
 *
 * @copyright: Copyright © 2025
 *
 */

#ifndef __AWH_EVENT_WATCH__
#define __AWH_EVENT_WATCH__

/**
 * Стандартные заголовочные файлы
 */
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "notifier.hpp"
#include "../sys/locker.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс для работы с часами
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Watch {
		private:
			/**
			 * Таймаут блокировки времени по умолчанию (100ms)
			 */
			static constexpr const uint64_t TIMEOUT = 0x5F5E100;
		private:
			// Объект дочернего потока
			std::thread _thr;
			// Условная переменная, ожидания поступления данных
			std::condition_variable _cv;
		private:
			// Флаг работающего модуля
			std::atomic_bool _working;
		private:
			// Мютекс для блокировки потока
			lock_state_t <std::mutex> _mtx;
			// Мютекс ожидания данных
			lock_state_t <std::mutex> _locker;
		private:
			// Таймаут ожидания блокировки базы событий
			std::chrono::nanoseconds _delay;
		private:
			// Объект работы с уведомителем
			notifier_t _notifier;
		private:
			// Список активных таймеров
			std::multimap <uint64_t, uint32_t> _timers;
			// Очередь таймеров ожидающих активацию
			std::queue <std::pair <uint32_t, uint64_t>> _items;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод обработки событий триггера
			 *
			 */
			void trigger() noexcept;
			/**
			 * @brief Метод получения данных
			 *
			 */
			void receiving() noexcept;
		public:
			/**
			 * @brief Метод остановки работы таймера
			 *
			 * @return результат работы функции
			 *
			 */
			bool stop() noexcept;
			/**
			 * @brief Метод запуска работы таймера
			 *
			 * @return результат работы функции
			 *
			 */
			bool start() noexcept;
		public:
			/**
			 * @brief Метод создания нового таймера
			 *
			 * @return файловый дескриптор для отслеживания
			 *
			 */
			SOCKET create() noexcept;
		public:
			/**
			 * @brief Метод извлечения идентификатора события
			 *
			 * @return идентификатор события
			 *
			 */
			uint32_t event() noexcept;
		public:
			/**
			 * @brief Метод убрать таймер из отслеживания
			 *
			 * @param id идентификатор таймера
			 *
			 */
			void away(const uint32_t id) noexcept;
			/**
			 * @brief Метод ожидания указанного промежутка времени
			 *
			 * @param id    идентификатор таймера
			 * @param delay задержка времени в миллисекундах
			 *
			 */
			void wait(const uint32_t id, const uint32_t delay) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Watch(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Watch() noexcept;
	} watch_t;
};

#endif // __AWH_EVENT_WATCH__
