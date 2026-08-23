/**
 * @file document.hpp
 * @date 2026-08-12
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл контейнера CSV — класс Document, удерживающий таблицу целиком
 *        и дающий доступ к полям по номеру записи и по имени столбца
 *
 * \~english
 * @brief Header file of the CSV container — the Document class, which holds the table in full
 *        and gives access to the fields by the number of a record and by the name of a column
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV_DOCUMENT__
#define __AWH_CODEC_CSV_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
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
	 * @brief Пространство имён контейнеров данных
	 *
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера CSV
		 *
		 *
		 * \~english
		 * @brief CSV container namespace
		 *
		 * \~
		 */
		namespace csv {
			/**
			 * \~russian
			 * @brief Класс контейнера CSV
			 *
			 * @details Удерживает таблицу целиком и даёт доступ к полям по номеру записи
			 * и по имени столбца. Годится там, где таблица помещается в память и нужна
			 * вся сразу; для таблиц, в память не помещающихся, есть потоковое чтение
			 *
			 * @par Порядок работы
			 *
			 * @note Поля хранятся единым хранилищем знаков, а записи - указаниями в него:
			 * таблица на миллион полей иначе стоила бы миллиона отдельных строк со всеми
			 * их заголовками и выделениями памяти
			 *
			 *  @code{.cpp}
			 *  document_t document;
			 *
			 *  document.parse(text);
			 *
			 *  for(size_t i = 0; i < document.rows(); i++)
			 *      const string_view value = document.get(i, "имя");
			 *
			 *  const string result = document.text();
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the CSV container
			 * @details Holds the table in full and gives access to the fields by the number of a record
			 * and by the name of a column. Suitable where the table fits into the memory and is needed
			 * all at once; for the tables that do not fit into the memory there is the streaming reading
			 * @par Order of the work
			 * @note The fields are stored in a single storage of the characters, while the records — as pointers into it:
			 * a table of a million fields would otherwise cost a million separate strings with all
			 * their headers and memory allocations
			 *
			 *  @code{.cpp}
			 *  document_t document;
			 *
			 *  document.parse(text);
			 *
			 *  for(size_t i = 0; i < document.rows(); i++)
			 *      const string_view value = document.get(i, "name");
			 *
			 *  const string result = document.text();
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Код отказа остаётся доступен потребителю через error(): журнал
					 * его не заменяет, а лишь оповещает о случившемся
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 * @details The code of the refusal remains available to the consumer through error():
					 * the log does not replace it but merely notifies about what has happened
					 *
					 * \~
					 */
					void report() const noexcept;
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * \~english
					 * Object for working with logs
					 *
					 * \~
					 */
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Настройки контейнера
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать
					 * таблицу с одним разделителем и записать её с другим - обыкновенное
					 * дело, и связывать их незачем
					 *
					 * \~english
					 * @brief Settings of the container
					 * @details The settings of the parsing and of the writing are kept separately: to read
					 * a table with one separator and to write it with another one is an ordinary
					 * matter, and there is no point in binding them together
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Настройки разбора текста
						reader_t::settings_t reader;
						// Настройки записи текста
						writer_t::settings_t writer;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Settings() noexcept {}
					} settings_t;
				private:
					// Настройки контейнера
					settings_t _settings;
				private:
					// Код ошибки разбора
					error_t _error;
					// Положение ошибки в исходном тексте
					location_t _location;
				private:
					// Хранилище знаков полей таблицы
					string _storage;
					// Хранилище имён столбцов
					string _names;
				private:
					// Указания на поля таблицы в хранилище знаков
					vector <span_t> _fields;
					// Указания на начало каждой записи в перечне полей
					vector <uint32_t> _records;
					// Указания на имена столбцов в хранилище имён
					vector <span_t> _header;
				private:
					// Соответствие имён столбцов их номерам
					unordered_map <string_view, uint32_t> _columns;
				private:
					/**
					 * \~russian
					 * Признак того, что запись начата и ещё не завершена
					 *
					 * @note Признак держится членом, а не выводится из накопленного:
					 *       события приходят кусками, и между двумя вызовами сбора
					 *       запись вправе остаться незавершённой
					 *
					 * \~english
					 * Flag of a record having been begun and not yet completed
					 * @note The flag is kept as a member rather than inferred from what has been accumulated:
					 *       the events arrive by chunks, and between two calls of the assembly
					 *       a record has the right to remain unfinished
					 *
					 * \~
					 */
					bool _opened;
				private:
					/**
					 * \~russian
					 * @brief Метод получения содержимого по указанию в хранилище знаков
					 *
					 * @param span указание на содержимое в хранилище знаков
					 * @return     содержимое, на которое указывает указание
					 *
					 * \~english
					 * @brief Method of getting the content by a pointer into the storage of the characters
					 * @param span pointer to the content in the storage of the characters
					 * @return     content to which the pointer points
					 *
					 * \~
					 */
					string_view get(const span_t & span) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перестроения соответствия имён столбцов их номерам
					 *
					 * @details Перестроение выполняется после всякого изменения заголовка:
					 * хранилище имён при росте перемещается, обесценивая ссылки, по каким
					 * ведётся поиск
					 *
					 * \~english
					 * @brief Method of rebuilding the correspondence of the names of the columns to their numbers
					 * @details The rebuilding is performed after every change of the header:
					 * the storage of the names is moved at a growth, invalidating the references by which
					 * the search is conducted
					 *
					 * \~
					 */
					void reindex() noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора событий разбора в таблицу
					 *
					 * @details Собирает выданное чтением, не полагаясь на то, подан ли
					 * текст целиком или кусками: тем одним методом обслуживаются и разбор
					 * готового текста, и чтение файла по кускам
					 *
					 * @param reader чтение, выдающее события разбора
					 *
					 * \~english
					 * @brief Method of assembling the parsing events into the table
					 * @details Assembles what has been issued by the reading without relying on whether
					 * the text has been fed in full or by chunks: by that single method both the parsing
					 * of a ready text and the reading of a file by chunks are served
					 * @param reader reading which issues the parsing events
					 *
					 * \~
					 */
					void consume(reader_t & reader) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста таблицы
					 *
					 * @param text разбираемый текст таблицы
					 * @return     результат разбора
					 *
					 * \~english
					 * @brief Method of parsing the text of a table
					 * @param text text of the table being parsed
					 * @return     result of the parsing
					 *
					 * \~
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста таблицы с заданными настройками
					 *
					 * @param text     разбираемый текст таблицы
					 * @param settings настройки контейнера
					 * @return         результат разбора
					 *
					 * \~english
					 * @brief Method of parsing the text of a table with the given settings
					 * @param text     text of the table being parsed
					 * @param settings settings of the container
					 * @return         result of the parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла
					 *
					 * @details Файл читается кусками и разбирается потоково: таблица в
					 * несколько гигабайт целиком в память не поднимается, в память ложится
					 * лишь разобранное
					 *
					 * @param filename адрес файла таблицы для чтения
					 * @return         результат чтения
					 *
					 * \~english
					 * @brief Method of reading a table from a file
					 * @details The file is read by chunks and parsed in a streaming way: a table of
					 * several gigabytes is not raised into the memory in full, only what has been parsed
					 * goes into the memory
					 * @param filename address of the file of the table to be read
					 * @return         result of the reading
					 *
					 * \~
					 */
					bool read(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла записями
					 *
					 * @details Записи выдаются по одной, а в памяти держится лишь текущая:
					 * файл любого размера проходит через чтение, не оседая в ней. Таблица
					 * при этом не заполняется вовсе - тем и отличается настоящий метод от
					 * чтения, удерживающего таблицу
					 *
					 * @note Заголовок, если он объявлен настройками, обработчику не
					 * выдаётся: имена столбцов берутся у header по окончании чтения либо
					 * прямо из обработчика
					 *
					 * @warning Поля живут лишь на время вызова обработчика: буфер записи
					 * переиспользуется, и сохранять ссылки на поля дольше вызова нельзя -
					 * содержимое, нужное дольше, следует копировать
					 *
					 * @param filename адрес файла таблицы для чтения
					 * @param callback обработчик очередной записи, ложь прекращает чтение
					 * @return         результат чтения
					 *
					 * \~english
					 * @brief Method of reading a table from a file by records
					 * @details The records are issued one by one, while only the current one is kept in the memory:
					 * a file of any size passes through the reading without settling in it. The table
					 * is thereby not filled at all — that is what distinguishes the present method from
					 * the reading that holds the table
					 * @note The header, if it is declared by the settings, is not issued to the
					 * handler: the names of the columns are taken from header upon the end of the reading or
					 * right from the handler
					 * @warning The fields live only for the duration of the call of the handler: the buffer of a record
					 * is reused, and the references to the fields cannot be preserved longer than the call —
					 * the content needed for longer should be copied
					 * @param filename address of the file of the table to be read
					 * @param callback handler of the next record, false terminates the reading
					 * @return         result of the reading
					 *
					 * \~
					 */
					bool read(const string & filename, const function <bool (const vector <string_view> &)> & callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста таблицы записями
					 *
					 * @param text     разбираемый текст таблицы
					 * @param callback обработчик очередной записи, ложь прекращает разбор
					 * @return         результат разбора
					 *
					 * \~english
					 * @brief Method of parsing the text of a table by records
					 * @param text     text of the table being parsed
					 * @param callback handler of the next record, false terminates the parsing
					 * @return         result of the parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, const function <bool (const vector <string_view> &)> & callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи таблицы в файл
					 *
					 * @param filename адрес файла таблицы для записи
					 * @return         результат записи
					 *
					 * \~english
					 * @brief Method of writing a table into a file
					 * @param filename address of the file of the table to be written
					 * @return         result of the writing
					 *
					 * \~
					 */
					bool write(const string & filename) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки разбора
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 * @return error code of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения ошибки разбора
					 *
					 * @return положение ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of a parsing error
					 * @return position of the error in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества записей таблицы
					 *
					 * @details Заголовок записью не считается: количество это отвечает
					 * количеству записей со значениями
					 *
					 * @return количество записей таблицы
					 *
					 * \~english
					 * @brief Method of getting the number of the records of the table
					 * @details The header is not counted as a record: this number corresponds to
					 * the number of the records with the values
					 * @return number of the records of the table
					 *
					 * \~
					 */
					size_t rows() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества столбцов таблицы
					 *
					 * @details Количеством столбцов считается наибольшее количество полей
					 * среди записей: записи с разным числом полей договор дозволяет, и
					 * брать здесь количество полей первой записи значило бы терять поля
					 *
					 * @return количество столбцов таблицы
					 *
					 * \~english
					 * @brief Method of getting the number of the columns of the table
					 * @details The number of the columns is the largest number of the fields
					 * among the records: the protocol permits the records with a differing number of fields, and
					 * to take the number of the fields of the first record here would mean to lose the fields
					 * @return number of the columns of the table
					 *
					 * \~
					 */
					size_t cols() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества полей записи
					 *
					 * @param row номер записи, считая с нуля
					 * @return    количество полей записи
					 *
					 * \~english
					 * @brief Method of getting the number of the fields of a record
					 * @param row number of the record, counting from zero
					 * @return    number of the fields of the record
					 *
					 * \~
					 */
					size_t size(const size_t row) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения имён столбцов
					 *
					 * @return имена столбцов в порядке объявления
					 *
					 * \~english
					 * @brief Method of getting the names of the columns
					 * @return names of the columns in the order of the declaration
					 *
					 * \~
					 */
					vector <string_view> header() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия столбца с заданным именем
					 *
					 * @param name имя проверяемого столбца
					 * @return     результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of a column with a given name
					 * @param name name of the column being checked
					 * @return     result of the check
					 *
					 * \~
					 */
					bool has(const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения номера столбца по его имени
					 *
					 * @param name имя искомого столбца
					 * @return     номер столбца либо признак отсутствия
					 *
					 * \~english
					 * @brief Method of getting the number of a column by its name
					 * @param name name of the column being sought
					 * @return     number of the column or the sign of the absence
					 *
					 * \~
					 */
					uint32_t column(const string_view name) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения содержимого поля по номеру записи и столбца
					 *
					 * @param row номер записи, считая с нуля
					 * @param col номер столбца, считая с нуля
					 * @return    содержимое поля, пустое при его отсутствии
					 *
					 * \~english
					 * @brief Method of getting the content of a field by the number of the record and of the column
					 * @param row number of the record, counting from zero
					 * @param col number of the column, counting from zero
					 * @return    content of the field, empty in its absence
					 *
					 * \~
					 */
					string_view get(const size_t row, const size_t col) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого поля по номеру записи и имени столбца
					 *
					 * @param row  номер записи, считая с нуля
					 * @param name имя столбца
					 * @return     содержимое поля, пустое при его отсутствии
					 *
					 * \~english
					 * @brief Method of getting the content of a field by the number of the record and the name of the column
					 * @param row  number of the record, counting from zero
					 * @param name name of the column
					 * @return     content of the field, empty in its absence
					 *
					 * \~
					 */
					string_view get(const size_t row, const string_view name) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения записи целиком
					 *
					 * @param row номер записи, считая с нуля
					 * @return    поля записи в порядке следования
					 *
					 * \~english
					 * @brief Method of getting a record in full
					 * @param row number of the record, counting from zero
					 * @return    fields of the record in the order of the succession
					 *
					 * \~
					 */
					vector <string_view> row(const size_t row) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения столбца целиком
					 *
					 * @param col номер столбца, считая с нуля
					 * @return    поля столбца в порядке следования записей
					 *
					 * \~english
					 * @brief Method of getting a column in full
					 * @param col number of the column, counting from zero
					 * @return    fields of the column in the order of the succession of the records
					 *
					 * \~
					 */
					vector <string_view> col(const size_t col) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения столбца целиком по его имени
					 *
					 * @param name имя столбца
					 * @return     поля столбца в порядке следования записей
					 *
					 * \~english
					 * @brief Method of getting a column in full by its name
					 * @param name name of the column
					 * @return     fields of the column in the order of the succession of the records
					 *
					 * \~
					 */
					vector <string_view> col(const string_view name) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод приведения содержимого поля к числу либо логическому значению
					 *
					 * @tparam T тип получаемого значения
					 * @param row номер записи, считая с нуля
					 * @param col номер столбца, считая с нуля
					 * @param result полученное значение
					 * @return       результат приведения
					 *
					 * \~english
					 * @brief Method of converting the content of a field to a number or to a logical value
					 * @tparam T type of the value being obtained
					 * @param row number of the record, counting from zero
					 * @param col number of the column, counting from zero
					 * @param result obtained value
					 * @return       result of the conversion
					 *
					 * \~
					 */
					template <typename T>
					bool numeric(const size_t row, const size_t col, T & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки заголовка таблицы
					 *
					 * @details Заголовок задаётся отдельно от записей: таблица, прочитанная
					 * без заголовка, вправе получить имена столбцов извне
					 *
					 * @param names имена столбцов в порядке следования
					 * @return      результат установки
					 *
					 * \~english
					 * @brief Method of setting the header of the table
					 * @details The header is given separately from the records: a table read
					 * without a header has the right to receive the names of the columns from the outside
					 * @param names names of the columns in the order of the succession
					 * @return      result of the setting
					 *
					 * \~
					 */
					bool header(const vector <string> & names) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления записи в конец таблицы
					 *
					 * @param fields поля добавляемой записи
					 *
					 * \~english
					 * @brief Method of adding a record to the end of the table
					 * @param fields fields of the record being added
					 *
					 * \~
					 */
					void append(const vector <string> & fields) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления записи в конец таблицы
					 *
					 * @param fields поля добавляемой записи
					 *
					 * \~english
					 * @brief Method of adding a record to the end of the table
					 * @param fields fields of the record being added
					 *
					 * \~
					 */
					void append(const vector <string_view> & fields) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текста таблицы
					 *
					 * @return собранный текст таблицы
					 *
					 * \~english
					 * @brief Method of getting the text of the table
					 * @return assembled text of the table
					 *
					 * \~
					 */
					string text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки таблицы
					 *
					 * \~english
					 * @brief Method of clearing the table
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек контейнера
					 *
					 * @return настройки контейнера
					 *
					 * \~english
					 * @brief Method of getting the settings of the container
					 * @return settings of the container
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек контейнера
					 *
					 * @param settings настройки контейнера
					 *
					 * \~english
					 * @brief Method of setting the settings of the container
					 * @param settings settings of the container
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор вывода таблицы последовательностью знаков
					 *
					 * @return собранный текст таблицы
					 *
					 * \~english
					 * @brief Operator of the output of the table as a sequence of characters
					 * @return assembled text of the table
					 *
					 * \~
					 */
					operator string() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Document(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки контейнера
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the container
					 *
					 * \~
					 */
					Document(const log_t * log, const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept {}
			} document_t;
		}
	}
}

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_CSV_DOCUMENT__
