/**
 * @file: queue.hpp
 * @date: 2025-10-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл бинарной очереди —
 *        класс Queue для последовательного хранения записей произвольного размера в непрерывной памяти с итераторами,
 *        лимитами и невладеющими обёртками доступа
 *
 * @copyright: Copyright © 2025
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс бинарной очереди
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Queue {
		private:
			/**
			 * @brief Структура диапазонов записей
			 *
			 * @note Структура является внутренней и используется для хранения диапазонов записей в очереди
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				size_t end;    // Конец записи
				size_t begin;  // Начало записи
				size_t count;  // Количество добавленных записей
				size_t offset; // Смещение для чтения данных
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Range() noexcept;
			} __attribute__((packed)) range_t;
			/**
			 * @brief Структура параметров максимальных значений
			 *
			 * @note Структура является внутренней и используется для хранения максимальных значений очереди
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Max {
				// Максимальный размер выделения памяти (по умолчанию 1 МБ)
				size_t memory;
				// Максимальное количество добавляемых записей (по умолчанию 1000)
				size_t records;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Max() noexcept;
			} __attribute__((packed)) max_t;
		public:
			/**
			 * Создаём тип данных добавляемой записи
			 */
			typedef pair <const void *, size_t> record_t;
			/**
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Предварительное объявление константного итератора
			 *
			 */
			class Const_Iterator;
			/**
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Класс итератора как вложенный класс
			 *
			 */
			class Iterator {
				private:
					// Позиция в бинарной очереди
					T * _ptr;
				private:
					/**
					 * @brief Разрешаем доступ к позиции константному итератору
					 *
					 */
					template <typename U> friend class Const_Iterator;
				public:
					/**
					 * @brief Оператор разыменования
					 *
					 * @return значение элемента
					 *
					 */
					const T & operator * () const noexcept {
						// Извлекаем значение сдвига итератора
						return * this->_ptr;
					}
				public:
					/**
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
					 */
					Iterator & operator ++ () noexcept {
						// Выполняем смещение текущего значения итератора
						++this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
					/**
					 * @brief Оператор смещения назад
					 *
					 * @return значение текущего итератора
					 *
					 */
					Iterator & operator -- () noexcept {
						// Выполняем смещение текущего значения итератора
						--this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
				public:
					/**
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator == (const Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator != (const Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
					/**
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other константный итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator == (const Const_Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other константный итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator != (const Const_Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param ptr позиция в контейнере
					 *
					 */
					explicit Iterator(T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Создаём тип данных итератора
			 *
			 */
			using iterator_t = Iterator <T>;
			/**
			 * @brief Шаблон типа данных константного итератора
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Класс константного итератора как вложенный класс
			 *
			 */
			class Const_Iterator {
				private:
					// Позиция в бинарной очереди
					const T * _ptr;
				private:
					/**
					 * @brief Разрешаем доступ к позиции обычному итератору
					 *
					 */
					template <typename U> friend class Iterator;
				public:
					/**
					 * @brief Оператор разыменования
					 *
					 * @return значение элемента
					 *
					 */
					const T & operator * () const noexcept {
						// Извлекаем значение сдвига итератора
						return * this->_ptr;
					}
				public:
					/**
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 *
					 */
					Const_Iterator & operator ++ () noexcept {
						// Выполняем смещение текущего значения итератора
						++this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
					/**
					 * @brief Оператор смещения назад
					 *
					 * @return значение текущего итератора
					 *
					 */
					Const_Iterator & operator -- () noexcept {
						// Выполняем смещение текущего значения итератора
						--this->_ptr;
						// Возвращаем текущее значение итератора
						return (* this);
					}
				public:
					/**
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator == (const Const_Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator != (const Const_Iterator & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
					/**
					 * @brief Оператор сравнения соответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator == (const Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr == other._ptr);
					}
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 *
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 *
					 */
					bool operator != (const Iterator <T> & other) const noexcept {
						// Выполняем сравнение итератора
						return (this->_ptr != other._ptr);
					}
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param ptr позиция в контейнере
					 *
					 */
					explicit Const_Iterator(const T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * @brief Шаблон типа данных константного итератора
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Создаём тип данных константного итератора
			 *
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
			 * @brief Метод контроля памяти
			 *
			 * @param size желаемый размер выделения памяти
			 * @return     результат выполнения операции
			 *
			 */
			bool rss(const size_t size) noexcept;
		private:
			/**
			 * @brief Метод вывода сообщения об ошибке
			 *
			 * @param func    название функции, в которой произошла ошибка
			 * @param size    размер данных, связанный с ошибкой
			 * @param message текст сообщения об ошибке
			 *
			 */
			void error(const char * func, const size_t size, const char * message) const noexcept;
		public:
			/**
			 * @brief Метод удаления записи в очереди
			 *
			 */
			void pop() noexcept;
		public:
			/**
			 * @brief Метод очистки всех данных очереди
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод полной очистки памяти
			 *
			 */
			void reset() noexcept;
		public:
			/**
			 * @brief Количество добавленных элементов
			 *
			 * @return количество добавленных элементов
			 *
			 */
			size_t count() const noexcept;
		public:
			/**
			 * @brief Метод получения размера добавленных данных
			 *
			 * @return размер добавленных данных
			 *
			 */
			size_t size() const noexcept;
		public:
			/**
			 * @brief Метод вывода размера занимаемой памяти очередью
			 *
			 * @return количество памяти которую занимает очередь
			 *
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * @brief Шаблон для метода получения конечного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного итератора
			 *
			 * @return конечный итератор
			 *
			 */
			iterator_t <T> end() noexcept;
			/**
			 * @brief Шаблон для метода получение начального итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получение начального итератора
			 *
			 * @return начальный итератор
			 *
			 */
			iterator_t <T> begin() noexcept;
			/**
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 *
			 */
			const_iterator_t <T> end() const noexcept;
			/**
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 *
			 */
			const_iterator_t <T> cend() const noexcept;
			/**
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 *
			 */
			const_iterator_t <T> begin() const noexcept;
			/**
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 *
			 */
			const_iterator_t <T> cbegin() const noexcept;
		public:
			/**
			 * @brief Получения данных указанного элемента в очереди
			 *
			 * @return указатель на элемент очереди
			 *
			 */
			const void * data() const noexcept;
		public:
			/**
			 * @brief Метод фиксации прочитанного размера данных
			 *
			 * @param size размер данных для фиксации
			 *
			 */
			void commit(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод проверки на заполненность очереди
			 *
			 * @note При установленном таймауте и активной потокобезопасности метод блокирует
			 *       поток до появления данных в очереди либо до истечения времени ожидания.
			 *
			 * @param timeout время ожидания в миллисекундах
			 * @return        результат проверки
			 *
			 */
			bool empty(const uint32_t timeout = 0) const noexcept;
		public:
			/**
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 * @return       текущий размер очереди
			 *
			 */
			size_t push(const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param records список бинарных буферов для добавления
			 * @param size    общий размер добавляемых данных
			 * @return        текущий размер очереди
			 *
			 */
			size_t push(const vector <record_t> & records, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки максимального размера потребления памяти
			 *
			 * @param size максимальный размер потребления памяти
			 *
			 */
			void setMaxMemory(const size_t size) noexcept;
			/**
			 * @brief Метод установки максимального количества записей очереди
			 *
			 * @param count максимальное количество записей очереди
			 *
			 */
			void setMaxRecords(const size_t count) noexcept;
		public:
			/**
			 * @brief Метод обмена очередями
			 *
			 * @param queue очередь для обмена
			 *
			 */
			void swap(Queue & queue) noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности работы потоков
			 *
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 *
			 */
			void setLogger(const log_t * log) noexcept;
		public:
			/**
			 * @brief Получения размера данных в очереди
			 *
			 * @return размер данных в очереди
			 *
			 */
			operator size_t() const noexcept;
			/**
			 * @brief Получения бинарных данных очереди
			 *
			 * @return бинарные данные очереди
			 *
			 */
			operator const char * () const noexcept;
			/**
			 * @brief Получения бинарных данных очереди
			 *
			 * @return бинарные данные очереди
			 *
			 */
			operator const uint8_t * () const noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param queue очередь для перемещения
			 * @return      текущий контейнер очереди
			 *
			 */
			Queue & operator = (Queue && queue) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param queue очередь для копирования
			 * @return      текущий контейнер очереди
			 *
			 */
			Queue & operator = (const Queue & queue) noexcept;
		public:
			/**
			 * @brief Оператор сравнения двух очередей
			 *
			 * @param queue очередь для сравнения
			 * @return      результат сравнения
			 *
			 */
			bool operator == (const Queue & queue) const noexcept;
		public:
			/**
			 * @brief Разрешаем пустое значение объекта
			 *
			 */
			explicit Queue() noexcept;
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param queue очередь для перемещения
			 *
			 */
			explicit Queue(Queue && queue) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param queue очередь для копирования
			 *
			 */
			explicit Queue(const Queue & queue) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Queue(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Queue() noexcept;
		private:
			/**
			 * @brief Шаблон типа данных обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Класс обёртки очереди
			 *
			 */
			class view {
				// Очередь которую необходимо обернуть
				Queue & _queue;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор очереди
				 *
				 */
				auto end() noexcept -> decltype(this->_queue.template end <T> ()) {
					// Возвращаем конечный итератор очереди
					return this->_queue.template end <T> ();
				}
				/**
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор очереди
				 *
				 */
				auto begin() noexcept -> decltype(this->_queue.template begin <T> ()) {
					// Возвращаем начальный итератор очереди
					return this->_queue.template begin <T> ();
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param queue обёртываемая очередь
				 *
				 */
				explicit view(Queue & queue) noexcept : _queue(queue) {}
			};
		public:
			/**
			 * @brief Шаблон типа данных обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод обёртки бинарной очереди
			 *
			 * @return обёрнутая бинарная очередь
			 *
			 */
			view <T> as() & {
				// Возвращаем очередь по ссылке — только для lvalue
				return view <T> (* this);
			}
		private:
			/**
			 * @brief Шаблон типа данных константной обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Класс константной обёртки очереди
			 *
			 */
			class const_view {
				// Очередь которую необходимо обернуть
				const Queue & _queue;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор очереди
				 *
				 */
				auto end() const noexcept -> decltype(this->_queue.template end <T> ()) {
					// Возвращаем конечный итератор очереди
					return this->_queue.template end <T> ();
				}
				/**
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор очереди
				 *
				 */
				auto begin() const noexcept -> decltype(this->_queue.template begin <T> ()) {
					// Возвращаем начальный итератор очереди
					return this->_queue.template begin <T> ();
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param queue обёртываемая очередь
				 *
				 */
				explicit const_view(const Queue & queue) noexcept : _queue(queue) {}
			};
		public:
			/**
			 * @brief Шаблон типа данных константной обёртки очереди
			 *
			 * @tparam T тип итератора
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод константной обёртки бинарной очереди
			 *
			 * @return обёрнутая бинарная очередь
			 *
			 */
			const_view <T> as() const & {
				// Возвращаем очередь по константной ссылке — только для lvalue
				return const_view <T> (* this);
			}
	} queue_t;
};

#endif // __AWH_QUEUE__
