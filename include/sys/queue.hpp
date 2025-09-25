/**
 * @file: queue.hpp
 * @date: 2025-09-24
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
	 * Устанавливаем максимальное значение потребляемой памяти 512Мб
	 */
	#define AWH_MAX_MEMORY_QUEUE 0x20000000
#endif

/**
 * Стандартная библиотека
 */
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>
#include <condition_variable>

/**
 * Подключаем наши заголовочные файлы
 */
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
	 * @brief Класс создания очереди
	 *
	 */
	typedef class AWHSHARED_EXPORT Queue {
		private:
			/**
			 * @brief Структура условных переменных
			 * 
			 */
			typedef struct CV {
				// Условная переменная на чтение данных
				std::condition_variable read;
				// Условная переменная на запись данных
				std::condition_variable write;
			} cv_t;
			/**
			 * @brief Структура итерации записей
			 * 
			 */
			typedef struct Iterator {
				size_t end;   // Конец записи
				size_t begin; // Начало записи
				size_t count; // Количество добавленных записей
				/**
				 * @brief Конструктор
				 * 
				 */
				Iterator() noexcept : end(0), begin(0), count(0) {}
			} __attribute__((packed)) iter_t;
			/**
			 * @brief Структура параметров максимальных значений
			 * 
			 */
			typedef struct Max {
				size_t memory;  // Максимальный размер выделения памяти
				size_t records; // Максимальное количество добавляемых записей
				/**
				 * @brief Конструктор
				 * 
				 */
				Max() noexcept :
				 memory(AWH_MAX_MEMORY_QUEUE),
				 records(AWH_MAX_RECORDS_QUEUE) {}
			} __attribute__((packed)) max_t;
		public:
			/**
			 * Создаём тип данных добавляемой записи
			 */
			typedef std::pair <const void *, size_t> record_t;
		private:
			// Размеры максимальныйх ограничений
			max_t _max;
		private:
			// Объект итерации данных
			iter_t _iter;
		private:
			// Условные переменные
			mutable cv_t _cv;
		private:
			// Мютекс для блокировки потока
			mutable std::mutex _mtx;
		private:
			// Буфер данных выделенной памяти
			vector <uint8_t> _buffer;
		private:
			// Флаг завершения работы очереди
			std::atomic_bool _terminate;
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
			 */
			size_t count() const noexcept;
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
			 * @return количество памяти которую занимает очередь
			 */
			size_t capacity() const noexcept;
		public:
			/**
			 * @brief Получения данных указанного элемента в очереди
			 *
			 * @return указатель на элемент очереди
			 */
			const void * data() const noexcept;
		public:
			/**
			 * @brief Метод проверки на заполненность очереди
			 *
			 * @param timeout время ожидания в миллисекундах
			 * @return        результат проверки
			 */
			bool empty(const uint32_t timeout = 0) const noexcept;
		public:
			/**
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 * @return       текущий размер очереди
			 */
			size_t push(const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод добавления бинарного буфера данных в очередь
			 *
			 * @param records список бинарных буферов для добавления
			 * @param size    общий размер добавляемых данных
			 * @return        текущий размер очереди
			 */
			size_t push(const vector <record_t> & records, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки максимального размера потребления памяти
			 * 
			 * @param size максимальный размер потребления памяти
			 */
			void setMaxMemory(const size_t size) noexcept;
			/**
			 * @brief Метод установки максимального количества записей очереди
			 * 
			 * @param count максимальное количество записей очереди
			 */
			void setMaxRecords(const size_t count) noexcept;
		public:
			/**
			 * @brief Метод обмена очередями
			 * 
			 * @param queue очередь для обмена
			 */
			void swap(Queue & queue) noexcept;
		public:
			/**
			 * @brief Получения размера данных в очереди
			 *
			 * @return размер данных в очереди
			 */
			operator size_t() const noexcept;
			/**
			 * @brief Получения бинарных данных очереди
			 *
			 * @return бинарные данные очереди
			 */
			operator const char * () const noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param queue очередь для перемещения
			 * @return      текущий контейнер очереди
			 */
			Queue & operator = (Queue && queue) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param queue очередь для копирования
			 * @return      текущий контейнер очереди
			 */
			Queue & operator = (const Queue & queue) noexcept;
		public:
			/**
			 * @brief Оператор сравнения двух очередей
			 *
			 * @param queue очередь для сравнения
			 * @return      результат сравнения
			 */
			bool operator == (const Queue & queue) const noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param queue очередь для перемещения
			 */
			Queue(Queue && queue) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param queue очередь для копирования
			 */
			Queue(const Queue & queue) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Queue(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Queue() noexcept;
	} queue_t;
};

#endif // __AWH_QUEUE__
