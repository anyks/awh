/**
 * @file: buffer.hpp
 * @date: 2025-10-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл бинарного смарт-буфера — класс Buffer с раздельным учётом диапазонов записей,
 *        прямым и константным итераторами,
 *        транзакционной записью с откатом и невладеющими обёртками для доступа к данным без копирования
 *
 * \~english
 * @brief Header file of the binary smart buffer — the Buffer class with separate accounting of the record ranges,
 *        a direct and a constant iterator,
 *        transactional writing with rollback and non-owning wrappers for accessing the data without copying
 *
 * \~
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_BUFFER__
#define __AWH_BUFFER__

/**
 * Если максимальное значение потребляемой памяти не указано
 */
#ifndef AWH_MAX_MEMORY_BUFFER
	/**
	 * Устанавливаем максимальное значение потребляемой памяти
	 */
	#define AWH_MAX_MEMORY_BUFFER AWH_MAX_BODY_SIZE
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstddef>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

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
	 * @brief Класс бинарного смартбуфера
	 *
	 * \~english
	 * @brief Binary smart buffer class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Buffer {
		private:
			/**
			 * \~russian
			 * @brief Структура диапазонов записей
			 *
			 * \~english
			 * @brief Structure of the record ranges
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				// Конец записи
				size_t end;
				// Начало записи
				size_t begin;
				// Объём места, зарезервированного последним вызовом prepare и ещё не зафиксированного
				size_t reserved;
				// Максимальный размер выделения памяти (по умолчанию AWH_MAX_MEMORY_BUFFER)
				size_t maxMemory;
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
		public:
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
					// Позиция в бинарном буфере
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
					 * @return значение заголовка
					 *
					 * \~english
					 * @brief Dereference operator
					 *
					 * @return header value
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
					// Позиция в бинарном буфере
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
					 * @return значение заголовка
					 *
					 * \~english
					 * @brief Dereference operator
					 *
					 * @return header value
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
		public:
			/**
			 * \~russian
			 * @brief Класс RAII-обёртки для безопасной прямой записи в хвост буфера (zero-copy)
			 *
			 * @details Резервирует место в хвосте буфера, предоставляет указатель для прямой
			 *          записи (например, из сокета) и автоматически фиксирует записанные данные
			 *          при выходе из области видимости. Если фиксация не запрошена через commit(),
			 *          ничего не добавляется (безопасный откат).
			 *
			 * \~english
			 * @brief Class of the RAII wrapper for the safe direct writing into the tail of the buffer (zero-copy)
			 *
			 * @details Reserves room in the tail of the buffer, provides a pointer for direct writing
			 *          (for example, from a socket) and automatically commits the written data upon
			 *          leaving the scope. If the commit has not been requested through commit(),
			 *          nothing is added (a safe rollback).
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ Writer {
				private:
					// Признак выполненной фиксации
					bool _applied;
					// Размер зарезервированной области
					size_t _capacity;
					// Количество байт для фиксации
					size_t _committed;
					// Указатель на зарезервированную область
					void * _data;
					// Буфер для которого выполнено резервирование
					Buffer * _buffer;
				public:
					/**
					 * \~russian
					 * @brief Метод получения указателя на зарезервированную область
					 *
					 * @return указатель для записи данных либо nullptr при ошибке резервирования
					 *
					 * \~english
					 * @brief Method obtaining the pointer to the reserved area
					 *
					 * @return pointer for writing the data or nullptr upon a reservation error
					 *
					 * \~
					 */
					void * get() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения размера зарезервированной области
					 *
					 * @return размер доступного для записи места
					 *
					 * \~english
					 * @brief Method obtaining the size of the reserved area
					 *
					 * @return size of the room available for writing
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки корректности резервирования
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method checking the correctness of the reservation
					 *
					 * @return check result
					 *
					 * \~
					 */
					bool valid() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод отмены записи (зафиксировано не будет ничего)
					 *
					 * \~english
					 * @brief Method cancelling the writing (nothing will be committed)
					 *
					 * \~
					 */
					void cancel() noexcept;
					/**
					 * \~russian
					 * @brief Метод немедленной фиксации записанных данных в буфер
					 *
					 * @return количество зафиксированных байт
					 *
					 * \~english
					 * @brief Method of the immediate committing of the written data into the buffer
					 *
					 * @return number of the committed bytes
					 *
					 * \~
					 */
					size_t apply() noexcept;
					/**
					 * \~russian
					 * @brief Метод указания количества фактически записанных байт
					 *
					 * @param size количество записанных байт
					 * @return     количество байт которое будет зафиксировано
					 *
					 * \~english
					 * @brief Method of stating the number of the actually written bytes
					 *
					 * @param size number of the written bytes
					 * @return     number of bytes that will be committed
					 *
					 * \~
					 */
					size_t commit(const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Запрещаем копирование обёртки
					 *
					 * \~english
					 * @brief Copying of the wrapper is forbidden
					 *
					 * \~
					 */
					Writer(const Writer &) = delete;
					/**
					 * \~russian
					 * @brief Запрещаем присвоение обёртки
					 *
					 * \~english
					 * @brief Assignment of the wrapper is forbidden
					 *
					 * \~
					 */
					Writer & operator = (const Writer &) = delete;
				public:
					/**
					 * \~russian
					 * @brief Конструктор перемещения
					 *
					 * @param other обёртка для перемещения
					 *
					 * \~english
					 * @brief Move constructor
					 *
					 * @param other wrapper to move
					 *
					 * \~
					 */
					Writer(Writer && other) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param buffer   буфер для записи
					 * @param data     указатель на зарезервированную область
					 * @param capacity размер зарезервированной области
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param buffer   buffer for writing
					 * @param data     pointer to the reserved area
					 * @param capacity size of the reserved area
					 *
					 * \~
					 */
					explicit Writer(Buffer * buffer, void * data, const size_t capacity) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор (автоматически фиксирует записанные данные)
					 *
					 * \~english
					 * @brief Destructor (automatically commits the written data)
					 *
					 * \~
					 */
					~Writer() noexcept;
			};
		private:
			// Объект диапазонов записей
			range_t _range;
		private:
			// Буфер данных выделенной памяти
			vector <uint8_t> _buffer;
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
			 * @details Гарантирует наличие как минимум size свободных байт в хвосте буфера.
			 *          При необходимости переиспользует уже извлечённое место в начале буфера
			 *          (компактизация) и/или увеличивает буфер в пределах максимального лимита.
			 *
			 * @param size желаемый размер свободного места в хвосте буфера
			 * @return     результат выполнения операции
			 *
			 * \~english
			 * @brief Memory control method
			 *
			 * @details Guarantees the presence of at least size free bytes in the tail of the buffer.
			 *          Where necessary reuses the room already extracted at the beginning of the buffer
			 *          (compaction) and/or grows the buffer within the maximum limit.
			 *
			 * @param size desired size of the free room in the tail of the buffer
			 * @return     result of performing the operation
			 *
			 * \~
			 */
			bool rss(const size_t size) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод вывода сообщения об ошибке в лог
			 *
			 * @param func    название функции в которой произошла ошибка
			 * @param message текст сообщения об ошибке
			 * @param flag    флаг важности сообщения
			 *
			 * \~english
			 * @brief Method of outputting an error message into the log
			 *
			 * @param func    name of the function the error occurred in
			 * @param message text of the error message
			 * @param flag    flag of the importance of the message
			 *
			 * \~
			 */
			void error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки всех данных буфера
			 *
			 * \~english
			 * @brief Method clearing all the data of the buffer
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
			 * @brief Метод проверки на заполненность буфера
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method checking the buffer for fullness
			 *
			 * @return check result
			 *
			 * \~
			 */
			bool empty() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения размера добавленных данных
			 *
			 * @return размер всех добавленных данных
			 *
			 * \~english
			 * @brief Method obtaining the size of the added data
			 *
			 * @return size of all the added data
			 *
			 * \~
			 */
			size_t size() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода размера занимаемой памяти очередью
			 *
			 * @return количество памяти которую занимает буфер
			 *
			 * \~english
			 * @brief Method of outputting the size of the memory occupied by the queue
			 *
			 * @return amount of memory the buffer occupies
			 *
			 * \~
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения буфера сырых данных
			 *
			 * @details Нормализует внутреннее хранилище: переносит полезные данные в начало
			 *          и усекает буфер до их размера, после чего возвращает его как есть.
			 *
			 * @return буфер сырых данных
			 *
			 * \~english
			 * @brief Method of extracting the raw data buffer
			 *
			 * @details Normalizes the internal storage: moves the useful data to the beginning
			 *          and truncates the buffer to their size, after which returns it as it is.
			 *
			 * @return raw data buffer
			 *
			 * \~
			 */
			const vector <uint8_t> & raw() const noexcept;
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
			 * @brief Шаблон для метода удаления верхних записей
			 *
			 * @tparam T тип данных для удаления
			 *
			 * \~english
			 * @brief Template for the method removing the top records
			 *
			 * @tparam T data type for the removal
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод удаления записи в буфера
			 *
			 * \~english
			 * @brief Method removing a record from the buffer
			 *
			 * \~
			 */
			void pop() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для метода получения количества элементов в бинарном буфере
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 * \~english
			 * @brief Template for the method obtaining the number of elements in the binary buffer
			 *
			 * @tparam T data type for the counting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод получения количества элементов в бинарном буфере
			 *
			 * @return количество всех добавленных лементов
			 *
			 * \~english
			 * @brief Method obtaining the number of elements in the binary buffer
			 *
			 * @return number of all the added elements
			 *
			 * \~
			 */
			size_t count() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для метода извлечения нижнего значения в буфере
			 *
			 * @tparam T тип данных для извлечения
			 *
			 * \~english
			 * @brief Template for the method extracting the bottom value in the buffer
			 *
			 * @tparam T data type for the extraction
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения нижнего значения в буфере
			 *
			 * @return данные содержащиеся в буфере
			 *
			 * \~english
			 * @brief Method extracting the bottom value in the buffer
			 *
			 * @return data contained in the buffer
			 *
			 * \~
			 */
			T back() const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон для метода извлечения верхнего значения в буфере
			 *
			 * @tparam T тип данных для извлечения
			 *
			 * \~english
			 * @brief Template for the method extracting the top value in the buffer
			 *
			 * @tparam T data type for the extraction
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения верхнего значения в буфере
			 *
			 * @return данные содержащиеся в буфере
			 *
			 * \~english
			 * @brief Method extracting the top value in the buffer
			 *
			 * @return data contained in the buffer
			 *
			 * \~
			 */
			T front() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для метода извлечения содержимого контейнера по его индексу
			 *
			 * @tparam T тип данных для извлечения
			 *
			 * \~english
			 * @brief Template for the method extracting the content of the container by its index
			 *
			 * @tparam T data type for the extraction
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения содержимого контейнера по его индексу
			 *
			 * @param index индекс массива для извлечения
			 * @return      данные содержащиеся в буфере
			 *
			 * \~english
			 * @brief Method extracting the content of the container by its index
			 *
			 * @param index index of the array for the extraction
			 * @return      data contained in the buffer
			 *
			 * \~
			 */
			T at(const size_t index) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для метода установки значений в уже существующем буфере
			 *
			 * @tparam T тип данных для установки
			 *
			 * \~english
			 * @brief Template for the method setting the values in an already existing buffer
			 *
			 * @tparam T data type for the setting
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки значений в уже существующем буфере
			 *
			 * @param value значение для установки
			 * @param index индекс значения для установки
			 *
			 * \~english
			 * @brief Method setting the values in an already existing buffer
			 *
			 * @param value value to set
			 * @param index index of the value to set
			 *
			 * \~
			 */
			void set(const T value, const size_t index) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Получения данных указанного элемента в буфера
			 *
			 * @return указатель на элемент буфера
			 *
			 * \~english
			 * @brief Obtaining the data of the specified element in the buffer
			 *
			 * @return pointer to the element of the buffer
			 *
			 * \~
			 */
			const void * data() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления указанного количества байт из начала буфера
			 *
			 * @param size количество байт для удаления
			 *
			 * \~english
			 * @brief Method removing the specified number of bytes from the beginning of the buffer
			 *
			 * @param size number of bytes to remove
			 *
			 * \~
			 */
			void erase(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения (удаления) указанного количества уже обработанных байт из начала буфера
			 *
			 * @param size количество байт для извлечения
			 *
			 * \~english
			 * @brief Method extracting (removing) the specified number of already processed bytes from the beginning of the buffer
			 *
			 * @param size number of bytes to extract
			 *
			 * \~
			 */
			void consume(const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подготовки места в хвосте буфера для прямой записи (zero-copy)
			 *
			 * @details После записи данных по полученному указателю необходимо вызвать commit(n).
			 *          Указатель действителен только до следующей модификации буфера.
			 *          Для безопасной работы рекомендуется использовать метод write().
			 *
			 * @param size требуемое количество свободных байт в хвосте буфера
			 * @return     указатель на начало свободной области либо nullptr при ошибке
			 *
			 * \~english
			 * @brief Method preparing room in the tail of the buffer for direct writing (zero-copy)
			 *
			 * @details After writing the data by the obtained pointer commit(n) must be called.
			 *          The pointer is valid only until the next modification of the buffer.
			 *          For safe work it is recommended to use the write() method.
			 *
			 * @param size required number of free bytes in the tail of the buffer
			 * @return     pointer to the beginning of the free area or nullptr upon an error
			 *
			 * \~
			 */
			void * prepare(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод фиксации записанных в хвост буфера данных (zero-copy)
			 *
			 * @param size количество фактически записанных в хвост байт
			 * @return     количество зафиксированных байт
			 *
			 * \~english
			 * @brief Method of committing the data written into the tail of the buffer (zero-copy)
			 *
			 * @param size number of the bytes actually written into the tail
			 * @return     number of the committed bytes
			 *
			 * \~
			 */
			size_t commit(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения RAII-обёртки для безопасной прямой записи в хвост буфера (zero-copy)
			 *
			 * @param size требуемое количество свободных байт в хвосте буфера
			 * @return     объект записи с автоматической фиксацией данных
			 *
			 * \~english
			 * @brief Method obtaining the RAII wrapper for the safe direct writing into the tail of the buffer (zero-copy)
			 *
			 * @param size required number of free bytes in the tail of the buffer
			 * @return     writing object with the automatic committing of the data
			 *
			 * \~
			 */
			Writer write(const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод резервирования размера буфера
			 *
			 * @param size размер выделяемой памяти
			 *
			 * \~english
			 * @brief Method reserving the size of the buffer
			 *
			 * @param size size of the allocated memory
			 *
			 * \~
			 */
			void reserve(const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон для добавления числа в буфер
			 *
			 * @tparam T тип данных для добавления
			 *
			 * \~english
			 * @brief Template for adding a number into the buffer
			 *
			 * @tparam T data type for the addition
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления числа в буфер
			 *
			 * @param value значение для добавления
			 * @return       результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a number into the buffer
			 *
			 * @param value value to add
			 * @return       result of adding the data
			 *
			 * \~
			 */
			bool push(const T value) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a text into the buffer
			 *
			 * @param text text to add
			 * @return     result of adding the data
			 *
			 * \~
			 */
			bool push(const char * text) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a text into the buffer
			 *
			 * @param text text to add
			 * @return     result of adding the data
			 *
			 * \~
			 */
			bool push(string_view text) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a text into the buffer
			 *
			 * @param text text to add
			 * @return     result of adding the data
			 *
			 * \~
			 */
			bool push(const string & text) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @details Если текущий буфер пуст — выполняется перемещение хранилища (zero-copy),
			 *          иначе данные дописываются в хвост.
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the buffer
			 *
			 * @details If the current buffer is empty — the storage is moved (zero-copy),
			 *          otherwise the data is appended to the tail.
			 *
			 * @param buffer binary buffer to add
			 * @return       result of adding the data
			 *
			 * \~
			 */
			bool push(Buffer && buffer) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the buffer
			 *
			 * @param buffer binary buffer to add
			 * @return       result of adding the data
			 *
			 * \~
			 */
			bool push(const Buffer & buffer) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the buffer
			 *
			 * @param buffer binary buffer to add
			 * @return       result of adding the data
			 *
			 * \~
			 */
			bool push(const vector <uint8_t> & buffer) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 * @return       результат добавления данных
			 *
			 * \~english
			 * @brief Method adding a binary data buffer into the buffer
			 *
			 * @param buffer binary buffer to add
			 * @param size   size of the binary buffer
			 * @return       result of adding the data
			 *
			 * \~
			 */
			bool push(const void * buffer, const size_t size) noexcept;
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
		public:
			/**
			 * \~russian
			 * @brief Метод обмена очередями
			 *
			 * @param buffer бинарный буфер для обмена
			 *
			 * \~english
			 * @brief Method of exchanging the queues
			 *
			 * @param buffer binary buffer for the exchange
			 *
			 * \~
			 */
			void swap(Buffer & buffer) noexcept;
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
			 * @brief Получения размера данных в буфера
			 *
			 * @return размер данных в буфера
			 *
			 * \~english
			 * @brief Obtaining the size of the data in the buffer
			 *
			 * @return size of the data in the buffer
			 *
			 * \~
			 */
			operator size_t() const noexcept;
			/**
			 * \~russian
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 *
			 * \~english
			 * @brief Obtaining the binary data of the buffer
			 *
			 * @return binary data of the buffer
			 *
			 * \~
			 */
			operator const char * () const noexcept;
			/**
			 * \~russian
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 *
			 * \~english
			 * @brief Obtaining the binary data of the buffer
			 *
			 * @return binary data of the buffer
			 *
			 * \~
			 */
			operator const uint8_t * () const noexcept;
			/**
			 * \~russian
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 *
			 * \~english
			 * @brief Obtaining the binary data of the buffer
			 *
			 * @return binary data of the buffer
			 *
			 * \~
			 */
			operator const vector <uint8_t> & () const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @param buffer binary buffer to copy
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (const char * buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * @param buffer binary buffer to move
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (string && buffer) noexcept;
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @param buffer binary buffer to copy
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (const string & buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * @param buffer binary buffer to move
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (vector <uint8_t> && buffer) noexcept;
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @param buffer binary buffer to copy
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (const vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * @param buffer binary buffer to move
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (Buffer && buffer) noexcept;
			/**
			 * \~russian
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Copy assignment operator
			 *
			 * @param buffer binary buffer to copy
			 * @return       current buffer container
			 *
			 * \~
			 */
			Buffer & operator = (const Buffer & buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор сравнения двух очередей
			 *
			 * @param buffer бинарный буфер для сравнения
			 * @return       результат сравнения
			 *
			 * \~english
			 * @brief Operator of comparing two queues
			 *
			 * @param buffer binary buffer to compare with
			 * @return       comparison result
			 *
			 * \~
			 */
			bool operator == (const Buffer & buffer) const noexcept;
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
			explicit Buffer() noexcept;
			/**
			 * \~russian
			 * @brief Конструктор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 *
			 * \~english
			 * @brief Move constructor
			 *
			 * @param buffer binary buffer to move
			 *
			 * \~
			 */
			explicit Buffer(Buffer && buffer) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 *
			 * \~english
			 * @brief Copy constructor
			 *
			 * @param buffer binary buffer to copy
			 *
			 * \~
			 */
			explicit Buffer(const Buffer & buffer) noexcept;
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
			explicit Buffer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Buffer() noexcept;
		private:
			/**
			 * \~russian
			 * @brief Шаблон типа данных обёртки буфера
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the buffer wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс обёртки буфера
			 *
			 * \~english
			 * @brief Buffer wrapper class
			 *
			 * \~
			 */
			class view {
				// Буфер который необходимо обернуть
				Buffer & _buffer;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор буфера
				 *
				 * \~english
				 * @brief Method obtaining the end iterator
				 *
				 * @return end iterator of the buffer
				 *
				 * \~
				 */
				auto end() noexcept -> decltype(this->_buffer.template end <T> ()) {
					// Возвращаем конечный итератор буфера
					return this->_buffer.template end <T> ();
				}
				/**
				 * \~russian
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор буфера
				 *
				 * \~english
				 * @brief Method obtaining the begin iterator
				 *
				 * @return begin iterator of the buffer
				 *
				 * \~
				 */
				auto begin() noexcept -> decltype(this->_buffer.template begin <T> ()) {
					// Возвращаем начальный итератор буфера
					return this->_buffer.template begin <T> ();
				}
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param buffer обёртываемый буфер
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param buffer buffer being wrapped
				 *
				 * \~
				 */
				explicit view(Buffer & buffer) noexcept : _buffer(buffer) {}
			};
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа данных обёртки буфера
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the buffer wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод обёртки бинарного буфера
			 *
			 * @return обёрнутый бинарный буфер
			 *
			 * \~english
			 * @brief Method of wrapping the binary buffer
			 *
			 * @return wrapped binary buffer
			 *
			 * \~
			 */
			view <T> as() & {
				// Возвращаем буфер по ссылке — только для lvalue
				return view <T> (* this);
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон типа данных константной обёртки буфера
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the constant buffer wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Класс константной обёртки буфера
			 *
			 * \~english
			 * @brief Constant buffer wrapper class
			 *
			 * \~
			 */
			class const_view {
				// Буфер который необходимо обернуть
				const Buffer & _buffer;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор буфера
				 *
				 * \~english
				 * @brief Method obtaining the end iterator
				 *
				 * @return end iterator of the buffer
				 *
				 * \~
				 */
				auto end() const noexcept -> decltype(this->_buffer.template end <T> ()) {
					// Возвращаем конечный итератор буфера
					return this->_buffer.template end <T> ();
				}
				/**
				 * \~russian
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор буфера
				 *
				 * \~english
				 * @brief Method obtaining the begin iterator
				 *
				 * @return begin iterator of the buffer
				 *
				 * \~
				 */
				auto begin() const noexcept -> decltype(this->_buffer.template begin <T> ()) {
					// Возвращаем начальный итератор буфера
					return this->_buffer.template begin <T> ();
				}
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param buffer обёртываемый буфер
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param buffer buffer being wrapped
				 *
				 * \~
				 */
				explicit const_view(const Buffer & buffer) noexcept : _buffer(buffer) {}
			};
		public:
			/**
			 * \~russian
			 * @brief Шаблон типа данных константной обёртки буфера
			 *
			 * @tparam T тип итератора
			 *
			 * \~english
			 * @brief Template of the data type of the constant buffer wrapper
			 *
			 * @tparam T iterator type
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод константной обёртки бинарного буфера
			 *
			 * @return обёрнутый бинарный буфер
			 *
			 * \~english
			 * @brief Method of the constant wrapping of the binary buffer
			 *
			 * @return wrapped binary buffer
			 *
			 * \~
			 */
			const_view <T> as() const & {
				// Возвращаем буфер по константной ссылке — только для lvalue
				return const_view <T> (* this);
			}
	} buffer_t;
	/**
	 * \~russian
	 * @brief Оператор [>>] чтения из потока буфера
	 *
	 * @param is     поток для чтения
	 * @param buffer буфер для присвоения
	 *
	 * \~english
	 * @brief Operator [>>] of reading the buffer from a stream
	 *
	 * @param is     stream for reading
	 * @param buffer buffer for the assignment
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, buffer_t & buffer) noexcept;
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток буфера
	 *
	 * @param os     поток куда нужно вывести данные
	 * @param buffer буфер извлечения
	 *
	 * \~english
	 * @brief Operator [<<] of outputting the buffer into a stream
	 *
	 * @param os     stream the data should be output into
	 * @param buffer buffer of the extraction
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const buffer_t & buffer) noexcept;
};

#endif // __AWH_BUFFER__
