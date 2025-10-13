/**
 * @file: guard.hpp
 * @date: 2025-10-13
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

#ifndef __AWH_CORE_GUARD__
#define __AWH_CORE_GUARD__

/**
 * Стандартные модули
 */
#include <set>
#include <mutex>
#include <atomic>
#include <cinttypes>

/**
 * Наши модули
 */
#include "../sys/global.hpp"

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
	 * @brief Класс сторожа защиты доступа к функциям
	 *
	 */
	typedef class AWH_SHARED_EXPORT Guard {
		public:
			/**
			 * @brief Класс работы локера доступа
			 * 
			 */
			typedef class AWH_SHARED_EXPORT Locker {
				private:
					// Идентификатор захвативший доступ
					uint32_t _id;
				private:
					// Объект сторожа
					Guard * _guard;
				public:
					/**
					 * @brief Оператор проверки доступа
					 * 
					 * @return результат проверки
					 */
					explicit operator bool() const noexcept;
				public:
					// Запрещаем копирование
					Locker(const Locker &) = delete;
					/**
					 * @brief Оператор запрещения копирования
					 * 
					 * @return текущий объект локера
					 */
					Locker & operator = (const Locker &) = delete;
				public:
					/**
					 * @brief Конструктор перемещения
					 * 
					 * @param other объект другого локера
					 */
					Locker(Locker && other) noexcept;
				public:
					/**
					 * @brief Конструктор
					 * 
					 * @param guard основной объект сторожа
					 */
					Locker(Guard & guard) noexcept;
					/**
					 * @brief Конструктор
					 * 
					 * @param id    изентификатор желающий захватить функцию
					 * @param guard основной объект сторожа
					 */
					Locker(const uint32_t id, Guard & guard) noexcept;
				public:
					/**
					 * @brief Деструктор
					 * 
					 */
					~Locker() noexcept;
			} locker_t;
		private:
			// Флаг предоставления доступа
			std::atomic_bool _ok;
		private:
			// Мютекс для блокировки потока
			mutable std::mutex _mtx;
		private:
			// Список активных идентификаторов
			std::set <uint32_t> _ids;
		public:
			/**
			 * @brief Метод проверки на доступ к функции
			 * 
			 * @return результат проверки
			 */
			bool locked() noexcept;
			/**
			 * @brief Метод проверки на доступ к функции
			 * 
			 * @param id идентификатор проверяющий доступ
			 * @return   результат проверки
			 */
			bool locked(const uint32_t id) noexcept;
		public:
			/**
			 * @brief Метод выполнения блокировки доступа к функции
			 * 
			 * @return результат блокировки
			 */
			locker_t lock() noexcept;
			/**
			 * @brief Метод выполнения блокировки доступа к функции
			 * 
			 * @param id идентификатор желающий захватить доступ
			 * @return   результат блокировки
			 */
			locker_t lock(const uint32_t id) noexcept;
		public:
			/**
			 * @brief Конструктор
			 * 
			 */
			Guard() noexcept;
	} guard_t;
};

#endif // __AWH_CORE_GUARD__
