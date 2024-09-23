/**
 * @file: hold.hpp
 * @date: 2023-08-05
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_HOLDER__
#define __AWH_HOLDER__

/**
 * Стандартные модули
 */
#include <set>
#include <mutex>
#include <stack>

/**
 * Разрешаем сборку под Windows
 */
#include <sys/global.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Шаблон формата данных статусов холдера
	 * @tparam T данные статусов холдера
	 */
	template <typename T>
	/**
	 * Holder Класс холдера
	 */
	class AWHSHARED_EXPORT Holder {
		private:
			// Флаг холдирования
			bool _flag;
		private:
			// Мютекс для блокировки потока
			mutex _mtx;
		private:
			// Объект статуса работы DNS-резолвера
			stack <T> & _status;
		public:
			/**
			 * access Метод проверки на разрешение выполнения операции
			 * @param comp  статус сравнения
			 * @param hold  статус установки
			 * @param equal флаг эквивалентности
			 * @return      результат проверки
			 */
			bool access(const set <T> & comp, const T hold, const bool equal = true) noexcept {
				// Определяем есть ли фиксированные статусы
				this->_flag = this->_status.empty();
				// Если результат не получен
				if(!this->_flag && !comp.empty())
					// Получаем результат сравнения
					this->_flag = (equal ? (comp.count(this->_status.top()) > 0) : (comp.count(this->_status.top()) < 1));
				// Если результат получен, выполняем холд
				if(this->_flag){
					// Выполняем блокировку потока
					const lock_guard <mutex> lock(this->_mtx);
					// Выполняем установку холда
					this->_status.push(hold);
				}
				// Выводим результат
				return this->_flag;
			}
		public:
			/**
			 * Holder Конструктор
			 * @param status объект статуса работы DNS-резолвера
			 */
			Holder(stack <T> & status) noexcept : _flag(false), _status(status) {}
			/**
			 * ~Holder Деструктор
			 */
			~Holder() noexcept {
				// Если холдирование выполнено
				if(this->_flag){
					// Выполняем блокировку потока
					const lock_guard <mutex> lock(this->_mtx);
					// Выполняем снятие холда
					this->_status.pop();
				}
			}
	};
	/**
	 * Шаблон формата данных статусов холдера
	 * @tclass T данные статусов холдера
	 */
	template <class T>
	// Создаём тип данных работы с холдером
	using hold_t = Holder <T>;
};

#endif // __AWH_HOLDER__
