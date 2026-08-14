/**
 * @file holder.hpp
 * @date 2026-01-25
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
 * \~russian
 * @brief Заголовочный файл модуля холдера — класс Holder, реализующий RAII-владение состоянием и автоматическое
 *        восстановление предыдущего значения при выходе из области видимости
 *
 * \~english
 * @brief Header file of the holder module — the Holder class, which implements RAII ownership of a state and the automatic
 *        restoration of the previous value on leaving the scope
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_HOLDER__
#define __AWH_HOLDER__

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <stack>
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "locker.hpp"

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
	 * @brief Шаблон формата данных статусов холдера
	 *
	 * @tparam T         тип данных статусов холдера
	 * @tparam MutexType тип данных внешнего мьютекса (std::mutex или std::shared_mutex)
	 *
	 * \~english
	 * @brief Template of the data format of the statuses of the holder
	 * @tparam T         data type of the statuses of the holder
	 * @tparam MutexType data type of the external mutex (std::mutex or std::shared_mutex)
	 *
	 * \~
	 */
	template <typename T, typename MutexType = std::mutex>
	/**
	 * \~russian
	 * @brief Класс холдера
	 *
	 * \~english
	 * @brief Holder class
	 *
	 * \~
	 */
	class Holder {
		private:
			// Флаг холдирования
			bool _flag;
		private:
			// Ссылка на стек статусов
			std::stack <T> & _status;
		private:
			// Ссылка на внешний мьютекс владельца стека статусов
			lock_state_t <MutexType> & _mtx;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на разрешение выполнения операции
			 *
			 * @param comp  множество для сравнения текущего статуса
			 * @param hold  статус, который нужно установить (захолдить)
			 * @param equal флаг эквивалентности (true - разрешено, если есть в comp, false - если нет)
			 * @return      результат проверки (удалось ли захватить)
			 *
			 * \~english
			 * @brief Method of checking the permission to perform an operation
			 * @param comp  set to compare the current status against
			 * @param hold  status that needs to be set (held)
			 * @param equal equivalence flag (true — allowed if present in comp, false — if absent)
			 * @return      result of the check (whether the capture succeeded)
			 *
			 * \~
			 */
			bool access(const unordered_set <T> & comp, const T hold, const bool equal = true) noexcept {
				// Выполняем эксклюзивную блокировку потока на всю транзакцию (проверка вершины и установка холда)
				const locker_t <MutexType> lock(this->_mtx);
				// Определяем есть ли фиксированные статусы
				this->_flag = this->_status.empty();
				// Если на вершине стека уже есть активный статус и задано множество для сравнения
				if(!this->_flag && !comp.empty())
					// Получаем результат сравнения текущего статуса со множеством разрешённых/запрещённых
					this->_flag = (equal ? (comp.count(this->_status.top()) > 0) : (comp.count(this->_status.top()) < 1));
				// Если необходимо выполнить холдирование
				if(this->_flag)
					// Выполняем установку холда
					this->_status.push(hold);
				// Возвращаем результат
				return this->_flag;
			}
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param status ссылка на стек статусов
			 * @param mtx    ссылка на внешний мьютекс владельца стека статусов
			 *
			 * \~english
			 * @brief Constructor
			 * @param status reference to the stack of statuses
			 * @param mtx    reference to the external mutex of the owner of the stack of statuses
			 *
			 * \~
			 */
			explicit Holder(std::stack <T> & status, lock_state_t <MutexType> & mtx) noexcept :
			 _flag(false), _status(status), _mtx(mtx) {}
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
			~Holder() noexcept {
				// Если холдирование выполнено
				if(this->_flag){
					// Выполняем эксклюзивную блокировку потока
					const locker_t <MutexType> lock(this->_mtx);
					// Выполняем снятие холда
					this->_status.pop();
				}
			}
	};
	/**
	 * \~russian
	 * @brief Шаблон формата данных статусов холдера
	 *
	 * @tparam T         данные статусов холдера
	 * @tparam MutexType тип данных внешнего мьютекса (std::mutex или std::shared_mutex)
	 *
	 * \~english
	 * @brief Template of the data format of the statuses of the holder
	 * @tparam T         data of the statuses of the holder
	 * @tparam MutexType data type of the external mutex (std::mutex or std::shared_mutex)
	 *
	 * \~
	 */
	template <class T, typename MutexType = std::mutex>
	/**
	 * Создаём тип данных работы с холдером
	 */
	using holder_t = Holder <T, MutexType>;
};

#endif // __AWH_HOLDER__
