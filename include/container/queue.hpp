/**
 * @file queue.hpp
 * @date 2025-10-26
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
 * @brief Заголовочный файл бинарной очереди —
 *        класс Queue для последовательного хранения записей произвольного размера в непрерывной памяти с итераторами,
 *        лимитами и невладеющими обёртками доступа
 *
 * \~english
 * @brief Header file of the binary queue —
 *        the Queue class for the sequential storage of records of arbitrary size in contiguous memory with iterators,
 *        limits and non-owning access wrappers
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_QUEUE__
#define __AWH_QUEUE__

/**
 * Если максимальное количество записей очереди не указано
 */
#ifndef AWH_MAX_RECORDS_QUEUE
	/**
	 * Устанавливаем максимальное количество записей очереди 1000
	 */
	#define AWH_MAX_RECORDS_QUEUE 0x3E8
#endif

/**
 * Если максимальное значение потребляемой памяти не указано
 */
#ifndef AWH_MAX_MEMORY_QUEUE
	/**
	 * Устанавливаем максимальное значение потребляемой памяти
	 */
	#define AWH_MAX_MEMORY_QUEUE AWH_MAX_BODY_SIZE
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <cstddef>
#include <condition_variable>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/locker.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
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
	 * @brief Класс бинарной очереди
	 *
	 * \~english
	 * @brief Binary queue class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Queue {
		private:
			/**
			 * \~russian
			 * @brief Структура диапазонов записей
			 *
			 * @note Структура является внутренней и используется для хранения диапазонов записей в очереди
			 *
			 * \~english
			 * @brief Structure of the record ranges
			 *
			 * @note The structure is internal and is used for storing the ranges of the records in the queue
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				size_t end;    // Конец записи
				size_t begin;  // Начало записи
				size_t count;  // Количество добавленных записей
				size_t offset; // Смещение для чтения данных
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Range() noexcept;
			} __attribute__((packed)) range_t;
			/**
			 * \~russian
			 * @brief Структура параметров максимальных значений
			 *
			 * @note Структура является внутренней и используется для хранения максимальных значений очереди
			 *
			 * \~english
			 * @brief Structure of the parameters of the maximum values
			 *
			 * @note The structure is internal and is used for storing the maximum values of the queue
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Max {
				// Максимальный размер выделения памяти (по умолчанию 1 МБ)
				size_t memory;
				// Максимальное количество добавляемых записей (по умолчанию 1000)
				size_t records;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Max() noexcept;
			} __attribute__((packed)) max_t;
		public:
			/**
			 * Создаём тип данных добавляемой записи
			 */
			typedef pair <const void *, size_t> record_t;
			/**
			 * \~russian
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the iterator data type
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Предварительное объявление константного итератора
			 *
			 * \~english
			 * @brief Forward declaration of the constant iterator
			 *
			 * \~
			 */
			class Const_Iterator;
			/**
			 * \~russian
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the iterator data type
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс итератора как вложенный класс
			 *
			 * \~english
			 * @brief Iterator class as a nested class
			 *
			 * \~
			 */
			class Iterator {
				private:
					// Позиция в бинарной очереди
					T * _ptr;
				private:
					/**
					 * \~russian
					 * @brief Разрешаем доступ к позиции константному итератору
					 *
					 * \~english
					 * @brief Granting the constant iterator access to the position
					 *
					 * \~
					 */
					template <typename U> friend class Const_Iterator;
				public:
					/**
					 * \~russian
					 * @brief Оператор разыменования
					 *
					 * @return значение элемента
					 *
					 * \~english
					 * @brief Dereference operator
					 *
					 * @return element value
					 *
					 * \~
					 */
					const T & operator * () const noexcept {
						// Извлекаем значение сдвига итератора
						return * this->_ptr;
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
					 * \~english
					 * @brief Operator of shifting forward
					 *
					 * @return value of the current iterator
					 *
					 * \~
					 */
					Iterator & operator ++ () noexcept {
						// Выполняем смещение текущего значения итератора
						++this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
					/**
					 * \~russian
					 * @brief Оператор смещения назад
					 *
					 * @return значение текущего итератора
					 *
					 * \~english
					 * @brief Operator of shifting backward
					 *
					 * @return value of the current iterator
					 *
					 * \~
					 */
					Iterator & operator -- () noexcept {
						// Выполняем смещение текущего значения итератора
						--this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for equality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator == (const Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for inequality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator != (const Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other константный итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for equality
					 *
					 * @param other constant iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator == (const Const_Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other константный итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for inequality
					 *
					 * @param other constant iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator != (const Const_Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param ptr позиция в контейнере
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param ptr position in the container
					 *
					 * \~
					 */
					explicit Iterator(T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * \~russian
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the iterator data type
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Создаём тип данных итератора
			 *
			 * \~english
			 * @brief Creating the iterator data type
			 *
			 * \~
			 */
			using iterator_t = Iterator <T>;
			/**
			 * \~russian
			 * @brief Шаблон типа данных константного итератора
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the constant iterator data type
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс константного итератора как вложенный класс
			 *
			 * \~english
			 * @brief Constant iterator class as a nested class
			 *
			 * \~
			 */
			class Const_Iterator {
				private:
					// Позиция в бинарной очереди
					const T * _ptr;
				private:
					/**
					 * \~russian
					 * @brief Разрешаем доступ к позиции обычному итератору
					 *
					 * \~english
					 * @brief Granting the ordinary iterator access to the position
					 *
					 * \~
					 */
					template <typename U> friend class Iterator;
				public:
					/**
					 * \~russian
					 * @brief Оператор разыменования
					 *
					 * @return значение элемента
					 *
					 * \~english
					 * @brief Dereference operator
					 *
					 * @return element value
					 *
					 * \~
					 */
					const T & operator * () const noexcept {
						// Извлекаем значение сдвига итератора
						return * this->_ptr;
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
					 * \~english
					 * @brief Operator of shifting forward
					 *
					 * @return value of the current iterator
					 *
					 * \~
					 */
					Const_Iterator & operator ++ () noexcept {
						// Выполняем смещение текущего значения итератора
						++this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
					/**
					 * \~russian
					 * @brief Оператор смещения назад
					 *
					 * @return значение текущего итератора
					 *
					 * \~english
					 * @brief Operator of shifting backward
					 *
					 * @return value of the current iterator
					 *
					 * \~
					 */
					Const_Iterator & operator -- () noexcept {
						// Выполняем смещение текущего значения итератора
						--this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
				public:
					/**
					 * \~russian
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for equality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator == (const Const_Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for inequality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator != (const Const_Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for equality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator == (const Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * \~russian
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 * \~english
					 * @brief Operator of comparing the iterators for inequality
					 *
					 * @param other iterator to compare with
					 * @return      comparison result
					 *
					 * \~
					 */
					bool operator != (const Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param ptr позиция в контейнере
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param ptr position in the container
					 *
					 * \~
					 */
					explicit Const_Iterator(const T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * \~russian
			 * @brief Шаблон типа данных константного итератора
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the constant iterator data type
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Создаём тип данных константного итератора
			 *
			 * \~english
			 * @brief Creating the constant iterator data type
			 *
			 * \~
			 */
			using const_iterator_t = Const_Iterator <T>;
		private:
			// Размеры максимальныйх ограничений
			max_t _max;
			// Объект диапазонов записей
			range_t _range;
		private:
			// Буфер данных выделенной памяти
			vector <uint8_t> _buffer;
		private:
			// Условная переменная ожидания появления данных в очереди
			mutable condition_variable _cv;
			// Объект состояния блокировки для обеспечения потокобезопасности
			mutable lock_state_t <std::mutex> _mtx;
		private:
			// Объект фреймворка
			const fmk_t * _fmk = nullptr;
			// Объект работы с логами
			const log_t * _log = nullptr;
		private:
			/**
			 * \~russian
			 * @brief Метод контроля памяти
			 *
			 * @param size желаемый размер выделения памяти
			 * @return     результат выполнения операции
			 *
			 * \~english
			 * @brief Memory control method
			 *
			 * @param size desired size of the memory allocation
			 * @return     result of performing the operation
			 *
			 * \~
			 */
			bool rss(const size_t size) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод вывода сообщения об ошибке
			 *
			 * @param func    название функции, в которой произошла ошибка
			 * @param size    размер данных, связанный с ошибкой
			 * @param message текст сообщения об ошибке
			 *
			 * \~english
			 * @brief Method of outputting an error message
			 *
			 * @param func    name of the function the error occurred in
			 * @param size    data size associated with the error
			 * @param message text of the error message
			 *
			 * \~
			 */
			void error(const char * func, const size_t size, const char * message) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления записи в очереди
			 *
			 * \~english
			 * @brief Method removing a record from the queue
			 *
			 * \~
			 */
			void pop() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки всех данных очереди
			 *
			 * \~english
			 * @brief Method clearing all the data of the queue
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод полной очистки памяти
			 *
			 * \~english
			 * @brief Method of the complete clearing of the memory
			 *
			 * \~
			 */
			void reset() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Количество добавленных элементов
			 *
			 * @return количество добавленных элементов
			 *
			 * \~english
			 * @brief Number of the added elements
			 *
			 * @return number of the added elements
			 *
			 * \~
			 */
			size_t count() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения размера добавленных данных
			 *
			 * @return размер добавленных данных
			 *
			 * \~english
			 * @brief Method obtaining the size of the added data
			 *
			 * @return size of the added data
			 *
			 * \~
			 */
			size_t size() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода размера занимаемой памяти очередью
			 *
			 * @return количество памяти которую занимает очередь
			 *
			 * \~english
			 * @brief Method of outputting the size of the memory occupied by the queue
			 *
			 * @return amount of memory the queue occupies
			 *
			 * \~
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для метода получения конечного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the end iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения конечного итератора
			 *
			 * @return конечный итератор
			 *
			 * \~english
			 * @brief Method obtaining the end iterator
			 *
			 * @return end iterator
			 *
			 * \~
			 */
			iterator_t <T> end() noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода получение начального итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the begin iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получение начального итератора
			 *
			 * @return начальный итератор
			 *
			 * \~english
			 * @brief Method obtaining the begin iterator
			 *
			 * @return begin iterator
			 *
			 * \~
			 */
			iterator_t <T> begin() noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the constant end iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 *
			 * \~english
			 * @brief Method obtaining the constant end iterator
			 *
			 * @return constant end iterator
			 *
			 * \~
			 */
			const_iterator_t <T> end() const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the constant end iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 *
			 * \~english
			 * @brief Method obtaining the constant end iterator
			 *
			 * @return constant end iterator
			 *
			 * \~
			 */
			const_iterator_t <T> cend() const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the constant begin iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 *
			 * \~english
			 * @brief Method obtaining the constant begin iterator
			 *
			 * @return constant begin iterator
			 *
			 * \~
			 */
			const_iterator_t <T> begin() const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the constant begin iterator
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 *
			 * \~english
			 * @brief Method obtaining the constant begin iterator
			 *
			 * @return constant begin iterator
			 *
			 * \~
			 */
			const_iterator_t <T> cbegin() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получения данных указанного элемента в очереди
			 *
			 * @return указатель на элемент очереди
			 *
			 * \~english
			 * @brief Obtaining the data of the specified element in the queue
			 *
			 * @return pointer to the element of the queue
			 *
			 * \~
			 */
			const void * data() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод фиксации прочитанного размера данных
			 *
			 * @param size размер данных для фиксации
			 *
			 * \~english
			 * @brief Method of committing the read data size
			 *
			 * @param size data size to commit
			 *
			 * \~
			 */
			void commit(const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на заполненность очереди
			 *
			 * @note При установленном таймауте и активной потокобезопасности метод блокирует
			 *       поток до появления данных в очереди либо до истечения времени ожидания.
			 *
			 * @param timeout время ожидания в миллисекундах
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method checking the queue for fullness
			 *
			 * @note With a timeout set and thread safety active the method blocks the thread
			 *       until data appears in the queue or until the waiting time expires.
			 *
			 * @param timeout waiting time in milliseconds
			 * @return        check result
			 *
			 * \~
			 */
			bool empty(const uint32_t timeout = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 * @return       текущий размер очереди
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the queue
			 *
			 * @param buffer binary buffer to add
			 * @param size   size of the binary buffer
			 * @return       current size of the queue
			 *
			 * \~
			 */
			size_t push(const void * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param records список бинарных буферов для добавления
			 * @param size    общий размер добавляемых данных
			 * @return        текущий размер очереди
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the queue
			 *
			 * @param records list of binary buffers to add
			 * @param size    total size of the added data
			 * @return        current size of the queue
			 *
			 * \~
			 */
			size_t push(const vector <record_t> & records, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки максимального размера потребления памяти
			 *
			 * @param size максимальный размер потребления памяти
			 *
			 * \~english
			 * @brief Method setting the maximum size of the memory consumption
			 *
			 * @param size maximum size of the memory consumption
			 *
			 * \~
			 */
			void setMaxMemory(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального количества записей очереди
			 *
			 * @param count максимальное количество записей очереди
			 *
			 * \~english
			 * @brief Method setting the maximum number of the records of the queue
			 *
			 * @param count maximum number of the records of the queue
			 *
			 * \~
			 */
			void setMaxRecords(const size_t count) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод обмена очередями
			 *
			 * @param queue очередь для обмена
			 *
			 * \~english
			 * @brief Method of exchanging the queues
			 *
			 * @param queue queue for the exchange
			 *
			 * \~
			 */
			void swap(Queue & queue) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности работы потоков
			 *
			 * \~english
			 * @brief Method setting the safety of the work of the threads
			 *
			 * @param mode flag of the mode of the safety of the work of the threads
			 *
			 * \~
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 * \~english
			 * @brief Method setting the logging object
			 *
			 * @param log object for working with logs
			 *
			 * \~
			 */
			void setLogger(const log_t * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получения размера данных в очереди
			 *
			 * @return размер данных в очереди
			 *
			 * \~english
			 * @brief Obtaining the size of the data in the queue
			 *
			 * @return size of the data in the queue
			 *
			 * \~
			 */
			operator size_t() const noexcept;
			/**
			 * \~russian
			 * @brief Получения бинарных данных очереди
			 *
			 * @return бинарные данные очереди
			 *
			 * \~english
			 * @brief Obtaining the binary data of the queue
			 *
			 * @return binary data of the queue
			 *
			 * \~
			 */
			operator const char * () const noexcept;
			/**
			 * \~russian
			 * @brief Получения бинарных данных очереди
			 *
			 * @return бинарные данные очереди
			 *
			 * \~english
			 * @brief Obtaining the binary data of the queue
			 *
			 * @return binary data of the queue
			 *
			 * \~
			 */
			operator const uint8_t * () const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения
			 *
			 * @param queue очередь для перемещения
			 * @return      текущий контейнер очереди
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * @param queue queue to move
			 * @return      current queue container
			 *
			 * \~
			 */
			Queue & operator = (Queue && queue) noexcept;
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @param queue очередь для копирования
			 * @return      текущий контейнер очереди
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @param queue queue to copy
			 * @return      current queue container
			 *
			 * \~
			 */
			Queue & operator = (const Queue & queue) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор сравнения двух очередей
			 *
			 * @param queue очередь для сравнения
			 * @return      результат сравнения
			 *
			 * \~english
			 * @brief Operator of comparing two queues
			 *
			 * @param queue queue to compare with
			 * @return      comparison result
			 *
			 * \~
			 */
			bool operator == (const Queue & queue) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Разрешаем пустое значение объекта
			 *
			 * \~english
			 * @brief Permitting an empty value of the object
			 *
			 * \~
			 */
			explicit Queue() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор перемещения
			 *
			 * @param queue очередь для перемещения
			 *
			 * \~english
			 * @brief Move constructor
			 *
			 * @param queue queue to move
			 *
			 * \~
			 */
			explicit Queue(Queue && queue) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * @param queue очередь для копирования
			 *
			 * \~english
			 * @brief Copy constructor
			 *
			 * @param queue queue to copy
			 *
			 * \~
			 */
			explicit Queue(const Queue & queue) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Queue(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Queue() noexcept;
		private:
			/**
			 * \~russian
			 * @brief Шаблон типа данных обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the queue wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс обёртки очереди
			 *
			 * \~english
			 * @brief Queue wrapper class
			 *
			 * \~
			 */
			class view {
				// Очередь которую необходимо обернуть
				Queue & _queue;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор очереди
				 *
				 * \~english
				 * @brief Method obtaining the end iterator
				 *
				 * @return end iterator of the queue
				 *
				 * \~
				 */
				auto end() noexcept -> decltype(this->_queue.template end <T> ()) {
					// Возвращаем конечный итератор очереди
					return this->_queue.template end <T> ();
				}
				/**
				 * \~russian
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор очереди
				 *
				 * \~english
				 * @brief Method obtaining the begin iterator
				 *
				 * @return begin iterator of the queue
				 *
				 * \~
				 */
				auto begin() noexcept -> decltype(this->_queue.template begin <T> ()) {
					// Возвращаем начальный итератор очереди
					return this->_queue.template begin <T> ();
				}
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param queue обёртываемая очередь
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param queue queue being wrapped
				 *
				 * \~
				 */
				explicit view(Queue & queue) noexcept : _queue(queue) {}
			};
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа данных обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the queue wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод обёртки бинарной очереди
			 *
			 * @return обёрнутая бинарная очередь
			 *
			 * \~english
			 * @brief Method of wrapping the binary queue
			 *
			 * @return wrapped binary queue
			 *
			 * \~
			 */
			view <T> as() & {
				// Возвращаем очередь по ссылке — только для lvalue
				return view <T> (* this);
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон типа данных константной обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the constant queue wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс константной обёртки очереди
			 *
			 * \~english
			 * @brief Constant queue wrapper class
			 *
			 * \~
			 */
			class const_view {
				// Очередь которую необходимо обернуть
				const Queue & _queue;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор очереди
				 *
				 * \~english
				 * @brief Method obtaining the end iterator
				 *
				 * @return end iterator of the queue
				 *
				 * \~
				 */
				auto end() const noexcept -> decltype(this->_queue.template end <T> ()) {
					// Возвращаем конечный итератор очереди
					return this->_queue.template end <T> ();
				}
				/**
				 * \~russian
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор очереди
				 *
				 * \~english
				 * @brief Method obtaining the begin iterator
				 *
				 * @return begin iterator of the queue
				 *
				 * \~
				 */
				auto begin() const noexcept -> decltype(this->_queue.template begin <T> ()) {
					// Возвращаем начальный итератор очереди
					return this->_queue.template begin <T> ();
				}
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param queue обёртываемая очередь
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param queue queue being wrapped
				 *
				 * \~
				 */
				explicit const_view(const Queue & queue) noexcept : _queue(queue) {}
			};
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа данных константной обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the constant queue wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод константной обёртки бинарной очереди
			 *
			 * @return обёрнутая бинарная очередь
			 *
			 * \~english
			 * @brief Method of the constant wrapping of the binary queue
			 *
			 * @return wrapped binary queue
			 *
			 * \~
			 */
			const_view <T> as() const & {
				// Возвращаем очередь по константной ссылке — только для lvalue
				return const_view <T> (* this);
			}
	} queue_t;
};

#endif // __AWH_QUEUE__
