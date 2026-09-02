/**
 * @file locker.hpp
 * @date 2025-10-25
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
 * @brief Заголовочный файл модуля блокировок — классы Locker, LockState и Enabled_Property,
 *        обеспечивающие управление исключительными и разделяемыми мьютексами,
 *        отслеживание состояния блокировок и булевы свойства с уведомлением об изменении
 *
 * \~english
 * @brief Header file of the locking module — the Locker, LockState and Enabled_Property classes,
 *        which provide the management of exclusive and shared mutexes,
 *        the tracking of the state of the locks and boolean properties with a notification of a change
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LOCKER__
#define __AWH_LOCKER__

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <memory>
#include <atomic>
#include <functional>
#include <type_traits>

/**
 * Системный заголовочный файл
 */
#include <unistd.h>

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "push.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Шаблон trait для определения поддержки shared_lock
	 *
	 * @note Работает в C++17 через SFINAE
	 *
	 * @tparam T тип данных для проверки поддержки shared_lock
	 *
	 * \~english
	 * @brief Template of the trait determining the support of shared_lock
	 * @note Works in C++17 through SFINAE
	 * @tparam T data type to check the support of shared_lock for
	 *
	 * \~
	 */
	template <typename T, typename = void>
	/**
	 * \~russian
	 * @brief Trait для определения поддержки shared_lock
	 *
	 * \~english
	 * @brief Trait determining the support of shared_lock
	 *
	 * \~
	 */
	struct has_shared_lock : std::false_type {};

	/**
	 * \~russian
	 * @brief Шаблон для определения поддержки shared_lock
	 *
	 * @note Работает в C++17 через SFINAE
	 *
	 * @tparam T тип данных для проверки поддержки shared_lock
	 *
	 * \~english
	 * @brief Template determining the support of shared_lock
	 * @note Works in C++17 through SFINAE
	 * @tparam T data type to check the support of shared_lock for
	 *
	 * \~
	 */
	template <typename T>
	/**
	 * \~russian
	 * @brief Тип данных для определения поддержки shared_lock
	 *
	 * \~english
	 * @brief Data type determining the support of shared_lock
	 *
	 * \~
	 */
	struct has_shared_lock <T, std::void_t<
		decltype(std::declval <T &> ().lock_shared()),
		decltype(std::declval <T &> ().unlock_shared())
	>> : std::true_type {};

	/**
	 * \~russian
	 * @brief Класс свойства булевого значения с поддержкой функции обратного вызова при изменении значения
	 *
	 * \~english
	 * @brief Class of a boolean value property with the support of a callback function on a change of the value
	 *
	 * \~
	 */
	typedef class Enabled_Property {
		private:
			// Значение свойства
			std::atomic_bool _value;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова для дополнительных действий при изменении значения свойства
			 *
			 * @param value новое значение свойства
			 *
			 * \~english
			 * @brief Callback function for additional actions on a change of the value of the property
			 * @param value new value of the property
			 *
			 * \~
			 */
			function <void (bool)> _callback;
		public:
			/**
			 * \~russian
			 * @brief Оператор преобразования к булевому типу для получения текущего значения свойства
			 *
			 * @return текущее значение свойства
			 *
			 * \~english
			 * @brief Conversion operator to the boolean type for getting the current value of the property
			 * @return current value of the property
			 *
			 * \~
			 */
			operator bool() const noexcept {
				// Возвращаем текущее значение свойства
				return this->_value.load(std::memory_order_acquire);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор сравнения несоответствия значения свойства с заданным булевым значением
			 *
			 * @param value булевое значение для сравнения
			 * @return      результат сравнения
			 *
			 * \~english
			 * @brief Inequality comparison operator of the value of the property with the given boolean value
			 * @param value boolean value to compare with
			 * @return      result of the comparison
			 *
			 * \~
			 */
			bool operator != (const bool value) const noexcept {
				// Сравниваем текущее значение свойства с заданным значением
				return (value != this->_value.load(std::memory_order_acquire));
			}
			/**
			 * \~russian
			 * @brief Оператор сравнения соответствия значения свойства с заданным булевым значением
			 *
			 * @param value булевое значение для сравнения
			 * @return      результат сравнения
			 *
			 * \~english
			 * @brief Equality comparison operator of the value of the property with the given boolean value
			 * @param value boolean value to compare with
			 * @return      result of the comparison
			 *
			 * \~
			 */
			bool operator == (const bool value) const noexcept {
				// Сравниваем текущее значение свойства с заданным значением
				return (value == this->_value.load(std::memory_order_acquire));
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания для копирования значения свойства из другого свойства
			 *
			 * @param other другое свойство для копирования значения
			 * @return      ссылка на текущий объект для цепочки присваиваний
			 *
			 * \~english
			 * @brief Assignment operator for copying the value of the property from another property
			 * @param other another property to copy the value from
			 * @return      reference to the current object for a chain of assignments
			 *
			 * \~
			 */
			Enabled_Property & operator = (const Enabled_Property & other) noexcept {
				// Копируем значение из другого свойства
				return (* this) = static_cast <bool> (other);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор присваивания для изменения значения свойства
			 *
			 * @param value новое значение свойства
			 * @return      ссылка на текущий объект для цепочки присваиваний
			 *
			 * \~english
			 * @brief Assignment operator for changing the value of the property
			 * @param value new value of the property
			 * @return      reference to the current object for a chain of assignments
			 *
			 * \~
			 */
			Enabled_Property & operator = (const bool value) noexcept {
				// Если значение изменилось
				if(this->_value.load(std::memory_order_acquire) != value){
					// Меняем значение только если оно действительно изменилось
					this->_value.store(value, std::memory_order_release);
					// Если функция обратного вызова установлена
					if(this->_callback != nullptr)
						// Выполняем дополнительные действия
						this->_callback(this->_value.load(std::memory_order_acquire));
				}
				// Возвращаем текущее значение объекта
				return (* this);
			}
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param value    начальное значение свойства
			 * @param callback функция обратного вызова для дополнительных действий при изменении значения 
			 *
			 * \~english
			 * @brief Constructor
			 * @param value    initial value of the property
			 * @param callback callback function for additional actions on a change of the value
			 *
			 * \~
			 */
			Enabled_Property(const bool value, function <void (bool)> callback = nullptr) noexcept :
			 _value(value), _callback(callback) {}
	} enabled_property_t;

	/**
	 * \~russian
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam MutexType тип данных состояния блокировок
	 *
	 * \~english
	 * @brief Template of the data format of the state of the locks
	 * @tparam MutexType data type of the state of the locks
	 *
	 * \~
	 */
	template <typename MutexType = std::mutex>
	/**
	 * \~russian
	 * @brief Класс состояния блокировок
	 *
	 * \~english
	 * @brief Class of the state of the locks
	 *
	 * \~
	 */
	class LockState {
		private:
			/**
			 * \~russian
			 * @brief Шаблон формата данных локера
			 *
			 * @tparam T тип данных локера
			 *
			 * \~english
			 * @brief Template of the data format of the locker
			 * @tparam T data type of the locker
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Устанавливаем дружбу с локером
			 *
			 * \~english
			 * @brief Establish friendship with the locker
			 *
			 * \~
			 */
			friend class Locker;
		private:
			// Флаг активации режима работы
			std::atomic_bool _enabled;
		private:
			// Идентификатор процесса
			std::atomic <pid_t> _pid;
		public:
			// Флаг включённости блокировок
			enabled_property_t enabled;
		private:
			// Мютекс для блокировки потока
			std::unique_ptr <MutexType> _mtx;
		private:
			/**
			 * \~russian
			 * @brief Гарантирует существование рабочего мьютекса с учётом смены процесса
			 *
			 * @note Выполняет ленивое создание мьютекса и его пересоздание после fork
			 *
			 * @return указатель на актуальный рабочий мьютекс
			 *
			 * \~english
			 * @brief Guarantees the existence of the working mutex taking into account a change of the process
			 * @note Performs the lazy creation of the mutex and its recreation after fork
			 * @return pointer to the current working mutex
			 *
			 * \~
			 */
			MutexType * _ensure() noexcept {
				// Если идентификатор процесса не совпадает (например, после fork)
				if(this->_pid.load(std::memory_order_acquire) != ::getpid()){
					// Устанавливаем идентификатор процесса
					this->_pid.store(::getpid(), std::memory_order_release);
					// Если мютекс уже создан ранее
					if(this->_mtx != nullptr)
						// Выполняем удаление унаследованного от родителя мютекса
						this->_mtx.reset(nullptr);
				}
				// Если мютекс пустой
				if(this->_mtx == nullptr)
					// Создаём рабочий мьютекс по требованию
					this->_mtx = std::make_unique <MutexType> ();
				// Возвращаем актуальный рабочий мьютекс
				return this->_mtx.get();
			}
		private:
			/**
			 * \~russian
			 * @brief Метод, который будет вызван при изменении флага активации/деактивации блокировок
			 *
			 * @param value новое значение флага
			 *
			 * \~english
			 * @brief Method that will be called on a change of the flag of enabling/disabling the locks
			 * @param value new value of the flag
			 *
			 * \~
			 */
			void onEnabledChanged(const bool value) noexcept {
				// Устанавливаем новое значение флага активации/деактивации блокировок
				this->_enabled.store(value, std::memory_order_release);
				// Если блокировки включены, гарантируем существование рабочего мьютекса
				if(this->_enabled.load(std::memory_order_acquire))
					// Если блокировки включены, гарантируем существование рабочего мьютекса
					(void) this->_ensure();
				// Если блокировки отключены, удаляем рабочий мьютекс для освобождения ресурсов
				else this->_mtx.reset(nullptr);
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор преобразования к мютексу
			 *
			 * @note Лениво создаёт мьютекс по требованию (например, для condition_variable)
			 *
			 * \~english
			 * @brief Conversion operator to a mutex
			 * @note Lazily creates the mutex on demand (for example, for condition_variable)
			 *
			 * \~
			 */
			operator MutexType & () noexcept {
				// Лениво создаём (или пересоздаём после fork) рабочий мьютекс
				MutexType & mtx = (* this->_ensure());
				// Возвращаем мютекс для блокировки потока
				return mtx;
			}
		public:
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * \~
			 */
			LockState & operator = (const LockState &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 *
			 * \~english
			 * @brief Copy constructor
			 *
			 * \~
			 */
			explicit LockState(const LockState &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			explicit LockState() noexcept :
			 _enabled(false), _pid(::getpid()), enabled {
				// Активируем блокировки по умолчанию при создании объекта
				true,
				/**
				 * \~russian
				 * @brief Функция обратного вызова для изменения флага активации/деактивации блокировок
				 *
				 * @param value новое значение флага
				 *
				 * \~english
				 * @brief Callback function for changing the flag of enabling/disabling the locks
				 * @param value new value of the flag
				 *
				 * \~
				 */
				[this](const bool value) noexcept {
					// Устанавливаем новое значение флага активации/деактивации блокировок
					this->onEnabledChanged(value);
				}
			}, _mtx(nullptr) {
				// Активируем блокировки по умолчанию при создании объекта
				this->onEnabledChanged(true);
			}
	};
	/**
	 * \~russian
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam T данные состояния блокировок
	 *
	 * \~english
	 * @brief Template of the data format of the state of the locks
	 * @tparam T data of the state of the locks
	 *
	 * \~
	 */
	template <typename MutexType = std::mutex>
	/**
	 * \~russian
	 * @brief Создаём тип данных работы с состоянием блокировок
	 *
	 * \~english
	 * @brief Create the data type for working with the state of the locks
	 *
	 * \~
	 */
	using lock_state_t = LockState <MutexType>;

	/**
	 * \~russian
	 * @brief Шаблон формата данных состояния блокировок
	 *
	 * @tparam MutexType тип данных состояния блокировок
	 *
	 * \~english
	 * @brief Template of the data format of the state of the locks
	 * @tparam MutexType data type of the state of the locks
	 *
	 * \~
	 */
	template <typename MutexType = std::mutex>
	/**
	 * \~russian
	 * @brief Класс локера
	 *
	 * \~english
	 * @brief Locker class
	 *
	 * \~
	 */
	class Locker {
		public:
			/**
			 * \~russian
			 * @brief Режим блокировки мьютекса
			 *
			 * \~english
			 * @brief Locking mode of the mutex
			 *
			 * \~
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
		private:
			// Указатель на мьютекс, который был фактически захвачен текущим локером
			MutexType * _held;
		public:
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * \~
			 */
			Locker & operator = (const Locker &) = delete;
		public:
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 *
			 * \~english
			 * @brief Copy constructor
			 *
			 * \~
			 */
			explicit Locker(const Locker &) = delete;
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода блокировки в эксклюзивном режиме
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of locking in the exclusive mode
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод блокировки в эксклюзивном режиме
			 *
			 * \~english
			 * @brief Method of locking in the exclusive mode
			 *
			 * \~
			 */
			void _lockImpl(std::false_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную блокировку
					this->_held->lock();
			}
			/**
			 * \~russian
			 * @brief Шаблон метода разблокировки в эксклюзивном режиме
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of unlocking in the exclusive mode
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод разблокировки в эксклюзивном режиме
			 *
			 * \~english
			 * @brief Method of unlocking in the exclusive mode
			 *
			 * \~
			 */
			void _unlockImpl(std::false_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную разблокировку
					this->_held->unlock();
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода разделённой блокировки
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of shared locking
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод разделённой блокировки
			 *
			 * @return результат выполнения операции
			 *
			 * \~english
			 * @brief Method of shared locking
			 * @return result of performing the operation
			 *
			 * \~
			 */
			typename std::enable_if <has_shared_lock <M>::value, void>::type _lockImpl(std::true_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем разделённую блокировку
					this->_held->lock_shared();
			}
			/**
			 * \~russian
			 * @brief Шаблон метода уникальной блокировки при поддержке shared_lock
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of unique locking when shared_lock is supported
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод уникальной блокировки при отсутствии поддержки shared_lock
			 *
			 * @return результат выполнения операции
			 *
			 * \~english
			 * @brief Method of unique locking when shared_lock is not supported
			 * @return result of performing the operation
			 *
			 * \~
			 */
			typename std::enable_if <!has_shared_lock <M>::value, void>::type _lockImpl(std::true_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную блокировку, так как мьютекс не поддерживает разделённую блокировку
					this->_held->lock();
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода разделённой разблокировки
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of shared unlocking
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод разделённой разблокировки
			 *
			 * @return результат выполнения операции
			 *
			 * \~english
			 * @brief Method of shared unlocking
			 * @return result of performing the operation
			 *
			 * \~
			 */
			typename std::enable_if <has_shared_lock <M>::value, void>::type _unlockImpl(std::true_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем разделённую разблокировку
					this->_held->unlock_shared();
			}
			/**
			 * \~russian
			 * @brief Шаблон метода уникальной разблокировки при поддержке shared_lock
			 *
			 * @tparam M тип данных мьютекса
			 *
			 * \~english
			 * @brief Template of the method of unique unlocking when shared_lock is supported
			 * @tparam M data type of the mutex
			 *
			 * \~
			 */
			template <typename M = MutexType>
			/**
			 * \~russian
			 * @brief Метод уникальной разблокировки при отсутствии поддержки shared_lock
			 *
			 * @return результат выполнения операции
			 *
			 * \~english
			 * @brief Method of unique unlocking when shared_lock is not supported
			 * @return result of performing the operation
			 *
			 * \~
			 */
			typename std::enable_if <!has_shared_lock <M>::value, void>::type _unlockImpl(std::true_type) noexcept {
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную разблокировку, так как мьютекс не поддерживает разделённую блокировку
					this->_held->unlock();
			}
		private:
			/**
			 * \~russian
			 * @brief Метод выполнения блокировки в зависимости от режима и типа мьютекса
			 *
			 * \~english
			 * @brief Method of performing the locking depending on the mode and on the type of the mutex
			 *
			 * \~
			 */
			void _lock() noexcept {
				/**
				 * Вычисляем константное выражение для определения поддержки shared_lock
				 */
				constexpr bool hasShared = has_shared_lock <MutexType>::value;
				// Если режим блокировки SHARED и мьютекс поддерживает shared_lock
				if((this->_mode == mode_t::SHARED) && hasShared)
					// Выполняем разделённую блокировку
					this->_lockImpl(std::true_type{});
				// Выполняем обычную блокировку
				else this->_lockImpl(std::false_type{});
			}
			/**
			 * \~russian
			 * @brief Метод выполнения разблокировки в зависимости от режима и типа мьютекса
			 *
			 * \~english
			 * @brief Method of performing the unlocking depending on the mode and on the type of the mutex
			 *
			 * \~
			 */
			void _unlock() noexcept {
				/**
				 * Вычисляем константное выражение для определения поддержки shared_lock
				 */
				constexpr bool hasShared = has_shared_lock <MutexType>::value;
				// Если режим блокировки SHARED и мьютекс поддерживает shared_lock
				if((this->_mode == mode_t::SHARED) && hasShared)
					// Выполняем разделённую разблокировку
					this->_unlockImpl(std::true_type{});
				// Выполняем обычную разблокировку
				else this->_unlockImpl(std::false_type{});
			}
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param state объект состояния блокировок
			 * @param mode  режим блокировки (по умолчанию Exclusive для обратной совместимости)
			 *
			 * \~english
			 * @brief Constructor
			 * @param state object of the state of the locks
			 * @param mode  locking mode (Exclusive by default for backward compatibility)
			 *
			 * \~
			 */
			explicit Locker(LockState <MutexType> & state, mode_t mode = mode_t::EXCLUSIVE) noexcept
			 : _locked(false), _mode(mode), _state(state), _held(nullptr) {
				// Если захватывать доступ к памяти нам не нужно (однопоточный режим)
				if(!this->_state._enabled.load(std::memory_order_acquire))
					// Выходим из конструктора, не создавая и не захватывая мьютекс
					return;
				// Лениво создаём (или пересоздаём после fork) рабочий мьютекс и запоминаем его
				this->_held = this->_state._ensure();
				// Выполняем блокировку
				this->_locked = true;
				// Выполняем блокировку потока
				this->_lock();
			}
		public:
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
			~Locker() noexcept {
				// Если мьютекс был захвачен текущим локером
				if(this->_locked && (this->_held != nullptr))
					// Выполняем разблокировку потока
					this->_unlock();
			}
	};
	/**
	 * \~russian
	 * @brief Шаблон формата данных локера
	 *
	 * @tparam MutexType данные локера
	 *
	 * \~english
	 * @brief Template of the data format of the locker
	 * @tparam MutexType data of the locker
	 *
	 * \~
	 */
	template <typename MutexType = std::mutex>
	/**
	 * Создаём тип данных работы с локом
	 */
	using locker_t = Locker <MutexType>;
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "pop.hpp"

#endif // __AWH_LOCKER__
