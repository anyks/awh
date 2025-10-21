/**
 * @file: locker.hpp
 * @date: 2025-10-20
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

#ifndef __AWH_LOCKER__
#define __AWH_LOCKER__

/**
 * Стандартные модули
 */
#include <mutex>
#include <memory>
#include <atomic>
#include <cinttypes>
#include <unistd.h>

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
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam MutexType тип данных состояния блокировок
	 */
	template <typename MutexType = std::mutex>
	/**
	 * @brief Класс состояния блокировок
	 *
	 */
	class LockState {
		private:
			/**
			* @brief Шаблон формата данных локера
			*
			* @tparam T тип данных локера
			*/
			template <typename T>
			/**
			 * @brief Устанавливаем дружбу с локером
			 *
			 */
			friend class Locker;
		private:
			// Идентификатор процесса
			std::atomic <pid_t> _pid;
		private:
			// Мютекс для блокировки потока
			std::unique_ptr <MutexType> _mtx;
		public:
			/**
			* @brief Оператор копирования
			*
			*/
			LockState & operator = (const LockState &) = delete;
		public:
			/**
			* @brief Конструктор копирования
			*
			*/
			LockState(const LockState &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			LockState() noexcept :
			 _pid(::getpid()),
			 _mtx(std::make_unique <MutexType> ()) {}
	};
	/**
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam T данные состояния блокировок
	 */
	template <typename MutexType = std::mutex>
	 /**
	  * Создаём тип данных работы с состоянием блокировок
	  */
	using lock_state_t = LockState <MutexType>;
	/**
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam MutexType тип данных состояния блокировок
	 */
	 template <typename MutexType = std::mutex>
	/**
	 * @brief Класс локера
	 *
	 */
	class Locker {
		private:
			// Сохраняем временно объект состояния блокировок
			LockState <MutexType> & _state;
		public:
			/**
			* @brief Оператор копирования
			*
			*/
			Locker & operator = (const Locker &) = delete;
		public:
			/**
			* @brief Конструктор копирования
			*
			*/
			Locker(const Locker &) = delete;
		public:
			/**
			* @brief Конструктор
			*
			* @param state объект состояния блокировок
			*/
			Locker(LockState <MutexType> & state) noexcept : _state(state) {
				// Если идентификатор процесса не совпадает
				if(this->_state._pid != ::getpid()){
					// Устанавливаем идентификатор процесса
					this->_state._pid = ::getpid();
					// Выполняем удаление мютекса
					this->_state._mtx.reset(nullptr);
				}
				// Если мютекс пустой
				if(this->_state._mtx == nullptr)
					// Пересоздаём мютекс
					this->_state._mtx = std::make_unique <MutexType> ();
				// Выполняем блокировку потока
				this->_state._mtx->lock();
			}
		public:
			/**
			* @brief Деструктор
			*
			*/
			~Locker() noexcept {
				// Если мютекс не пустой
				if(this->_state._mtx != nullptr)
					// Выполняем разблокировку потока
					this->_state._mtx->unlock();
			}
	};
	/**
	 * @brief Шаблон формата данных локера
	 *
	 * @tparam MutexType данные локера
	 */
	 template <typename MutexType = std::mutex>
	 /**
	  * Создаём тип данных работы с локера
	  */
	using locker_t = Locker <MutexType>;
};

#endif // __AWH_LOCKER__
