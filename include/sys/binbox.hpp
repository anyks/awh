/**
 * @file: binbox.hpp
 * @date: 2026-02-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_BINBOX_CONTAINER__
#define __AWH_BINBOX_CONTAINER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fs.hpp"
#include "fmk.hpp"
#include "log.hpp"
#include "crypto.hpp"

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
	 * @brief Класс контейнера бинарных данных
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ BinBox {
		public:
			/**
			 * @brief Структура контейнера бинарных данных
			 *
			 * @details Содержит размер записи и буфер бинарных данных.
			 */
			typedef struct __AWH_SHARED_EXPORT__ Record {
				uintmax_t size;                 // Размер записи данных
				unique_ptr <uint8_t []> buffer; // Буфер записи данных
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Record() noexcept;
			} record_t;
		public:
			/**
			 * @brief Итератор как вложенный класс
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Iterator {
				public:
					/**
					 * @brief Создаём необходимые нам типы данных
					 *
					 */
					using value_type        = record_t;
					using pointer           = record_t *;
					using reference         = record_t &;
					using difference_type   = std::ptrdiff_t;
					using iterator_category = std::bidirectional_iterator_tag;
				public:
					/**
					 * Создаём тип данных итератора
					 */
					using iterator = unordered_map <uint64_t, record_t>::iterator;
				private:
					// Текущее значение итератора
					iterator _it;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Оператор преобразования в сырой итератор
					 *
					 * @return iterator итератор для преобразования
					 */
					operator iterator() noexcept;
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
					reference operator * () const noexcept;
				public:
					/**
					 * @brief Оператор смещения вперед
					 *
					 * @return значение текущего итератора
					 */
					Iterator & operator ++ () noexcept;
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
					 * @param it  итератор для установки
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					explicit Iterator(iterator it, const fmk_t * fmk, const log_t * log) noexcept;
			} iterator_t;
		private:
			// Название контейнера
			string _name;
		private:
			// Версия контейнера
			uint32_t _version;
		private:
			// Объект работы с файловой системой
			shared_ptr <fs_t> _fs;
			// Объект работы с криптографией
			shared_ptr <crypto_t> _crypto;
		private:
			/**
			 * Контейнер для хранения бинарных данных
			 */
			unordered_map <uint64_t, record_t> _records;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод очистки всех данных
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод проверки на пустое значение контейнера
			 *
			 * @return результат проверки
			 */
			bool empty() const noexcept;
		public:
			/**
			 * @brief Метод получения количества записей в контейнере
			 *
			 * @return количество записей в контейнере
			 */
			size_t count() const noexcept;
		public:
			/**
			 * @brief Метод получения названия контейнера
			 *
			 * @return название контейнера
			 */
			string getName() const noexcept;
			/**
			 * @brief Метод установки названия контейнера
			 *
			 * @param name название контейнера
			 */
			void setName(string_view name) noexcept;
		public:
			/**
			 * @brief Метод получения версии контейнера
			 *
			 * @return версия контейнера
			 */
			string getVersion() const noexcept;
			/**
			 * @brief Метод установки версии контейнера
			 *
			 * @param version версия контейнера для установки
			 */
			void setVersion(string_view version) noexcept;
		public:
			/**
			 * @brief Метод удаления записи по ключу
			 *
			 * @param key ключ для удаления записи
			 * @return    результат работы функции
			 */
			bool erase(string_view key) noexcept;
			/**
			 * @brief Метод удаления записи по идентификатору ключа
			 *
			 * @param idw идентификатор ключа для удаления записи
			 * @return    результат работы функции
			 */
			bool erase(const uint64_t idw) noexcept;
			/**
			 * @brief erase Метод удаления записи по итератору
			 *
			 * @param it итератор записи для удаления
			 * @return   следующий итератор
			 */
			iterator_t erase(const iterator_t & it) noexcept;
		public:
			/**
			 * @brief Метод загрузки контейнера из файла
			 *
			 * @param filename путь к файлу для загрузки
			 */
			void load(string_view filename) noexcept;
			/**
			 * @brief Метод сохранения контейнера в файл
			 *
			 * @param filename путь к файлу для сохранения
			 */
			void save(string_view filename) noexcept;
		public:
			/**
			 * @brief Метод генерирования идентификатора ключа
			 *
			 * @param key ключ для генерации
			 * @return    идентификатор ключа
			 */
			uint64_t idw(string_view key) const noexcept;
		public:
			/**
			 * @brief Метод проверки на существование ключа
			 *
			 * @param key ключ для проверки
			 * @return    результат проверки
			 */
			bool has(string_view key) noexcept;
			/**
			 * @brief Метод проверки на существование идентификатора ключа
			 *
			 * @param idw идентификатор ключа для проверки
			 * @return    результат проверки
			 */
			bool has(const uint64_t idw) noexcept;
		public:
			/**
			 * @brief Метод получения размера данных по ключу
			 *
			 * @param key ключ записи
			 * @return    размер данных записи
			 */
			size_t size(string_view key) const noexcept;
			/**
			 * @brief Метод получения размера данных по идентификатору ключа
			 *
			 * @param idw идентификатор ключа
			 * @return    размер данных записи
			 */
			size_t size(const uint64_t idw) const noexcept;
		public:
			/**
			 * @brief Метод получения данных по ключу
			 *
			 * @param key ключ записи
			 * @return    запрашиваемые данные по ключу
			 */
			void * get(string_view key) const noexcept;
			/**
			 * @brief Метод получения данных по идентификатору ключа
			 *
			 * @param idw идентификатор ключа
			 * @return    запрашиваемые данные по идентификатору ключа
			 */
			void * get(const uint64_t idw) const noexcept;
		public:
			/**
			 * @brief Шаблон метода чтения бинарных данных из бинарного контейнера
			 *
			 * @tparam T тип извлекаемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод чтения бинарных данных из бинарного контейнера
			 *
			 * @param key ключ записи
			 * @return    результат работы функции
			 */
			T get(string_view key) noexcept;
			/**
			 * @brief Шаблон метода чтения данных из бинарного контейнера
			 *
			 * @tparam T тип извлекаемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод чтения бинарных данных из бинарного контейнера
			 *
			 * @param idw идентификатор ключа
			 * @return    результат работы функции
			 */
			T get(const uint64_t idw) noexcept;
		public:
			/**
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param key    ключ записи
			 * @param buffer бинарный буфер для чтения данных
			 * @return       результат работы функции
			 */
			bool get(string_view key, vector <uint8_t> & buffer) noexcept;
			/**
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer бинарный буфер для чтения данных
			 * @return       результат работы функции
			 */
			bool get(const uint64_t idw, vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param key    ключ записи
			 * @param buffer бинарный буфер для чтения данных
			 * @param size   размер извлекаемых бинарных данных
			 * @return       результат работы функции
			 */
			bool get(string_view key, uint8_t ** buffer, size_t * size) noexcept;
			/**
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer бинарный буфер для чтения данных
			 * @param size   размер извлекаемых бинарных данных
			 * @return       результат работы функции
			 */
			bool get(const uint64_t idw, uint8_t ** buffer, size_t * size) noexcept;
		public:
			/**
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод добавления простого типа данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(const uint64_t idw, const T value) noexcept;
			/**
			 * @brief Шаблон метода добавления бинарного буфера данных
			 *
			 * @tparam T тип устанавливаемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод добавления бинарного буфера данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(const uint64_t idw, const vector <T> & value) noexcept;
		public:
			/**
			 * @brief Метод добавления строкового типа данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(const uint64_t idw, const string & value) noexcept;
			/**
			 * @brief Метод добавления бинарных данных
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer буфер для записи данных
			 * @param size   размер буфера для записи данных
			 * @return       результат работы функции
			 */
			bool add(const uint64_t idw, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод добавления простого типа данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(string_view key, const T value) noexcept;
			/**
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 */
			template <typename T>
			/**
			 * @brief Метод добавления бинарного буфера данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(string_view key, const vector <T> & value) noexcept;
		public:
			/**
			 * @brief Метод добавления строкового типа данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 */
			bool add(string_view key, const string & value) noexcept;
			/**
			 * @brief Метод добавления бинарных данных
			 *
			 * @param key    ключ записи
			 * @param buffer буфер для записи данных
			 * @param size   размер буфера для записи данных
			 * @return       результат работы функции
			 */
			bool add(string_view key, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод обмена данными контейнерами
			 *
			 * @param binbox объект для обмена
			 */
			void swap(BinBox & binbox) noexcept;
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
			 * @brief Метод поиска записи по ключу
			 *
			 * @param key ключ для поиска записи
			 * @return     итератор указанного ключа
			 */
			iterator_t find(string_view key) noexcept;
			/**
			 * @brief Метод поиска записи по идентификатору ключа
			 *
			 * @param idw идентификатор ключа для поиска записи
			 * @return    итератор указанного идентификатора ключа
			 */
			iterator_t find(const uint64_t idw) noexcept;
		public:
			/**
			 * @brief Оператор проверки на существование контейнера
			 *
			 * @return результат проверки
			 */
			operator bool() const noexcept;
			/**
			 * @brief Оператор извлечения бинарного буфера данных
			 *
			 * @return бинарный буфер данных
			 */
			operator vector <uint8_t> () const noexcept;
		public:
			/**
			 * @brief Оператор перемещения
			 *
			 * @param binbox объект для перемещения
			 * @return       текущий контейнер буфера
			 */
			BinBox & operator = (BinBox && binbox) noexcept;
		public:
			/**
			 * @brief Оператор установки буфера бинарных данных
			 *
			 * @param buffer буфер бинарных данных
			 * @return       текущий объект
			 */
			BinBox & operator = (const vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * @brief Конструктор перемещения
			 *
			 * @param binbox объект для перемещения
			 */
			explicit BinBox(BinBox && binbox) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit BinBox(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~BinBox() noexcept;
	} binbox_t;
};

#endif // __AWH_BINBOX_CONTAINER__
