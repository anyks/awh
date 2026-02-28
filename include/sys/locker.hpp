/**
 * @file: locker.hpp
 * @date: 2025-10-25
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

/**
 * Экранируем повторную инициализацию модуля
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
#include <type_traits>
#include <shared_mutex>
#include <unistd.h>

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Шаблон trait для определения поддержки shared_lock
	 * Работает в C++17 через SFINAE
	 *
	 * @tparam T тип данных для проверки поддержки shared_lock
	 */
	template <typename T, typename = void>
	/**
	 * @brief Trait для определения поддержки shared_lock
	 *
	 */
	struct has_shared_lock : std::false_type {};

	/**
	 * @brief Шаблон для определения поддержки shared_lock
	 * Работает в C++17 через SFINAE
	 *
	 * @tparam T тип данных для проверки поддержки shared_lock
	 */
	template <typename T>
	/**
	 * @brief Тип данных для определения поддержки shared_lock
	 *
	 */
	struct has_shared_lock <T, std::void_t<
		decltype(std::declval <T &> ().lock_shared()),
		decltype(std::declval <T &> ().unlock_shared())
	>> : std::true_type {};

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
		public:
			// Флаг активации режима работы
			std::atomic_bool enabled;
		private:
			// Идентификатор процесса
			std::atomic <pid_t> _pid;
		private:
			// Мютекс для блокировки потока
			std::unique_ptr <MutexType> _mtx;
		public:
			/**
			 * @brief Оператор преобразования к мютексу
			 *
			 */
			operator MutexType & () noexcept {
				// Выводим мютекс для блокировки потока
				return (* this->_mtx);
			}
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
			explicit LockState(const LockState &) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit LockState() noexcept :
			 enabled(true),
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
	 * @brief Создаём тип данных работы с состоянием блокировок
	 *
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
		public:
			/**
			 * @brief Режим блокировки мьютекса
			 *
			 */
			enum class mode_t : uint8_t {
				NONE      = 0x00, // Без блокировки
				SHARED    = 0x01, // Разделённая блокировка (lock_shared/unlock_shared) - для чтения
				EXCLUSIVE = 0x02  // Уникальная блокировка (lock/unlock) - для записи
			};
		private:
			// Флаг захвата мютексом потока
			bool _locked;
		private:
			// Режим блокировки
			mode_t _mode;
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
			explicit Locker(const Locker &) = delete;
		private:
			/**
			 * @brief Шаблон метода блокировки в эксклюзивном режиме
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод блокировки в эксклюзивном режиме
			 *
			 */
			void _lockImpl(std::false_type) noexcept {
				// Выполняем обычную блокировку
				this->_state._mtx->lock();
			}
			/**
			 * @brief Шаблон метода разблокировки в эксклюзивном режиме
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод разблокировки в эксклюзивном режиме
			 *
			 */
			void _unlockImpl(std::false_type) noexcept {
				// Выполняем обычную разблокировку
				this->_state._mtx->unlock();
			}
		private:
			/**
			 * @brief Шаблон метода разделённой блокировки
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод разделённой блокировки
			 *
			 * @return результат выполнения операции
			 */
			typename std::enable_if <has_shared_lock <M>::value, void>::type _lockImpl(std::true_type) noexcept {
				// Выполняем разделённую блокировку
				this->_state._mtx->lock_shared();
			}
			/**
			 * @brief Шаблон метода уникальной блокировки при поддержке shared_lock
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод уникальной блокировки при отсутствии поддержки shared_lock
			 *
			 * @return результат выполнения операции
			 */
			typename std::enable_if <!has_shared_lock <M>::value, void>::type _lockImpl(std::true_type) noexcept {
				// Выполняем обычную блокировку, так как мьютекс не поддерживает разделённую блокировку
				this->_state._mtx->lock();
			}
		private:
			/**
			 * @brief Шаблон метода разделённой разблокировки
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод разделённой разблокировки
			 *
			 * @return результат выполнения операции
			 */
			typename std::enable_if <has_shared_lock <M>::value, void>::type _unlockImpl(std::true_type) noexcept {
				// Выполняем разделённую разблокировку
				this->_state._mtx->unlock_shared();
			}
			/**
			 * @brief Шаблон метода уникальной разблокировки при поддержке shared_lock
			 *
			 * @tparam M тип данных мьютекса
			 */
			template <typename M = MutexType>
			/**
			 * @brief Метод уникальной разблокировки при отсутствии поддержки shared_lock
			 *
			 * @return результат выполнения операции
			 */
			typename std::enable_if <!has_shared_lock <M>::value, void>::type _unlockImpl(std::true_type) noexcept {
				// Выполняем обычную разблокировку, так как мьютекс не поддерживает разделённую блокировку
				this->_state._mtx->unlock();
			}
		private:
			/**
			 * @brief Метод выполнения блокировки в зависимости от режима и типа мьютекса
			 *
			 */
			void _lock() noexcept {
				/**
				 * Вычисляем константное выражение для определения поддержки shared_lock
				 */
				constexpr bool has_shared = has_shared_lock <MutexType>::value;
				// Если режим блокировки SHARED и мьютекс поддерживает shared_lock
				if((this->_mode == mode_t::SHARED) && has_shared)
					// Выполняем разделённую блокировку
					this->_lockImpl(std::true_type{});
				// Выполняем обычную блокировку
				else this->_lockImpl(std::false_type{});
			}
			/**
			 * @brief Метод выполнения разблокировки в зависимости от режима и типа мьютекса
			 *
			 */
			void _unlock() noexcept {
				/**
				 * Вычисляем константное выражение для определения поддержки shared_lock
				 */
				constexpr bool has_shared = has_shared_lock <MutexType>::value;
				// Если режим блокировки SHARED и мьютекс поддерживает shared_lock
				if((this->_mode == mode_t::SHARED) && has_shared)
					// Выполняем разделённую разблокировку
					this->_unlockImpl(std::true_type{});
				// Выполняем обычную разблокировку
				else this->_unlockImpl(std::false_type{});
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param state объект состояния блокировок
			 * @param mode  режим блокировки (по умолчанию Exclusive для обратной совместимости)
			 */
			explicit Locker(LockState <MutexType> & state, mode_t mode = mode_t::EXCLUSIVE) noexcept
			 : _locked(false), _mode(mode), _state(state) {
				// Если идентификатор процесса не совпадает
				if(this->_state._pid.load(std::memory_order_acquire) != ::getpid()){
					// Устанавливаем идентификатор процесса
					this->_state._pid.store(::getpid(), std::memory_order_release);
					// Выполняем удаление мютекса
					this->_state._mtx.reset(nullptr);
				}
				// Если мютекс пустой
				if(this->_state._mtx == nullptr)
					// Пересоздаём мютекс
					this->_state._mtx = std::make_unique <MutexType> ();
				// Если захватывать доступ к памяти нам не нужно
				if(!this->_state.enabled.load(std::memory_order_acquire))
					// Выходим из конструктора
					return;
				// Выполняем блокировку
				this->_locked = true;
				// Выполняем блокировку потока
				this->_lock();
			}
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Locker() noexcept {
				// Если мютекс не пустой и захвачен
				if((this->_state._mtx != nullptr) && this->_locked)
					// Выполняем разблокировку потока
					this->_unlock();
			}
	};
	/**
	 * @brief Шаблон формата данных локера
	 *
	 * @tparam MutexType данные локера
	 */
	 template <typename MutexType = std::mutex>
	 /**
	  * Создаём тип данных работы с локом
	  */
	using locker_t = Locker <MutexType>;
};

#endif // __AWH_LOCKER__
