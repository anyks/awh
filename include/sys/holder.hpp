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
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Шаблон формата данных статусов холдера
	 *
	 * @tparam T данные статусов холдера
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
			bool _flag;
		private:
			// Мютекс для блокировки потока
			lock_state_t <std::mutex> _mtx;
		private:
			// Объект статуса работы DNS-резолвера
			std::stack <T> & _status;
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
			 * @param comp  статус сравнения
			 * @param hold  статус установки
			 * @param equal флаг эквивалентности
			 * @return      результат проверки
			 */
			bool access(const std::unordered_set <T> & comp, const T hold, const bool equal = true) noexcept {
				// Определяем есть ли фиксированные статусы
				this->_flag = this->_status.empty();
				// Если результат не получен
				if(!this->_flag && !comp.empty())
					// Получаем результат сравнения
					this->_flag = (equal ? (comp.count(this->_status.top()) > 0) : (comp.count(this->_status.top()) < 1));
				// Если результат получен, выполняем холд
				if(this->_flag){
					// Выполняем блокировку потока
					const locker_t <> lock(this->_mtx);
					// Выполняем установку холда
					this->_status.push(hold);
				}
				// Выводим результат
				return this->_flag;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param status объект статуса работы DNS-резолвера
			 */
			explicit Holder(std::stack <T> & status) noexcept : _flag(false), _status(status) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~Holder() noexcept {
				// Если холдирование выполнено
				if(this->_flag){
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
