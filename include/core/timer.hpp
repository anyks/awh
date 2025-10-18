/**
 * @file: timer.hpp
 * @date: 2025-10-11
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

#ifndef __AWH_CORE_TIMER__
#define __AWH_CORE_TIMER__

/**
 * Наши модули
 */
#include "core.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс таймера ядра биндинга
	 *
	 */
	typedef class AWH_SHARED_EXPORT Timer : public awh::core_t {
		private:
			// Мютекс для блокировки основного потока
			std::mutex _mtx;
		private:
			// Хранилище функций обратного вызова
			callback_t _callback;
		private:
			// Список активных брокеров
			std::unordered_map <uint16_t, std::unique_ptr <events_t>> _events;
		private:
			/**
			 * @brief Метод генерации уникального идентификатора
			 * 
			 * @return уникальный идентификатор
			 */
			uint16_t identifier() const noexcept;
		private:
			/**
			 * @brief Метод вызова при активации базы событий
			 *
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void launching(const bool mode, const bool status) noexcept;
			/**
			 * @brief Метод вызова при деакцтивации базы событий
			 *
			 * @param mode   флаг работы с сетевым протоколом
			 * @param status флаг вывода события статуса
			 */
			void closedown(const bool mode, const bool status) noexcept;
		private:
			/**
			 * @brief Метод события таймера
			 *
			 * @param tid   идентификатор таймера
			 * @param sock  сетевой сокет
			 * @param event произошедшее событие
			 */
			void event(const uint16_t tid, const SOCKET sock, const events_t::type_t event) noexcept;
		public:
			/**
			 * @brief Метод очистки всех таймеров
			 *
			 */
			void clear() noexcept;
			/**
			 * @brief Метод очистки таймера
			 *
			 * @param tid идентификатор таймера для очистки
			 */
			void clear(const uint16_t tid) noexcept;
		public:
			/**
			 * @brief Метод создания таймаута
			 *
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t timeout(const uint32_t delay) noexcept;
			/**
			 * @brief Метод создания интервала
			 *
			 * @param delay задержка времени в миллисекундах
			 * @return      идентификатор таймера
			 */
			uint16_t interval(const uint32_t delay) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param tid  идентификатор таймера для которого устанавливается функция
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint16_t tid, Args... args) noexcept -> uint16_t {
				// Если мы получили название функции обратного вызова
				if(tid > 0)
					// Выполняем установку функции обратного вызова
					return static_cast <uint16_t> (this->_callback.on <void ()> (static_cast <uint32_t> (tid), args...));
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Timer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Timer() noexcept;
	} timer_t;
};

#endif // __AWH_CORE_TIMER__
