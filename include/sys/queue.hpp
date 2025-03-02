/**
 * @file: queue.hpp
 * @date: 2024-12-17
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
#include <vector>
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
	 * Queue Класс создания очереди
	 */
	typedef class AWHSHARED_EXPORT Queue {
		public:
			/**
			 * Позиция в очереди
			 */
			enum class pos_t : uint8_t {
				NONE  = 0x00, // Позиция не определена
				BACK  = 0x01, // Конец очереди
				FRONT = 0x02  // Начало очереди
			};
		public:
			/**
			 * Создаём тип данных инарного буфера
			 */
			typedef pair <const void *, size_t> buffer_t;
		private:
			// Количество аллоцированных элементов
			static constexpr uint16_t BATCH = 0x3E8;
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
			// Размер батча добавляемых элементов
			size_t _batch;
		private:
			// Размер всех добавленных данных
			size_t _bytes;
		private:
			// Размеры добавленных данных
			size_t * _sizes;
			// Адреса добавленных данных
			uint64_t * _data;
		private:
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * realloc Метод увеличения памяти под записи
			 */
			void realloc() noexcept;
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
			 * count Количество добавленных элементов
			 * @return количество добавленных элементов
			 */
			size_t count() const noexcept;
		public:
			/**
			 * reserve Метод резервирования размера очереди
			 * @param count размер очереди для аллокации
			 */
			void reserve(const size_t count) noexcept;
		public:
			/**
			 * pop Метод удаления записи в очереди
			 * @param pos позиция в очереди
			 */
			void pop(const pos_t pos = pos_t::FRONT) noexcept;
		public:
			/**
			 * size Метод получения размера добавленных данных
			 * @param pos позиция в очереди
			 * @return    размер всех добавленных данных
			 */
			size_t size(const pos_t pos = pos_t::FRONT) const noexcept;
		public:
			/**
			 * get Получения данных указанного элемента в очереди
			 * @param pos позиция в очереди
			 * @return    указатель на элемент очереди
			 */
			const void * get(const pos_t pos = pos_t::FRONT) const noexcept;
		public:
			/**
			 * push Метод добавления бинарного буфера данных в очередь
			 * @param buffer бинарный буфер для добавления
			 * @param size   размер бинарного буфера
			 */
			void push(const void * buffer, const size_t size) noexcept;
			/**
			 * push Метод добавления бинарного буфера данных в очередь
			 * @param buffers список бинарных буферов для добавления
			 * @param size    общий размер добавляемых данных
			 */
			void push(const vector <buffer_t> & buffers, const size_t size) noexcept;
		public:
			/**
			 * Queue Конструктор
			 * @param log объект для работы с логами
			 */
			Queue(const log_t * log) noexcept;
			/**
			 * ~Queue Деструктор
			 */
			~Queue() noexcept;
	} queue_t;
};

#endif // __AWH_QUEUE__
