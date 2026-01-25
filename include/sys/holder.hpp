/**
 * @file: holder.hpp
 * @date: 2026-01-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_HOLDER__
#define __AWH_HOLDER__

/**
 * Стандартные модули
 */
#include <mutex>
#include <stack>
#include <atomic>
#include <cstdint>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "locker.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Шаблон формата данных статусов холдера
	 *
	 * @tparam T тип данных статусов холдера
	 */
	template <typename T>
	/**
	 * @brief Класс холдера
	 *
	 */
	class Holder {
		public:
			/**
			 * @brief Режимы событий
			 *
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x00, // Режим включён
				DISABLED = 0x01  // Режим отключён
			};
		private:
			// Флаг холдирования
			std::atomic_bool _flag;
		private:
			// Ссылка на стек статусов
			std::stack <T> & _status;
		private:
			// Мютекс для блокировки потока
			lock_state_t <std::mutex> _mtx;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode режим безопасности потоков
			 */
			void threadSafety(const mode_t mode) noexcept {
				// Устанавливаем режим безопасности потоков
				this->_mtx.enabled = (mode == mode_t::ENABLED);
			}
		public:
			/**
			 * @brief Метод проверки на разрешение выполнения операции
			 *
			 * @param comp  множество для сравнения текущего статуса
			 * @param hold  статус, который нужно установить (захолдить)
			 * @param equal флаг эквивалентности (true - разрешено, если есть в comp, false - если нет)
			 * @return      результат проверки (удалось ли захватить)
			 */
			bool access(const std::unordered_set <T> & comp, const T hold, const bool equal = true) noexcept {
				// Определяем есть ли фиксированные статусы
				this->_flag.store(this->_status.empty(), std::memory_order_release);
				// Если результат не получен
				if(!this->_flag.load(std::memory_order_acquire) && !comp.empty())
					// Получаем результат сравнения
					this->_flag.store((equal ? (comp.count(this->_status.top()) > 0) : (comp.count(this->_status.top()) < 1)), std::memory_order_release);
				// Если необходимо выполнить холдирование
				if(this->_flag.load(std::memory_order_acquire)){
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx);
					// Выполняем установку холда
					this->_status.push(hold);
				}
				// Выводим результат
				return this->_flag.load(std::memory_order_acquire);
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param status ссылка на стек статусов
			 */
			explicit Holder(std::stack <T> & status) noexcept : _flag(false), _status(status) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Holder() noexcept {
				// Если холдирование выполнено
				if(this->_flag.load(std::memory_order_acquire)){
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx);
					// Выполняем снятие холда
					this->_status.pop();
				}
			}
	};
	/**
	 * @brief Шаблон формата данных статусов холдера
	 *
	 * @tparam T данные статусов холдера
	 */
	template <class T>
	/**
	 * Создаём тип данных работы с холдером
	 */
	using holder_t = Holder <T>;
};

#endif // __AWH_HOLDER__
