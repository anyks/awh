/**
 * @file: event.hpp
 * @date: 2025-09-16
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

#ifndef __AWH_EVENT__
#define __AWH_EVENT__

/**
 * Наши модули
 */
#include "base.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Event Класс события AWHEvent
	 */
	typedef class Event {
		public:
			/**
			 * Типы события
			 */
			enum class type_t : uint8_t {
				NONE  = 0x00, // Тип события не установлен
				EVENT = 0x01, // Тип события обычное
				TIMER = 0x02  // Тип события таймер
			};
		private:
			// Режим активации события
			bool _mode;
		private:
			// Флаг активации серийного таймера
			bool _series;
		private:
			// Файловый дескриптор
			SOCKET _fd;
		private:
			// Идентификатор события
			uint64_t _id;
		private:
			// Тип события таймера
			type_t _type;
		private:
			// Задержка времени таймера
			uint32_t _delay;
		private:
			// Мютекс для блокировки потока
			std::recursive_mutex _mtx;
		private:
			// Функция обратного вызова
			base_t::callback_t _callback;
		private:
			// База данных событий
			base_t * _base;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * type Метод получения типа события
			 * @return установленный тип события
			 */
			type_t type() const noexcept;
		public:
			/**
			 * set Метод установки базы событий
			 * @param base база событий для установки
			 */
			void set(base_t * base) noexcept;
			/**
			 * set Метод установки файлового дескриптора
			 * @param fd файловый дескриптор для установки
			 */
			void set(const SOCKET fd) noexcept;
			/**
			 * set Метод установки функции обратного вызова
			 * @param callback функция обратного вызова
			 */
			void set(base_t::callback_t callback) noexcept;
		public:
			/**
			 * Метод удаления типа события
			 * @param type тип события для удаления
			 */
			void del(const base_t::event_type_t type) noexcept;
		public:
			/**
			 * timeout Метод установки задержки времени таймера
			 * @param delay  задержка времени в миллисекундах
			 * @param series флаг серийного таймаута
			 */
			void timeout(const uint32_t delay, const bool series = false) noexcept;
		public:
			/**
			 * mode Метод установки режима работы модуля
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const base_t::event_type_t type, const base_t::event_mode_t mode) noexcept;
		public:
			/**
			 * stop Метод остановки работы события
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска работы события
			 */
			void start() noexcept;
		public:
			/**
			 * Оператор [=] для установки базы событий
			 * @param base база событий для установки
			 * @return     текущий объект
			 */
			Event & operator = (base_t * base) noexcept;
			/**
			 * Оператор [=] для установки файлового дескриптора
			 * @param fd файловый дескриптор для установки
			 * @return   текущий объект
			 */
			Event & operator = (const SOCKET fd) noexcept;
			/**
			 * Оператор [=] для установки задержки времени таймера
			 * @param delay задержка времени в миллисекундах
			 * @return      текущий объект
			 */
			Event & operator = (const uint32_t delay) noexcept;
			/**
			 * Оператор [=] для установки функции обратного вызова
			 * @param callback функция обратного вызова
			 * @return         текущий объект
			 */
			Event & operator = (base_t::callback_t callback) noexcept;
		public:
			/**
			 * Event Конструктор
			 * @param type тип события
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			Event(const type_t type, const fmk_t * fmk, const log_t * log) noexcept :
			 _mode(false), _series(false), _fd(INVALID_SOCKET), _id(0), _type(type),
			 _delay(0), _callback(nullptr), _base(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~Event Деструктор
			 */
			~Event() noexcept;
	} event_t;
};

#endif // __AWH_EVENT__
