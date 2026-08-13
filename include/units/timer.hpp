/**
 * @file: timer.hpp
 * @date: 2026-02-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля таймера —
 *        класс unit::Timer для создания одиночных и периодических таймеров цикла событий с миллисекундным разрешением
 *
 * \~english
 * @brief Header file of the timer module —
 *        the unit::Timer class for creating one-shot and periodic event loop timers with millisecond resolution
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_TIMER__
#define __AWH_UNIT_TIMER__

/**
 * Стандартный заголовочный файл
 */
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс узла таймера
		 *
		 * \~english
		 * @brief Timer unit class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Timer : public unit_t {
			private:
				// Список идентификаторов событий таймеров
				unordered_set <event::id_t> _timers;
			private:
				/**
				 * \~russian
				 * @brief Метод обновления состояния таймера
				 *
				 * @param eid    идентификатор таймера
				 * @param status новый статус таймера
				 *
				 * \~english
				 * @brief Method of updating the timer state
				 * @param eid    timer identifier
				 * @param status new timer status
				 *
				 * \~
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки всех таймеров
				 *
				 * \~english
				 * @brief Method of clearing all timers
				 *
				 * \~
				 */
				void clear() noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки таймера
				 *
				 * @param eid идентификатор таймера для очистки
				 *
				 * \~english
				 * @brief Method of clearing a timer
				 * @param eid identifier of the timer to be cleared
				 *
				 * \~
				 */
				void clear(const event::id_t eid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания таймаута
				 *
				 * @param delay задержка времени в миллисекундах
				 * @return      идентификатор таймера
				 *
				 * \~english
				 * @brief Method of creating a timeout
				 * @param delay time delay in milliseconds
				 * @return      timer identifier
				 *
				 * \~
				 */
				event::id_t timeout(const uint32_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания интервала
				 *
				 * @param delay задержка времени в миллисекундах
				 * @return      идентификатор таймера
				 *
				 * \~english
				 * @brief Method of creating an interval
				 * @param delay time delay in milliseconds
				 * @return      timer identifier
				 *
				 * \~
				 */
				event::id_t interval(const uint32_t delay) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				Timer(const Timer &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				Timer & operator = (const Timer &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Timer(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Timer() noexcept;
		} timer_t;
	};
};

#endif // __AWH_UNIT_TIMER__
