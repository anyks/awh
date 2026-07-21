/**
 * @file: buffer.hpp
 * @date: 2025-10-26
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
#include "fmk.hpp"
#include "log.hpp"

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
	 * @brief Класс бинарного смартбуфера
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Buffer {
		private:
			/**
			 * @brief Структура диапазонов записей
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Range {
				// Конец записи
				size_t end;
				// Начало записи
				size_t begin;
				// Максимальный размер выделения памяти (по умолчанию AWH_MAX_MEMORY_BUFFER)
				size_t maxMemory;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Range() noexcept;
			} __attribute__((packed)) range_t;
		public:
			/**
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
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
			 */
			template <typename T>
			/**
			 * @brief Класс итератора как вложенный класс
			 *
			 */
			class Iterator {
				private:
					// Позиция в бинарном буфере
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
					 * @return значение заголовка
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
					 */
					explicit Iterator(T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * @brief Шаблон типа данных итератора
			 *
			 * @tparam T тип итератора
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
			 */
			template <typename T>
			/**
			 * @brief Класс константного итератора как вложенный класс
			 *
			 */
			class Const_Iterator {
				private:
					// Позиция в бинарном буфере
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
					 * @return значение заголовка
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
					 */
					explicit Const_Iterator(const T * ptr) noexcept : _ptr(ptr) {}
			};
			/**
			 * @brief Шаблон типа данных константного итератора
			 *
			 * @tparam T тип итератора
			 */
			template <typename T>
			/**
			 * @brief Создаём тип данных константного итератора
			 *
			 */
			using const_iterator_t = Const_Iterator <T>;
		public:
			/**
			 * @brief Класс RAII-обёртки для безопасной прямой записи в хвост буфера (zero-copy)
			 *
			 * @details Резервирует место в хвосте буфера, предоставляет указатель для прямой
			 *          записи (например, из сокета) и автоматически фиксирует записанные данные
			 *          при выходе из области видимости. Если фиксация не запрошена через commit(),
			 *          ничего не добавляется (безопасный откат).
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
					 * @brief Метод получения указателя на зарезервированную область
					 *
					 * @return указатель для записи данных либо nullptr при ошибке резервирования
					 */
					void * get() noexcept;
					/**
					 * @brief Метод получения размера зарезервированной области
					 *
					 * @return размер доступного для записи места
					 */
					size_t size() const noexcept;
					/**
					 * @brief Метод проверки корректности резервирования
					 *
					 * @return результат проверки
					 */
					bool valid() const noexcept;
				public:
					/**
					 * @brief Метод отмены записи (зафиксировано не будет ничего)
					 *
					 */
					void cancel() noexcept;
					/**
					 * @brief Метод немедленной фиксации записанных данных в буфер
					 *
					 * @return количество зафиксированных байт
					 */
					size_t apply() noexcept;
					/**
					 * @brief Метод указания количества фактически записанных байт
					 *
					 * @param size количество записанных байт
					 * @return     количество байт которое будет зафиксировано
					 */
					size_t commit(const size_t size) noexcept;
				public:
					/**
					 * @brief Запрещаем копирование обёртки
					 *
					 */
					Writer(const Writer &) = delete;
					/**
					 * @brief Запрещаем присвоение обёртки
					 *
					 */
					Writer & operator = (const Writer &) = delete;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param other обёртка для перемещения
					 */
					Writer(Writer && other) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param buffer   буфер для записи
					 * @param data     указатель на зарезервированную область
					 * @param capacity размер зарезервированной области
					 */
					explicit Writer(Buffer * buffer, void * data, const size_t capacity) noexcept;
					/**
					 * @brief Деструктор (автоматически фиксирует записанные данные)
					 *
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
			 * @brief Метод контроля памяти
			 *
			 * @details Гарантирует наличие как минимум size свободных байт в хвосте буфера.
			 *          При необходимости переиспользует уже извлечённое место в начале буфера
			 *          (компактизация) и/или увеличивает буфер в пределах максимального лимита.
			 *
			 * @param size желаемый размер свободного места в хвосте буфера
			 * @return     результат выполнения операции
			 */
			bool rss(const size_t size) noexcept;
		private:
			/**
			 * @brief Метод вывода сообщения об ошибке в лог
			 *
			 * @param func    название функции в которой произошла ошибка
			 * @param message текст сообщения об ошибке
			 * @param flag    флаг важности сообщения
			 */
			void error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
		public:
			/**
			 * @brief Метод очистки всех данных буфера
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
			 * @brief Метод проверки на заполненность буфера
			 *
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * @brief Метод получения размера добавленных данных
			 *
			 * @return размер всех добавленных данных
			 */
			size_t size() const noexcept;
		public:
			/**
			 * @brief Метод вывода размера занимаемой памяти очередью
			 *
			 * @return количество памяти которую занимает буфер
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * @brief Метод извлечения буфера сырых данных
			 *
			 * @details Нормализует внутреннее хранилище: переносит полезные данные в начало
			 *          и усекает буфер до их размера, после чего возвращает его как есть.
			 *
			 * @return буфер сырых данных
			 */
			const vector <uint8_t> & raw() const noexcept;
		public:
			/**
			 * @brief Шаблон для метода получения конечного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного итератора
			 *
			 * @return конечный итератор
			 */
			iterator_t <T> end() noexcept;
			/**
			 * @brief Шаблон для метода получение начального итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получение начального итератора
			 *
			 * @return начальный итератор
			 */
			iterator_t <T> begin() noexcept;
			/**
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 */
			const_iterator_t <T> end() const noexcept;
			/**
			 * @brief Шаблон для метода получения конечного константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения конечного константного итератора
			 *
			 * @return конечный константный итератор
			 */
			const_iterator_t <T> cend() const noexcept;
			/**
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 */
			const_iterator_t <T> begin() const noexcept;
			/**
			 * @brief Шаблон для метода получения начального константного итератора
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения начального константного итератора
			 *
			 * @return начальный константный итератор
			 */
			const_iterator_t <T> cbegin() const noexcept;
		public:
			/**
			 * @brief Шаблон для метода удаления верхних записей
			 *
			 * @tparam T тип данных для удаления
			 */
			template <typename T>
			/**
			 * @brief Метод удаления записи в буфера
			 *
			 */
			void pop() noexcept;
		public:
			/**
			 * @brief Шаблон для метода получения количества элементов в бинарном буфере
			 *
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * @brief Метод получения количества элементов в бинарном буфере
			 *
			 * @return количество всех добавленных лементов
			 */
			size_t count() const noexcept;
		public:
			/**
			 * @brief Шаблон для метода извлечения нижнего значения в буфере
			 *
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения нижнего значения в буфере
			 *
			 * @return данные содержащиеся в буфере
			 */
			T back() const noexcept;
			/**
			 * @brief Шаблон для метода извлечения верхнего значения в буфере
			 *
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения верхнего значения в буфере
			 *
			 * @return данные содержащиеся в буфере
			 */
			T front() const noexcept;
		public:
			/**
			 * @brief Шаблон для метода извлечения содержимого контейнера по его индексу
			 *
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * @brief Метод извлечения содержимого контейнера по его индексу
			 *
			 * @param index индекс массива для извлечения
			 * @return      данные содержащиеся в буфере
			 */
			T at(const size_t index) const noexcept;
		public:
			/**
			 * @brief Шаблон для метода установки значений в уже существующем буфере
			 *
			 * @tparam T тип данных для установки
			 */
			template <typename T>
			/**
			 * @brief Метод установки значений в уже существующем буфере
			 *
			 * @param value значение для установки
			 * @param index индекс значения для установки
			 */
			void set(const T value, const size_t index) noexcept;
		public:
			/**
			 * @brief Получения данных указанного элемента в буфера
			 *
			 * @return указатель на элемент буфера
			 */
			const void * data() const noexcept;
		public:
			/**
			 * @brief Метод удаления указанного количества байт из начала буфера
			 *
			 * @param size количество байт для удаления
			 */
			void erase(const size_t size) noexcept;
			/**
			 * @brief Метод извлечения (удаления) указанного количества уже обработанных байт из начала буфера
			 *
			 * @param size количество байт для извлечения
			 */
			void consume(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод подготовки места в хвосте буфера для прямой записи (zero-copy)
			 *
			 * @details После записи данных по полученному указателю необходимо вызвать commit(n).
			 *          Указатель действителен только до следующей модификации буфера.
			 *          Для безопасной работы рекомендуется использовать метод write().
			 *
			 * @param size требуемое количество свободных байт в хвосте буфера
			 * @return     указатель на начало свободной области либо nullptr при ошибке
			 */
			void * prepare(const size_t size) noexcept;
			/**
			 * @brief Метод фиксации записанных в хвост буфера данных (zero-copy)
			 *
			 * @param size количество фактически записанных в хвост байт
			 * @return     количество зафиксированных байт
			 */
			size_t commit(const size_t size) noexcept;
			/**
			 * @brief Метод получения RAII-обёртки для безопасной прямой записи в хвост буфера (zero-copy)
			 *
			 * @param size требуемое количество свободных байт в хвосте буфера
			 * @return     объект записи с автоматической фиксацией данных
			 */
			Writer write(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод резервирования размера буфера
			 *
			 * @param size размер выделяемой памяти
			 */
			void reserve(const size_t size) noexcept;
		public:
			/**
			 * @brief Шаблон для добавления числа в буфер
			 *
			 * @tparam T тип данных для добавления
			 */
			template <typename T>
			/**
			 * @brief Метод добавления числа в буфер
			 *
			 * @param value значение для добавления
			 * @return       результат добавления данных
			 */
			bool push(const T value) noexcept;
			/**
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 */
			bool push(const char * text) noexcept;
			/**
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 */
			bool push(string_view text) noexcept;
			/**
			 * @brief Метод добавления текста в буфер
			 *
			 * @param text текст для добавления
			 * @return     результат добавления данных
			 */
			bool push(const string & text) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @details Если текущий буфер пуст — выполняется перемещение хранилища (zero-copy),
			 *          иначе данные дописываются в хвост.
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 */
			bool push(Buffer && buffer) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 */
			bool push(const Buffer & buffer) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @return       результат добавления данных
			 */
			bool push(const vector <uint8_t> & buffer) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в буфер
			 *
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 * @return       результат добавления данных
			 */
			bool push(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки максимального размера потребления памяти
			 *
			 * @param size максимальный размер потребления памяти
			 */
			void setMaxMemory(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод обмена очередями
			 *
			 * @param buffer бинарный буфер для обмена
			 */
			void swap(Buffer & buffer) noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @param log объект работы с логами
			 */
			void setLogger(const log_t * log) noexcept;
		public:
			/**
			 * @brief Получения размера данных в буфера
			 *
			 * @return размер данных в буфера
			 */
			operator size_t() const noexcept;
			/**
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 */
			operator const char * () const noexcept;
			/**
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 */
			operator const uint8_t * () const noexcept;
			/**
			 * @brief Получения бинарных данных буфера
			 *
			 * @return бинарные данные буфера
			 */
			operator const vector <uint8_t> & () const noexcept;
		public:
			/**
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const char * buffer) noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (string && buffer) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const string & buffer) noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (vector <uint8_t> && buffer) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (Buffer && buffer) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const Buffer & buffer) noexcept;
		public:
			/**
			 * @brief Оператор сравнения двух очередей
			 *
			 * @param buffer бинарный буфер для сравнения
			 * @return       результат сравнения
			 */
			bool operator == (const Buffer & buffer) const noexcept;
		public:
			/**
			 * @brief Разрешаем пустое значение объекта
			 *
			 */
			explicit Buffer() noexcept;
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 */
			explicit Buffer(Buffer && buffer) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 */
			explicit Buffer(const Buffer & buffer) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Buffer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Buffer() noexcept;
		private:
			/**
			 * @brief Шаблон типа данных обёртки буфера
			 *
			 * @tparam T тип итератора
			 */
			template <typename T>
			/**
			 * @brief Класс обёртки буфера
			 *
			 */
			class view {
				// Буфер который необходимо обернуть
				Buffer & _buffer;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор буфера
				 */
				auto end() noexcept -> decltype(this->_buffer.template end <T> ()) {
					// Возвращаем конечный итератор буфера
					return this->_buffer.template end <T> ();
				}
				/**
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор буфера
				 */
				auto begin() noexcept -> decltype(this->_buffer.template begin <T> ()) {
					// Возвращаем начальный итератор буфера
					return this->_buffer.template begin <T> ();
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param buffer обёртываемый буфер
				 */
				explicit view(Buffer & buffer) noexcept : _buffer(buffer) {}
			};
		public:
			/**
			 * @brief Шаблон типа данных обёртки буфера
			 *
			 * @tparam T тип итератора
			 */
			template <typename T>
			/**
			 * @brief Метод обёртки бинарного буфера
			 *
			 * @return обёрнутый бинарный буфер
			 */
			view <T> as() & {
				// Возвращаем буфер по ссылке — только для lvalue
				return view <T> (* this);
			}
		private:
			/**
			 * @brief Шаблон типа данных константной обёртки буфера
			 *
			 * @tparam T тип итератора
			 */
			template <typename T>
			/**
			 * @brief Класс константной обёртки буфера
			 *
			 */
			class const_view {
				// Буфер который необходимо обернуть
				const Buffer & _buffer;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор буфера
				 */
				auto end() const noexcept -> decltype(this->_buffer.template end <T> ()) {
					// Возвращаем конечный итератор буфера
					return this->_buffer.template end <T> ();
				}
				/**
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор буфера
				 */
				auto begin() const noexcept -> decltype(this->_buffer.template begin <T> ()) {
					// Возвращаем начальный итератор буфера
					return this->_buffer.template begin <T> ();
				}
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param buffer обёртываемый буфер
				 */
				explicit const_view(const Buffer & buffer) noexcept : _buffer(buffer) {}
			};
		public:
			/**
			 * @brief Шаблон типа данных константной обёртки буфера
			 *
			 * @tparam T тип итератора
			 */
			template <typename T>
			/**
			 * @brief Метод константной обёртки бинарного буфера
			 *
			 * @return обёрнутый бинарный буфер
			 */
			const_view <T> as() const & {
				// Возвращаем буфер по константной ссылке — только для lvalue
				return const_view <T> (* this);
			}
	} buffer_t;
	/**
	 * @brief Оператор [>>] чтения из потока буфера
	 *
	 * @param is     поток для чтения
	 * @param buffer буфер для присвоения
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, buffer_t & buffer) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток буфера
	 *
	 * @param os     поток куда нужно вывести данные
	 * @param buffer буфер извлечения
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const buffer_t & buffer) noexcept;
};

#endif // __AWH_BUFFER__
