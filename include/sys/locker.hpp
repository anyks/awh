/**
 * @file: locker.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Шаблон trait для определения поддержки shared_lock
	 *
	 * @note Работает в C++17 через SFINAE
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
	 *
	 * @note Работает в C++17 через SFINAE
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
	 * @brief Класс свойства булевого значения с поддержкой функции обратного вызова при изменении значения
	 *
	 */
	typedef class Enabled_Property {
		private:
			// Значение свойства
			std::atomic_bool _value;
		private:
			/**
			 * @brief Функция обратного вызова для дополнительных действий при изменении значения свойства
			 *
			 * @param value новое значение свойства
			 */
			function <void (bool)> _callback;
		public:
			/**
			 * @brief Оператор преобразования к булевому типу для получения текущего значения свойства
			 *
			 * @return текущее значение свойства
			 */
			operator bool() const noexcept {
				// Возвращаем текущее значение свойства
				return this->_value.load(std::memory_order_acquire);
			}
		public:
			/**
			 * @brief Оператор сравнения несоответствия значения свойства с заданным булевым значением
			 *
			 * @param value булевое значение для сравнения
			 * @return      результат сравнения
			 */
			bool operator != (const bool value) const noexcept {
				// Сравниваем текущее значение свойства с заданным значением
				return (value != this->_value.load(std::memory_order_acquire));
			}
			/**
			 * @brief Оператор сравнения соответствия значения свойства с заданным булевым значением
			 *
			 * @param value булевое значение для сравнения
			 * @return      результат сравнения
			 */
			bool operator == (const bool value) const noexcept {
				// Сравниваем текущее значение свойства с заданным значением
				return (value == this->_value.load(std::memory_order_acquire));
			}
		public:
			/**
			 * @brief Оператор присваивания для копирования значения свойства из другого свойства
			 *
			 * @param other другое свойство для копирования значения
			 * @return      ссылка на текущий объект для цепочки присваиваний
			 */
			Enabled_Property & operator = (const Enabled_Property & other) noexcept {
				// Копируем значение из другого свойства
				return (* this) = static_cast <bool> (other);
			}
		public:
			/**
			 * @brief Оператор присваивания для изменения значения свойства
			 *
			 * @param value новое значение свойства
			 * @return      ссылка на текущий объект для цепочки присваиваний
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
			 * @brief Конструктор
			 *
			 * @param value    начальное значение свойства
			 * @param callback функция обратного вызова для дополнительных действий при изменении значения 
			 */
			Enabled_Property(const bool value, function <void (bool)> callback = nullptr) noexcept :
			 _value(value), _callback(callback) {}
	} enabled_property_t;

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
			 * @brief Гарантирует существование рабочего мьютекса с учётом смены процесса
			 *
			 * @note Выполняет ленивое создание мьютекса и его пересоздание после fork
			 *
			 * @return указатель на актуальный рабочий мьютекс
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
			 * @brief Метод, который будет вызван при изменении флага активации/деактивации блокировок
			 *
			 * @param value новое значение флага
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
			 * @brief Оператор преобразования к мютексу
			 *
			 * @note Лениво создаёт мьютекс по требованию (например, для condition_variable)
			 *
			 */
			operator MutexType & () noexcept {
				// Лениво создаём (или пересоздаём после fork) рабочий мьютекс
				MutexType & mtx = (* this->_ensure());
				// Возвращаем мютекс для блокировки потока
				return mtx;
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
			 _enabled(false), _pid(::getpid()), enabled {
				// Активируем блокировки по умолчанию при создании объекта
				true,
				/**
				 * @brief Функция обратного вызова для изменения флага активации/деактивации блокировок
				 *
				 * @param value новое значение флага
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
		private:
			// Указатель на мьютекс, который был фактически захвачен текущим локером
			MutexType * _held;
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную блокировку
					this->_held->lock();
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную разблокировку
					this->_held->unlock();
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем разделённую блокировку
					this->_held->lock_shared();
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную блокировку, так как мьютекс не поддерживает разделённую блокировку
					this->_held->lock();
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем разделённую разблокировку
					this->_held->unlock_shared();
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
				// Если мьютекс существует
				if(this->_held != nullptr)
					// Выполняем обычную разблокировку, так как мьютекс не поддерживает разделённую блокировку
					this->_held->unlock();
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
				constexpr bool hasShared = has_shared_lock <MutexType>::value;
				// Если режим блокировки SHARED и мьютекс поддерживает shared_lock
				if((this->_mode == mode_t::SHARED) && hasShared)
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
			 * @brief Конструктор
			 *
			 * @param state объект состояния блокировок
			 * @param mode  режим блокировки (по умолчанию Exclusive для обратной совместимости)
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
			 * @brief Деструктор
			 *
			 */
			~Locker() noexcept {
				// Если мьютекс был захвачен текущим локером
				if(this->_locked && (this->_held != nullptr))
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
