/**
 * @file: buffer.hpp
 * @date: 2024-12-28
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
 * Подстраиваемся под операционную систему
 */
#if defined(__BORLANDC__)
	typedef unsigned char uint8_t;
	typedef __int64 int64_t;
	typedef unsigned long uintptr_t;
#elif defined(_MSC_VER)
	typedef unsigned char uint8_t;
	typedef __int64 int64_t;
#else
	#include <stdint.h>
#endif

/**
 * Стандартная библиотека
 */
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <algorithm>

/**
 * Подключаем наши заголовочные файлы
 */
#include <sys/log.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Buffer Класс создания очереди
	 */
	typedef class AWHSHARED_EXPORT Buffer {
		private:
			// Количество аллоцированных элементов 100Mb
			static constexpr uint32_t BATCH = 0x6400000;
		private:
			// Мютекс для блокировки потока
			mutex _mtx;
		private:
			// Последний элемент в очереди
			size_t _end;
			// Первый элемент в очереди
			size_t _begin;
		private:
			// Текущий размер очереди
			size_t _size;
		private:
			// Максимальный размер выделяемой памяти
			size_t _batch;
		private:
			// Размер всех добавленных данных
			size_t _bytes;
		private:
			// Адреса добавленных данных
			uint8_t * _data;
		private:
			// Объект для работы с логами
			const log_t * _log;
		public:
			/**
			 * clear Метод очистки всех данных очереди
			 */
			void clear() noexcept;
		public:
			/**
			 * empty Метод проверки на заполненность очереди
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * size Метод получения размера добавленных данных
			 * @return размер всех добавленных данных
			 */
			size_t size() const noexcept;
		public:
			/**
			 * capacity Метод получения размера выделенной памяти
			 * @return размер выделенной памяти 
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * Шаблон для метода удаления верхних записей
			 * @tparam T тип данных для удаления
			 */
			template <typename T>
			/**
			 * pop Метод удаления верхних записей
			 */
			void pop() noexcept {
				// Если мы не дошли до конца
				if(!this->empty())
					// Выполняем удаление указанного количества байт
					this->erase(sizeof(T));
			}
		public:
			/**
			 * Шаблон для метода получения количества элементов в бинарном буфере
			 * @tparam T тип данных для подсчёта
			 */
			template <typename T>
			/**
			 * count Метод получения количества элементов в бинарном буфере
			 * @return количество всех добавленных лементов
			 */
			size_t count() const noexcept {
				// Если мы не дошли до конца
				if(!this->empty())
					// Выводим размер данных в буфере
					return (this->size() / sizeof(T));
				// Выводим пустое значение
				return 0;
			}
		public:
			/**
			 * Шаблон для метода извлечения нижнего значения в буфере
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * back Метод извлечения нижнего значения в буфере
			 * @return данные содержащиеся в буфере
			 */
			T back() const noexcept {
				// Результат работы функции
				T result = 0;
				// Если контейнер не пустой
				if(!this->empty()){
					// Получаем размер данных
					const size_t size = sizeof(result);
					// Выполняем копирование данных контейнера
					::memcpy(&result, this->get() + (this->_end - size), size);
				}
				// Выводим результат
				return result;
			}
			/**
			 * Шаблон для метода извлечения верхнего значения в буфере
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * front Метод извлечения верхнего значения в буфере
			 * @return данные содержащиеся в буфере
			 */
			T front() const noexcept {
				// Результат работы функции
				T result = 0;
				// Если контейнер не пустой
				if(!this->empty()){
					// Получаем размер данных
					const size_t size = sizeof(result);
					// Выполняем копирование данных контейнера
					::memcpy(&result, this->get(), size);
				}
				// Выводим результат
				return result;
			}
		public:
			/**
			 * Шаблон для метода извлечения содержимого контейнера по его индексу
			 * @tparam T тип данных для извлечения
			 */
			template <typename T>
			/**
			 * at Метод извлечения содержимого контейнера по его индексу
			 * @param index индекс массива для извлечения
			 * @return      данные содержащиеся в буфере
			 */
			T at(const size_t index) const noexcept {
				// Результат работы функции
				T result = 0;
				// Если контейнер не пустой
				if(!this->empty() && (index < this->size())){
					// Получаем размер данных
					const size_t size = sizeof(result);
					// Выполняем копирование данных контейнера
					::memcpy(&result, this->get() + (index * size), size);
				}
				// Выводим результат
				return result;
			}
		public:
			/**
			 * Шаблон для метода установки значений в уже существующем буфере
			 * @tparam T тип данных для установки
			 */
			template <typename T>
			/**
			 * set Метод установки значений в уже существующем буфере
			 * @param value значение для установки
			 * @param index индекс значения для установки
			 */
			void set(const T value, const size_t index) noexcept {
				// Если контейнер не пустой
				if(!this->empty() && (index < this->size())){
					// Получаем размер данных
					const size_t size = sizeof(value);
					// Выполняем установку значения
					::memcpy(const_cast <uint8_t *> (this->get() + (index * size)), &value, size);
				}
			}
		public:
			/**
			 * get Получения данных указанного элемента в очереди
			 * @return указатель на элемент очереди
			 */
			const uint8_t * get() const noexcept;
		public:
			/**
			 * erase Метод удаления указанного количества байт
			 * @param size количество байт для удаления
			 */
			void erase(const size_t size) noexcept;
		public:
			/**
			 * batch Метод установки размера выделяемой памяти
			 * @param batch размер выделяемой памяти
			 */
			void batch(const size_t batch) noexcept;
		public:
			/**
			 * reserve Метод резервирования размера очереди
			 * @param size размер выделяемой памяти
			 */
			void reserve(const size_t size = 0) noexcept;
		public:
			/**
			 * push Метод добавления бинарного буфера данных в очередь
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 */
			void push(const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * operator Получения размера данных в буфере
			 * @return размер данных в буфере
			 */
			operator size_t() const noexcept;
			/**
			 * operator Получения бинарных данных буфера
			 * @return бинарные данные буфера
			 */
			operator const char * () const noexcept;
		public:
			/**
			 * operator Оператор перемещения
			 * @param buffer буфер для перемещения
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (Buffer && buffer) noexcept;
			/**
			 * operator Оператор копирования
			 * @param buffer буфер для копирования
			 * @return       текущий контейнер буфера
			 */
			Buffer & operator = (const Buffer & buffer) noexcept;
		public:
			/**
			 * operator Оператор извлечения символов буфера по его индексу
			 * @param index индекс буфера
			 * @return      символ находящийся в буфере
			 */
			uint8_t operator [](const size_t index) const noexcept;
		public:
			/**
			 * operator Оператор сравнения двух буферов
			 * @param buffer буфер для сравнения
			 * @return       результат сравнения
			 */
			bool operator == (const Buffer & buffer) const noexcept;
		public:
			/**
			 * Buffer Конструктор перемещения
			 * @param buffer буфер данных для перемещения
			 */
			Buffer(Buffer && buffer) noexcept;
			/**
			 * Buffer Конструктор копирования
			 * @param buffer буфер данных для копирования
			 */
			Buffer(const Buffer & buffer) noexcept;
		public:
			/**
			 * Buffer Конструктор
			 * @param log объект для работы с логами
			 */
			Buffer(const log_t * log) noexcept;
			/**
			 * Buffer Конструктор
			 * @param batch максимальный размер выделяемой памяти
			 * @param log   объект для работы с логами
			 */
			Buffer(const size_t batch, const log_t * log) noexcept;
		public:
			/**
			 * ~Buffer Деструктор
			 */
			~Buffer() noexcept;
	} buffer_t;
};

#endif // __AWH_BUFFER__
