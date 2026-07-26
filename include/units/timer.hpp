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
 * @brief Заголовочный файл модуля таймера —
 *        класс unit::Timer для создания одиночных и периодических таймеров цикла событий с миллисекундным разрешением
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модулей
	 *
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс узла таймера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Timer : public unit_t {
			private:
				// Список идентификаторов событий таймеров
				unordered_set <event::id_t> _timers;
			private:
				/**
				 * @brief Метод обновления состояния таймера
				 *
				 * @param eid    идентификатор таймера
				 * @param status новый статус таймера
				 *
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			public:
				/**
				 * @brief Метод очистки всех таймеров
				 *
				 */
				void clear() noexcept;
				/**
				 * @brief Метод очистки таймера
				 *
				 * @param eid идентификатор таймера для очистки
				 *
				 */
				void clear(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод создания таймаута
				 *
				 * @param delay задержка времени в миллисекундах
				 * @return      идентификатор таймера
				 *
				 */
				event::id_t timeout(const uint32_t delay) noexcept;
				/**
				 * @brief Метод создания интервала
				 *
				 * @param delay задержка времени в миллисекундах
				 * @return      идентификатор таймера
				 *
				 */
				event::id_t interval(const uint32_t delay) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Timer(const Timer &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Timer & operator = (const Timer &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Timer(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Timer() noexcept;
		} timer_t;
	};
};

#endif // __AWH_UNIT_TIMER__
