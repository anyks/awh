/**
 * @file: buffer.hpp
 * @date: 2025-09-29
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
#include <mutex>
#include <vector>
#include <cstdint>

/**
 * Подключаем наши заголовочные файлы
 */
#include "fmk.hpp"
#include "log.hpp"

/**
 * @brief пространство имён
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
			 * @brief Структура итерации записей
			 * 
			 */
			typedef struct Iterator {
				size_t end;   // Конец записи
				size_t begin; // Начало записи
				/**
				 * @brief Конструктор
				 * 
				 */
				Iterator() noexcept : end(0), begin(0) {}
			} __attribute__((packed)) iter_t;
		private:
			// Объект итерации данных
			iter_t _iter;
		private:
			// Максимальный размер выделения памяти
			size_t _maxMemory;
		private:
			// Мютекс для блокировки потока
			mutable std::mutex _mtx;
		private:
			// Буфер данных выделенной памяти
			vector <char> _buffer;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
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
			 * @brief Конструктор перемещения
			 *
			 * @param buffer бинарный буфер для перемещения
			 */
			Buffer(Buffer && buffer) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param buffer бинарный буфер для копирования
			 */
			Buffer(const Buffer & buffer) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Buffer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Buffer() noexcept;
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
