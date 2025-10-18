/**
 * @file: events.hpp
 * @date: 2025-10-18
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


#ifndef __AWH_EVENTS__
#define __AWH_EVENTS__

/**
 * Стандартные модули
 */
#include <unordered_set>

/**
 * Наши модули
 */
#include "reactor.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип базы событий
	 *
	 */
	class Base;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс события Events Loop
	 *
	 */
	typedef class AWH_SHARED_EXPORT Events {
		public:
			/**
			 * Тип режима получения события
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x01, // Разрешено получение события
				DISABLED = 0x00  // Запрещено получение события
			};
			/**
			 * Тип активного события
			 */
			enum class type_t : uint8_t {
				NONE     = 0x00, // Тип активного события не установлено
				CLOSE    = 0x01, // Событие закрытия подключения
				READ     = 0x02, // Событие доступности данных на чтение
				WRITE    = 0x03, // Событие доступности сокета на запись
				TIMER    = 0x04, // Событие таймера в миллисекундах
				INTERVAL = 0x05  // Событие интервала в миллисекундах
			};
		public:
			/**
			 * Тип функции обратного вызова при получении событий сокета
			 */
			using callback_t = function <void (const SOCKET, const type_t)>;
		private:
			// Файловый дескриптор
			SOCKET _sock;
		private:
			// Идентификатор событий
			uint32_t _id;
		private:
			// Задержка времени таймера
			uint32_t _delay;
		private:
			// Установленные события
			uint16_t _events;
		private:
			// Мютекс для блокировки потока
			std::mutex _mtx;
		private:
			// Флаг активного ожидания событий
			std::atomic_bool _awaiting;
		private:
			// Функция обратного вызова
			callback_t _callback;
		private:
			// База данных событий
			Base * _base;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод проверки находятся ли событии в ожидании
			 * 
			 * @return результат проверки
			 */
			bool awaiting() const noexcept;
		public:
			/**
			 * @brief Метод получения типов установленных соыбтий
			 *
			 * @return установленные типы событий
			 */
			std::unordered_set <type_t> types() const noexcept;
		public:
			/**
			 * @brief Метод установки базы событий
			 *
			 * @param base база событий для установки
			 * @return     результат выполнения установки
			 */
			bool set(Base * base) noexcept;
			/**
			 * @brief Метод установки файлового дескриптора
			 *
			 * @param sock сетевой сокет для установки
			 * @return     результат выполнения установки
			 */
			bool set(const SOCKET sock) noexcept;
			/**
			 * @brief Метод установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова
			 * @return         результат выполнения установки
			 */
			bool set(callback_t callback) noexcept;
			/**
			 * @brief Метод для установки задержки времени таймера
			 *
			 * @param delay задержка времени в миллисекундах
			 * @return      результат выполнения установки
			 */
			bool set(const uint32_t delay) noexcept;
		public:
			/**
			 * @brief Метод установки режима работы модуля
			 *
			 * @param type тип событий модуля для которого требуется сменить режим работы
			 * @param mode флаг режима работы модуля
			 * @return     результат работы функции
			 */
			bool mode(const type_t type, const mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод остановки работы события
			 *
			 * @return результат работы функции
			 */
			bool stop() noexcept;
			/**
			 * @brief Метод запуска работы события
			 *
			 * @return результат работы функции
			 */
			bool start() noexcept;
		public:
			/**
			 * @brief Метод обмена данными между событиями
			 * 
			 * @param events объект событий для обмена
			 */
			void swap(Events && events) noexcept;
		public:
			/**
			 * @brief Оператор для установки базы событий
			 *
			 * @param base база событий для установки
			 * @return     текущий объект
			 */
			Events & operator = (Base * base) noexcept;
			/**
			 * @brief Оператор для установки файлового дескриптора
			 *
			 * @param sock сетевой сокет для установки
			 * @return     текущий объект
			 */
			Events & operator = (const SOCKET sock) noexcept;
			/**
			 * @brief Оператор для установки функции обратного вызова
			 *
			 * @param callback функция обратного вызова
			 * @return         текущий объект
			 */
			Events & operator = (callback_t callback) noexcept;
			/**
			 * @brief Оператор для установки задержки времени таймера
			 *
			 * @param delay задержка времени в миллисекундах
			 * @return      текущий объект
			 */
			Events & operator = (const uint32_t delay) noexcept;
		public:
			/**
			 * @brief Оператор перемещения объекта событий
			 * 
			 * @param events объект событий для перемещения
			 * @return       текущий объект
			 */
			Events & operator = (Events && events) noexcept;
			/**
			 * @brief Оператор копирования объекта событий
			 * 
			 * @param events объект событий для копирования
			 * @return       текущий объект
			 */
			Events & operator = (const Events & events) noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 * 
			 * @param events объект событий для перемещения
			 */
			Events(Events && events) noexcept;
			/**
			 * @brief Конструктор копирования
			 * 
			 * @param events объект событий для копирования
			 */
			Events(const Events & events) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Events(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Events() noexcept;
	} events_t;
};

#endif // __AWH_EVENTS__
