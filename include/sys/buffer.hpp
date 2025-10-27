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
 * Стандартные библиотеки
 */
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем наши заголовочные файлы
 */
#include "fmk.hpp"
#include "log.hpp"

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
	 * @brief Класс бинарного смартбуфера
	 *
	 */
	typedef class AWH_SHARED_EXPORT Buffer {
		private:
			/**
			 * @brief Структура диапазонов записей
			 *
			 */
			typedef struct Range {
				size_t end;   // Конец записи
				size_t begin; // Начало записи
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Range() noexcept : end(0), begin(0) {}
			} __attribute__((packed)) range_t;
		public:
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
						// Выводим текущее значение итератора
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
						// Выводим текущее значение итератора
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
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param ptr позиция в контейнера
					 */
					explicit Iterator(T * ptr) noexcept : _ptr(ptr) {}
			};
		private:
			// Объект диапазонов записей
			range_t _range;
		private:
			// Максимальный размер выделения памяти
			size_t _maxMemory;
		private:
			// Буфер данных выделенной памяти
			vector <char> _buffer;
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
			 */
			bool rss(const size_t size) noexcept;
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
			 * @return буфер сырых данных
			 */
			const vector <char> & raw() const noexcept;
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
			Iterator <T> end() noexcept;
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
			Iterator <T> begin() noexcept;
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
			 * @brief Метод удаления указанного количества байт
			 *
			 * @param size количество байт для удаления
			 */
			void erase(const size_t size) noexcept;
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
			bool push(const string & text) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в буфер
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
			bool push(const vector <char> & buffer) noexcept;
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
			operator const vector <char> & () const noexcept;
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
			Buffer & operator = (vector <char> && buffer) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const vector <char> & buffer) noexcept;
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
			explicit Buffer() = default;
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
					// Выводим конечный итератор буфера
					return this->_buffer.template end <T> ();
				}
				/**
				 * @brief Метод получения начального итератора
				 *
				 * @return начальный итератор буфера
				 */
				auto begin() noexcept -> decltype(this->_buffer.template begin <T> ()) {
					// Выводим начальный итератор буфера
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
				// Выводим буфер & — только для lvalue
				return view <T> (* this);
			}
	} buffer_t;
	/**
	 * @brief Оператор [>>] чтения из потока буфера
	 *
	 * @param is     поток для чтения
	 * @param buffer буфер для присвоения
	 */
	AWH_SHARED_EXPORT istream & operator >> (istream & is, buffer_t & buffer) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток буфера
	 *
	 * @param os     поток куда нужно вывести данные
	 * @param buffer буфер извлечения
	 */
	AWH_SHARED_EXPORT ostream & operator << (ostream & os, const buffer_t & buffer) noexcept;
};

#endif // __AWH_BUFFER__
