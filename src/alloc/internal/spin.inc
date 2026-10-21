/**
 * @file spin.inc
 * @date 2026-10-21
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Внутренние тела встраиваемых методов замка и удержания замка
 *
 * @details Файл подключается реализациями модуля и в поставляемый набор не входит:
 *          под include/ собраны только интерфейсные заголовки. Тело метода обязано
 *          быть видно в каждой единице трансляции, зовущей метод, - иначе
 *          всегда-встраивание не работает, - а общее тело в одном месте избавляет
 *          от копий по числу зовущих файлов.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Подключаем заголовочный файл модуля
 */
#include <alloc/spin.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Пространство имён фреймворка
 */
namespace awh {
	/**
	 * Пространство имён распределителя памяти
	 */
	namespace alloc {
		/**
		 * @brief Метод захвата замка
		 *
		 */
		AWH_SPIN_INLINE void spin_t::acquire() noexcept {
			// Значение незахваченного замка
			uint32_t free = 0;
			// Пробуем захватить замок разом
			if(!this->_held.compare_exchange_weak(free, 1, std::memory_order_acquire, std::memory_order_relaxed))
				// Захватываем замок кружением
				this->wait();
		}
		/**
		 * @brief Метод освобождения замка
		 *
		 */
		AWH_SPIN_INLINE void spin_t::release() noexcept {
			// Освобождаем замок
			this->_held.store(0, std::memory_order_release);
		}
		/**
		 * @brief Конструктор
		 *
		 * @param spin удерживаемый замок
		 *
		 */
		AWH_SPIN_INLINE hold_t::Hold(spin_t & spin) noexcept : _spin(spin) {
			// Захватываем замок
			this->_spin.acquire();
		}
		/**
		 * @brief Деструктор
		 *
		 */
		AWH_SPIN_INLINE hold_t::~Hold() noexcept {
			// Освобождаем замок
			this->_spin.release();
		}
	};
};
