/**
 * @file: queue.hpp
 * @date: 2025-10-03
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

#ifndef __AWH_HEADERS__
#define __AWH_HEADERS__

/**
 * Если максимальное количество заголовков не указано
 */
#ifndef AWH_MAX_COUNT_HEADERS
	/**
	 * Устанавливаем максимальное количество заголовков в 500
	 */
	#define AWH_MAX_COUNT_HEADERS 0x1F4
#endif

/**
 * Если максимальное значение потребляемой памяти не указано
 */
#ifndef AWH_MAX_MEMORY_HEADERS
	/**
	 * Устанавливаем максимальное значение потребляемой памяти 10Mb
	 */
	#define AWH_MAX_MEMORY_HEADERS 0xA00000
#endif

/**
 * Стандартная библиотека
 */
#include <map>
#include <mutex>
#include <vector>
#include <cstdint>
#include <unordered_map>

/**
 * Подключаем наши заголовочные файлы
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса контейнера HTTP-заголовков
	 *
	 */
	class Headers;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс контейнера HTTP-заголовков
	 *
	 */
	typedef class AWH_SHARED_EXPORT Headers {
		private:
			/**
			 * @brief Значение записи итератора
			 * 
			 */
			using item_t = std::pair <string, string>;
		public:
			/**
			 * @brief Итератор как вложенный класс
			 * 
			 */
			typedef class Iterator {
				private:
					// Объект родительского контейнера
					Headers * _ctx;
				public:
					/**
					 * Создаём необходимые нам типы данных
					 */
					using value_type        = item_t;
					using pointer           = item_t *;
					using reference         = item_t &;
					using difference_type   = std::ptrdiff_t;
					using iterator_category = std::bidirectional_iterator_tag;
				public:
					/**
					 * Создаём тип данных итератора
					 */
					using iterator = std::multimap <uint32_t, uintptr_t>::iterator;
				private:
					// Текущее значение итератора
					iterator _it;
				public:
					/**
					 * @brief Оператор извлечения указателя заголовка
					 * 
					 * @return указатель заголовка
					 */
					pointer operator -> () noexcept;
					/**
					 * @brief Оператор разыменования заголовка
					 * 
					 * @return значение заголовка
					 */
					reference operator * () noexcept;
				public:
					/**
					 * @brief Оператор смещения вперед
					 * 
					 * @return значение текущего итератора
					 */
					Iterator & operator ++ () noexcept;
					/**
					 * @brief Оператор смещения вперёд на указанную позицию
					 * 
					 * @return значение текущего итератора
					 */
					Iterator operator ++ (const int32_t) noexcept;
				public:
					/**
					 * @brief Оператор смещения назад
					 * 
					 * @return значение текущего итератора
					 */
					Iterator & operator -- () noexcept;
					/**
					 * @brief Оператор смещения назад на указанную позицию
					 * 
					 * @return значение текущего итератора
					 */
					Iterator operator -- (const int32_t) noexcept;
				public:
					/**
					 * @brief Оператор сравнения соответствия итератора
					 * 
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 */
					bool operator == (const Iterator & other) const noexcept;
					/**
					 * @brief Оператора сравнения несоответствия итератора
					 * 
					 * @param other итератор для сравнения
					 * @return      результат сравнения
					 */
					bool operator != (const Iterator & other) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param ctx объект родительского контейнера
					 * @param it   итератор для установки
					 */
					explicit Iterator(Headers * ctx, iterator it) noexcept : _ctx(ctx), _it(it) {}
			} iterator_t;
		private:
			/**
			 * @brief Структура диапазона записей
			 * 
			 */
			typedef struct Range {
				size_t end;   // Конец записи
				size_t begin; // Начало записи
				size_t count; // Количество добавленных записей
				/**
				 * @brief Конструктор
				 * 
				 */
				Range() noexcept : end(0), begin(0), count(0) {}
			} __attribute__((packed)) range_t;
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
				 memory(AWH_MAX_MEMORY_HEADERS),
				 records(AWH_MAX_COUNT_HEADERS) {}
			} __attribute__((packed)) max_t;
		private:
			// Размеры максимальныйх ограничений
			max_t _max;
		private:
			// Текущее значение записи
			item_t _item;
		private:
			// Объект диапазонов записей
			range_t _range;
		private:
			// Мютекс для блокировки потока
			mutable std::mutex _mtx;
		private:
			// Буфер данных выделенной памяти
			vector <uint8_t> _buffer;
		private:
			// Список записей HTTP-заголовков
			std::multimap <uint32_t, uintptr_t> _records;
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
			 * @brief Метод проверки на заполненность очереди
			 *
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * @brief Количество добавленных заголовков
			 *
			 * @return количество добавленных заголовков
			 */
			size_t count() const noexcept;
		public:
			/**
			 * @brief Метод печати содержимого заголовков в формате HTTP/1.1
			 * 
			 * @return заголовки в формате HTTP/1.1
			 */
			string print() const noexcept;
			/**
			 * @brief Метод печати содержимого заголовка
			 * 
			 * @param name печать заголовка в формате HTTP/1.1
			 * @return     распечатанный заголовок
			 */
			string print(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод удаления заголовка
			 *
			 * @param name название удаляемого заголовка
			 */
			void erase(const string & name) noexcept;
		public:
			/**
			 * @brief Метод проверки существования заголовка
			 * 
			 * @param name название заголовка для проверки
			 * @return     результат выполнения проверки
			 */
			bool has(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод извлечения содержимого заголовка
			 * 
			 * @param name название заголовка
			 * @return     содержимое заголовка
			 */
			string at(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод извлечения названий заголовков
			 * 
			 * @return список названий заголовков
			 */
			vector <string> names() const noexcept;
		public:
			/**
			 * @brief Метод вывода списка значений одинаковых заголовков
			 * 
			 * @param name название заголовка
			 * @return     список значений одинаковых заголовков
			 */
			vector <string> range(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод добавления нового заголовка
			 *
			 * @param name    название заголовка
			 * @param content содержимое заголовка
			 * @return        результат выполнения операции
			 */
			bool emplace(const string & name, const string & content) noexcept;
		public:
			/**
			 * @brief Метод установки максимального размера потребления памяти
			 * 
			 * @param size максимальный размер потребления памяти
			 */
			void setMaxMemory(const size_t size) noexcept;
			/**
			 * @brief Метод установки максимального количества заголовков
			 * 
			 * @param count максимальное количество заголовков
			 */
			void setMaxRecords(const size_t count) noexcept;
		public:
			/**
			 * @brief Метод обмена заголовками
			 * 
			 * @param headers заголовки для обмена
			 */
			void swap(Headers & headers) noexcept;
		public:
			/**
			 * @brief Метод получения конечного итератора
			 * 
			 * @return конечный итератор
			 */
			iterator_t end() noexcept;
			/**
			 * @brief Метод получение начального итератора
			 * 
			 * @return начальный итератор
			 */
			iterator_t begin() noexcept;
		public:
			/**
			 * @brief Метод поиска указанного заголовка
			 * 
			 * @param name название заголовка для поиска
			 * @return     итератор указанного заголовка
			 */
			iterator_t find(const string & name) noexcept;
		public:
			/**
			 * @brief Оператор получения количество заголовков
			 *
			 * @return количество заголовков
			 */
			operator size_t() const noexcept;
			/**
			 * @brief Оператор печати содержимого заголовков в формате HTTP/1.1
			 *
			 * @return заголовки в формате HTTP/1.1
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор получения списка заголовков в том виде как они есть
			 *
			 * @return список всех добавленных заголовков
			 */
			operator vector <std::pair <string, string>> () const noexcept;
		public:
			/**
			 * @brief Оператор получения списка заголовков
			 *
			 * @return список всех добавленных заголовков
			 */
			operator std::unordered_map <string, string> () const noexcept;
			/**
			 * @brief Оператор получения списка заголовков
			 *
			 * @return список всех добавленных заголовков
			 */
			operator std::unordered_multimap <string, string> () const noexcept;
		public:
			/**
			 * @brief Оператор извлечения содержимого заголовка
			 * 
			 * @param name название заголовка для извлечения
			 * @return     содержимое заголовка
			 */
			string operator[](const string & name) const noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param headers заголовки для перемещения
			 * @return        текущий контейнер заголовков
			 */
			Headers & operator = (Headers && headers) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param headers заголовки для копирования
			 * @return        текущий контейнер заголовков
			 */
			Headers & operator = (const Headers & headers) noexcept;
		public:
			/**
			 * @brief Оператор копирования
			 *
			 * @param headers заголовки для копирования
			 * @return        текущий контейнер заголовков
			 */
			Headers & operator = (const vector <std::pair <string, string>> & headers) noexcept;
		public:
			/**
			 * @brief Оператор копирования
			 *
			 * @param headers заголовки для копирования
			 * @return        текущий контейнер заголовков
			 */
			Headers & operator = (const std::unordered_map <string, string> & headers) noexcept;
			/**
			 * @brief Оператор копирования
			 *
			 * @param headers заголовки для копирования
			 * @return        текущий контейнер заголовков
			 */
			Headers & operator = (const std::unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * @brief Оператор сравнения двух заголовков
			 *
			 * @param headers заголовки для сравнения
			 * @return        результат сравнения
			 */
			bool operator == (const Headers & headers) const noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param headers заголовки для перемещения
			 */
			Headers(Headers && headers) noexcept;
			/**
			 * @brief Конструктор копирования
			 *
			 * @param headers заголовки для копирования
			 */
			Headers(const Headers & headers) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Headers(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Headers() noexcept;
	} headers_t;
};

#endif // __AWH_HEADERS__
