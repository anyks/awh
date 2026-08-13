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
 * \~russian
 * @brief Заголовочный файл контейнера бинарных данных — класс BinBox, хранящий последовательность записей
 *        произвольного размера в непрерывном буфере с итератором обхода и минимальными накладными расходами на запись
 *
 * \~english
 * @brief Header file of the binary data container — the BinBox class storing a sequence of records
 *        of arbitrary size in a contiguous buffer with a traversal iterator and minimal overhead per record
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
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
#include "../sys/fs.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../cryptography/crypto.hpp"

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
	 * @brief Класс контейнера бинарных данных
	 *
	 * \~english
	 * @brief Binary data container class
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ BinBox {
		public:
			/**
			 * \~russian
			 * @brief Структура контейнера бинарных данных
			 *
			 * @details Содержит размер записи и буфер бинарных данных.
			 *
			 * \~english
			 * @brief Structure of the binary data container
			 *
			 * @details Contains the record size and the binary data buffer.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Record {
				uintmax_t size;                 // Размер записи данных
				unique_ptr <uint8_t []> buffer; // Буфер записи данных
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Record() noexcept;
			} record_t;
		public:
			/**
			 * \~russian
			 * @brief Итератор как вложенный класс
			 *
			 * \~english
			 * @brief Iterator as a nested class
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Iterator {
				public:
					/**
					 * \~russian
					 * @brief Создаём необходимые нам типы данных
					 *
					 * \~english
					 * @brief Creating the data types we need
					 *
					 * \~
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
					 * \~russian
					 * @brief Оператор преобразования в сырой итератор
					 *
					 * @return iterator итератор для преобразования
					 *
					 * \~english
					 * @brief Operator of conversion into a raw iterator
					 *
					 * @return iterator iterator for the conversion
					 *
					 * \~
					 */
					operator iterator() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор извлечения указателя заголовка
					 *
					 * @return указатель заголовка
					 *
					 * \~english
					 * @brief Operator of extracting the header pointer
					 *
					 * @return header pointer
					 *
					 * \~
					 */
					pointer operator -> () noexcept;
					/**
					 * \~russian
					 * @brief Оператор разыменования заголовка
					 *
					 * @return значение заголовка
					 *
					 * \~english
					 * @brief Operator of dereferencing the header
					 *
					 * @return header value
					 *
					 * \~
					 */
					reference operator * () const noexcept;
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
					Iterator & operator ++ () noexcept;
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
					bool operator == (const Iterator & other) const noexcept;
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
					bool operator != (const Iterator & other) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param it  итератор для установки
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param it  iterator to set
					 * @param fmk framework object
					 * @param log object for working with logs
					 *
					 * \~
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
			 * \~russian
			 * @brief Метод очистки всех данных
			 *
			 * \~english
			 * @brief Method clearing all the data
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на пустое значение контейнера
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method checking the container for an empty value
			 *
			 * @return check result
			 *
			 * \~
			 */
			bool empty() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения количества записей в контейнере
			 *
			 * @return количество записей в контейнере
			 *
			 * \~english
			 * @brief Method obtaining the number of records in the container
			 *
			 * @return number of records in the container
			 *
			 * \~
			 */
			size_t count() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения названия контейнера
			 *
			 * @return название контейнера
			 *
			 * \~english
			 * @brief Method obtaining the container name
			 *
			 * @return container name
			 *
			 * \~
			 */
			string getName() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки названия контейнера
			 *
			 * @param name название контейнера
			 *
			 * \~english
			 * @brief Method setting the container name
			 *
			 * @param name container name
			 *
			 * \~
			 */
			void setName(string_view name) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения версии контейнера
			 *
			 * @return версия контейнера
			 *
			 * \~english
			 * @brief Method obtaining the container version
			 *
			 * @return container version
			 *
			 * \~
			 */
			string getVersion() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки версии контейнера
			 *
			 * @param version версия контейнера для установки
			 *
			 * \~english
			 * @brief Method setting the container version
			 *
			 * @param version container version to set
			 *
			 * \~
			 */
			void setVersion(string_view version) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод удаления записи по ключу
			 *
			 * @param key ключ для удаления записи
			 * @return    результат работы функции
			 *
			 * \~english
			 * @brief Method removing a record by key
			 *
			 * @param key key for removing the record
			 * @return    result of the function work
			 *
			 * \~
			 */
			bool erase(string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Метод удаления записи по идентификатору ключа
			 *
			 * @param idw идентификатор ключа для удаления записи
			 * @return    результат работы функции
			 *
			 * \~english
			 * @brief Method removing a record by key identifier
			 *
			 * @param idw key identifier for removing the record
			 * @return    result of the function work
			 *
			 * \~
			 */
			bool erase(const uint64_t idw) noexcept;
			/**
			 * \~russian
			 * @brief erase Метод удаления записи по итератору
			 *
			 * @param it итератор записи для удаления
			 * @return   следующий итератор
			 *
			 * \~english
			 * @brief erase Method removing a record by iterator
			 *
			 * @param it iterator of the record to remove
			 * @return   next iterator
			 *
			 * \~
			 */
			iterator_t erase(const iterator_t & it) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод загрузки контейнера из файла
			 *
			 * @param filename путь к файлу для загрузки
			 *
			 * \~english
			 * @brief Method loading the container from a file
			 *
			 * @param filename path to the file for loading
			 *
			 * \~
			 */
			void load(string_view filename) noexcept;
			/**
			 * \~russian
			 * @brief Метод сохранения контейнера в файл
			 *
			 * @param filename путь к файлу для сохранения
			 *
			 * \~english
			 * @brief Method saving the container into a file
			 *
			 * @param filename path to the file for saving
			 *
			 * \~
			 */
			void save(string_view filename) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод генерирования идентификатора ключа
			 *
			 * @param key ключ для генерации
			 * @return    идентификатор ключа
			 *
			 * \~english
			 * @brief Method generating a key identifier
			 *
			 * @param key key for the generation
			 * @return    key identifier
			 *
			 * \~
			 */
			uint64_t idw(string_view key) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на существование ключа
			 *
			 * @param key ключ для проверки
			 * @return    результат проверки
			 *
			 * \~english
			 * @brief Method checking a key for existence
			 *
			 * @param key key to check
			 * @return    check result
			 *
			 * \~
			 */
			bool has(string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки на существование идентификатора ключа
			 *
			 * @param idw идентификатор ключа для проверки
			 * @return    результат проверки
			 *
			 * \~english
			 * @brief Method checking a key identifier for existence
			 *
			 * @param idw key identifier to check
			 * @return    check result
			 *
			 * \~
			 */
			bool has(const uint64_t idw) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения размера данных по ключу
			 *
			 * @param key ключ записи
			 * @return    размер данных записи
			 *
			 * \~english
			 * @brief Method obtaining the data size by key
			 *
			 * @param key record key
			 * @return    data size of the record
			 *
			 * \~
			 */
			size_t size(string_view key) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения размера данных по идентификатору ключа
			 *
			 * @param idw идентификатор ключа
			 * @return    размер данных записи
			 *
			 * \~english
			 * @brief Method obtaining the data size by key identifier
			 *
			 * @param idw key identifier
			 * @return    data size of the record
			 *
			 * \~
			 */
			size_t size(const uint64_t idw) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения данных по ключу
			 *
			 * @param key ключ записи
			 * @return    запрашиваемые данные по ключу
			 *
			 * \~english
			 * @brief Method obtaining the data by key
			 *
			 * @param key record key
			 * @return    data requested by the key
			 *
			 * \~
			 */
			void * get(string_view key) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения данных по идентификатору ключа
			 *
			 * @param idw идентификатор ключа
			 * @return    запрашиваемые данные по идентификатору ключа
			 *
			 * \~english
			 * @brief Method obtaining the data by key identifier
			 *
			 * @param idw key identifier
			 * @return    data requested by the key identifier
			 *
			 * \~
			 */
			void * get(const uint64_t idw) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода чтения бинарных данных из бинарного контейнера
			 *
			 * @tparam T тип извлекаемого значения
			 *
			 * \~english
			 * @brief Template of the method reading binary data from the binary container
			 *
			 * @tparam T type of the extracted value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод чтения бинарных данных из бинарного контейнера
			 *
			 * @param key ключ записи
			 * @return    результат работы функции
			 *
			 * \~english
			 * @brief Method reading binary data from the binary container
			 *
			 * @param key record key
			 * @return    result of the function work
			 *
			 * \~
			 */
			T get(string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода чтения данных из бинарного контейнера
			 *
			 * @tparam T тип извлекаемого значения
			 *
			 * \~english
			 * @brief Template of the method reading data from the binary container
			 *
			 * @tparam T type of the extracted value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод чтения бинарных данных из бинарного контейнера
			 *
			 * @param idw идентификатор ключа
			 * @return    результат работы функции
			 *
			 * \~english
			 * @brief Method reading binary data from the binary container
			 *
			 * @param idw key identifier
			 * @return    result of the function work
			 *
			 * \~
			 */
			T get(const uint64_t idw) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param key    ключ записи
			 * @param buffer бинарный буфер для чтения данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method reading data from the binary container into a binary buffer
			 *
			 * @param key    record key
			 * @param buffer binary buffer for reading the data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool get(string_view key, vector <uint8_t> & buffer) noexcept;
			/**
			 * \~russian
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer бинарный буфер для чтения данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method reading data from the binary container into a binary buffer
			 *
			 * @param idw    key identifier
			 * @param buffer binary buffer for reading the data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool get(const uint64_t idw, vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param key    ключ записи
			 * @param buffer бинарный буфер для чтения данных
			 * @param size   размер извлекаемых бинарных данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method reading data from the binary container into a binary buffer
			 *
			 * @param key    record key
			 * @param buffer binary buffer for reading the data
			 * @param size   size of the extracted binary data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool get(string_view key, uint8_t ** buffer, size_t * size) noexcept;
			/**
			 * \~russian
			 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer бинарный буфер для чтения данных
			 * @param size   размер извлекаемых бинарных данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method reading data from the binary container into a binary buffer
			 *
			 * @param idw    key identifier
			 * @param buffer binary buffer for reading the data
			 * @param size   size of the extracted binary data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool get(const uint64_t idw, uint8_t ** buffer, size_t * size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 *
			 * \~english
			 * @brief Template of the method adding data into the binary container
			 *
			 * @tparam T type of the added value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления простого типа данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a simple data type
			 *
			 * @param idw   key identifier
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(const uint64_t idw, const T value) noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода добавления бинарного буфера данных
			 *
			 * @tparam T тип устанавливаемого значения
			 *
			 * \~english
			 * @brief Template of the method adding a binary data buffer
			 *
			 * @tparam T type of the set value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a binary data buffer
			 *
			 * @param idw   key identifier
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(const uint64_t idw, const vector <T> & value) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления строкового типа данных
			 *
			 * @param idw   идентификатор ключа
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a string data type
			 *
			 * @param idw   key identifier
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(const uint64_t idw, const string & value) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарных данных
			 *
			 * @param idw    идентификатор ключа
			 * @param buffer буфер для записи данных
			 * @param size   размер буфера для записи данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method adding binary data
			 *
			 * @param idw    key identifier
			 * @param buffer buffer for writing the data
			 * @param size   size of the buffer for writing the data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool add(const uint64_t idw, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 *
			 * \~english
			 * @brief Template of the method adding data into the binary container
			 *
			 * @tparam T type of the added value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления простого типа данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a simple data type
			 *
			 * @param key   record key
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(string_view key, const T value) noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода добавления данных в бинарный контейнер
			 *
			 * @tparam T тип добавляемого значения
			 *
			 * \~english
			 * @brief Template of the method adding data into the binary container
			 *
			 * @tparam T type of the added value
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод добавления бинарного буфера данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a binary data buffer
			 *
			 * @param key   record key
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(string_view key, const vector <T> & value) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод добавления строкового типа данных
			 *
			 * @param key   ключ записи
			 * @param value значение данных
			 * @return      результат работы функции
			 *
			 * \~english
			 * @brief Method adding a string data type
			 *
			 * @param key   record key
			 * @param value data value
			 * @return      result of the function work
			 *
			 * \~
			 */
			bool add(string_view key, const string & value) noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления бинарных данных
			 *
			 * @param key    ключ записи
			 * @param buffer буфер для записи данных
			 * @param size   размер буфера для записи данных
			 * @return       результат работы функции
			 *
			 * \~english
			 * @brief Method adding binary data
			 *
			 * @param key    record key
			 * @param buffer buffer for writing the data
			 * @param size   size of the buffer for writing the data
			 * @return       result of the function work
			 *
			 * \~
			 */
			bool add(string_view key, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод обмена данными контейнерами
			 *
			 * @param binbox объект для обмена
			 *
			 * \~english
			 * @brief Method of exchanging the data between containers
			 *
			 * @param binbox object for the exchange
			 *
			 * \~
			 */
			void swap(BinBox & binbox) noexcept;
		public:
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
			iterator_t end() noexcept;
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
			iterator_t begin() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод поиска записи по ключу
			 *
			 * @param key ключ для поиска записи
			 * @return     итератор указанного ключа
			 *
			 * \~english
			 * @brief Method searching for a record by key
			 *
			 * @param key key for searching the record
			 * @return     iterator of the specified key
			 *
			 * \~
			 */
			iterator_t find(string_view key) noexcept;
			/**
			 * \~russian
			 * @brief Метод поиска записи по идентификатору ключа
			 *
			 * @param idw идентификатор ключа для поиска записи
			 * @return    итератор указанного идентификатора ключа
			 *
			 * \~english
			 * @brief Method searching for a record by key identifier
			 *
			 * @param idw key identifier for searching the record
			 * @return    iterator of the specified key identifier
			 *
			 * \~
			 */
			iterator_t find(const uint64_t idw) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор проверки на существование контейнера
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Operator checking the container for existence
			 *
			 * @return check result
			 *
			 * \~
			 */
			operator bool() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор извлечения бинарного буфера данных
			 *
			 * @return бинарный буфер данных
			 *
			 * \~english
			 * @brief Operator of extracting the binary data buffer
			 *
			 * @return binary data buffer
			 *
			 * \~
			 */
			operator vector <uint8_t> () const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещения
			 *
			 * @param binbox объект для перемещения
			 * @return       текущий контейнер буфера
			 *
			 * \~english
			 * @brief Move assignment operator
			 *
			 * @param binbox object to move
			 * @return       current buffer container
			 *
			 * \~
			 */
			BinBox & operator = (BinBox && binbox) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор установки буфера бинарных данных
			 *
			 * @param buffer буфер бинарных данных
			 * @return       текущий объект
			 *
			 * \~english
			 * @brief Operator of setting the binary data buffer
			 *
			 * @param buffer binary data buffer
			 * @return       current object
			 *
			 * \~
			 */
			BinBox & operator = (const vector <uint8_t> & buffer) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор перемещения
			 *
			 * @param binbox объект для перемещения
			 *
			 * \~english
			 * @brief Move constructor
			 *
			 * @param binbox object to move
			 *
			 * \~
			 */
			explicit BinBox(BinBox && binbox) noexcept;
		public:
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
			explicit BinBox(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~BinBox() noexcept;
	} binbox_t;
};

#endif // __AWH_BINBOX_CONTAINER__
