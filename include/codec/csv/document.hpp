/**
 * @file: document.hpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл контейнера CSV — класс Document, удерживающий таблицу целиком
 *        и дающий доступ к полям по номеру записи и по имени столбца
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён контейнеров данных
	 *
	 */
	namespace codec {
		/**
		 * @brief Пространство имён контейнера CSV
		 *
		 */
		namespace csv {
			/**
			 * @brief Класс контейнера CSV
			 *
			 * @details Удерживает таблицу целиком и даёт доступ к полям по номеру записи
			 * и по имени столбца. Годится там, где таблица помещается в память и нужна
			 * вся сразу; для таблиц, в память не помещающихся, есть потоковое чтение
			 *
			 * @par Порядок работы
			 *
			 * @code{.cpp}
			 * document_t document;
			 *
			 * document.parse(text);
			 *
			 * for(size_t i = 0; i < document.rows(); i++)
			 *     const string_view value = document.get(i, "имя");
			 *
			 * const string result = document.text();
			 * @endcode
			 *
			 * @note Поля хранятся единым хранилищем знаков, а записи - указаниями в него:
			 * таблица на миллион полей иначе стоила бы миллиона отдельных строк со всеми
			 * их заголовками и выделениями памяти
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				public:
					/**
					 * @brief Настройки контейнера
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать
					 * таблицу с одним разделителем и записать её с другим - обыкновенное
					 * дело, и связывать их незачем
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Настройки разбора текста
						reader_t::settings_t reader;
						// Настройки записи текста
						writer_t::settings_t writer;
						/**
						 * @brief Конструктор
						 *
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
					 * Признак того, что запись начата и ещё не завершена
					 *
					 * @note Признак держится членом, а не выводится из накопленного:
					 *       события приходят кусками, и между двумя вызовами сбора
					 *       запись вправе остаться незавершённой
					 */
					bool _opened;
				private:
					/**
					 * @brief Метод получения содержимого по указанию в хранилище знаков
					 *
					 * @param span указание на содержимое в хранилище знаков
					 * @return     содержимое, на которое указывает указание
					 *
					 */
					string_view get(const span_t & span) const noexcept;
					/**
					 * @brief Метод перестроения соответствия имён столбцов их номерам
					 *
					 * @details Перестроение выполняется после всякого изменения заголовка:
					 * хранилище имён при росте перемещается, обесценивая ссылки, по каким
					 * ведётся поиск
					 *
					 */
					void reindex() noexcept;
					/**
					 * @brief Метод сбора событий разбора в таблицу
					 *
					 * @details Собирает выданное чтением, не полагаясь на то, подан ли
					 * текст целиком или кусками: тем одним методом обслуживаются и разбор
					 * готового текста, и чтение файла по кускам
					 *
					 * @param reader чтение, выдающее события разбора
					 *
					 */
					void consume(reader_t & reader) noexcept;
				public:
					/**
					 * @brief Метод разбора текста таблицы
					 *
					 * @param text разбираемый текст таблицы
					 * @return     результат разбора
					 *
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * @brief Метод разбора текста таблицы с заданными настройками
					 *
					 * @param text     разбираемый текст таблицы
					 * @param settings настройки контейнера
					 * @return         результат разбора
					 *
					 */
					bool parse(const string_view text, const settings_t & settings) noexcept;
				public:
					/**
					 * @brief Метод чтения таблицы из файла
					 *
					 * @details Файл читается кусками и разбирается потоково: таблица в
					 * несколько гигабайт целиком в память не поднимается, в память ложится
					 * лишь разобранное
					 *
					 * @param filename адрес файла таблицы для чтения
					 * @return         результат чтения
					 *
					 */
					bool read(const string & filename) noexcept;
					/**
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
					 */
					bool read(const string & filename, const function <bool (const vector <string_view> &)> & callback) noexcept;
					/**
					 * @brief Метод разбора текста таблицы записями
					 *
					 * @param text     разбираемый текст таблицы
					 * @param callback обработчик очередной записи, ложь прекращает разбор
					 * @return         результат разбора
					 *
					 */
					bool parse(const string_view text, const function <bool (const vector <string_view> &)> & callback) noexcept;
					/**
					 * @brief Метод записи таблицы в файл
					 *
					 * @param filename адрес файла таблицы для записи
					 * @return         результат записи
					 *
					 */
					bool write(const string & filename) const noexcept;
				public:
					/**
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки разбора
					 *
					 */
					error_t error() const noexcept;
					/**
					 * @brief Метод получения положения ошибки разбора
					 *
					 * @return положение ошибки в исходном тексте
					 *
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * @brief Метод получения количества записей таблицы
					 *
					 * @details Заголовок записью не считается: количество это отвечает
					 * количеству записей со значениями
					 *
					 * @return количество записей таблицы
					 *
					 */
					size_t rows() const noexcept;
					/**
					 * @brief Метод получения количества столбцов таблицы
					 *
					 * @details Количеством столбцов считается наибольшее количество полей
					 * среди записей: записи с разным числом полей договор дозволяет, и
					 * брать здесь количество полей первой записи значило бы терять поля
					 *
					 * @return количество столбцов таблицы
					 *
					 */
					size_t cols() const noexcept;
					/**
					 * @brief Метод получения количества полей записи
					 *
					 * @param row номер записи, считая с нуля
					 * @return    количество полей записи
					 *
					 */
					size_t size(const size_t row) const noexcept;
				public:
					/**
					 * @brief Метод получения имён столбцов
					 *
					 * @return имена столбцов в порядке объявления
					 *
					 */
					vector <string_view> header() const noexcept;
					/**
					 * @brief Метод проверки наличия столбца с заданным именем
					 *
					 * @param name имя проверяемого столбца
					 * @return     результат проверки
					 *
					 */
					bool has(const string_view name) const noexcept;
					/**
					 * @brief Метод получения номера столбца по его имени
					 *
					 * @param name имя искомого столбца
					 * @return     номер столбца либо признак отсутствия
					 *
					 */
					uint32_t column(const string_view name) const noexcept;
				public:
					/**
					 * @brief Метод получения содержимого поля по номеру записи и столбца
					 *
					 * @param row номер записи, считая с нуля
					 * @param col номер столбца, считая с нуля
					 * @return    содержимое поля, пустое при его отсутствии
					 *
					 */
					string_view get(const size_t row, const size_t col) const noexcept;
					/**
					 * @brief Метод получения содержимого поля по номеру записи и имени столбца
					 *
					 * @param row  номер записи, считая с нуля
					 * @param name имя столбца
					 * @return     содержимое поля, пустое при его отсутствии
					 *
					 */
					string_view get(const size_t row, const string_view name) const noexcept;
				public:
					/**
					 * @brief Метод получения записи целиком
					 *
					 * @param row номер записи, считая с нуля
					 * @return    поля записи в порядке следования
					 *
					 */
					vector <string_view> row(const size_t row) const noexcept;
					/**
					 * @brief Метод получения столбца целиком
					 *
					 * @param col номер столбца, считая с нуля
					 * @return    поля столбца в порядке следования записей
					 *
					 */
					vector <string_view> col(const size_t col) const noexcept;
					/**
					 * @brief Метод получения столбца целиком по его имени
					 *
					 * @param name имя столбца
					 * @return     поля столбца в порядке следования записей
					 *
					 */
					vector <string_view> col(const string_view name) const noexcept;
				public:
					/**
					 * @brief Метод приведения содержимого поля к числу либо логическому значению
					 *
					 * @tparam T тип получаемого значения
					 * @param row номер записи, считая с нуля
					 * @param col номер столбца, считая с нуля
					 * @param result полученное значение
					 * @return       результат приведения
					 *
					 */
					template <typename T>
					bool numeric(const size_t row, const size_t col, T & result) const noexcept;
				public:
					/**
					 * @brief Метод установки заголовка таблицы
					 *
					 * @details Заголовок задаётся отдельно от записей: таблица, прочитанная
					 * без заголовка, вправе получить имена столбцов извне
					 *
					 * @param names имена столбцов в порядке следования
					 * @return      результат установки
					 *
					 */
					bool header(const vector <string> & names) noexcept;
					/**
					 * @brief Метод добавления записи в конец таблицы
					 *
					 * @param fields поля добавляемой записи
					 *
					 */
					void append(const vector <string> & fields) noexcept;
					/**
					 * @brief Метод добавления записи в конец таблицы
					 *
					 * @param fields поля добавляемой записи
					 *
					 */
					void append(const vector <string_view> & fields) noexcept;
				public:
					/**
					 * @brief Метод получения текста таблицы
					 *
					 * @return собранный текст таблицы
					 *
					 */
					string text() const noexcept;
					/**
					 * @brief Метод очистки таблицы
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Метод получения настроек контейнера
					 *
					 * @return настройки контейнера
					 *
					 */
					const settings_t & settings() const noexcept;
					/**
					 * @brief Метод установки настроек контейнера
					 *
					 * @param settings настройки контейнера
					 *
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * @brief Оператор вывода таблицы последовательностью знаков
					 *
					 * @return собранный текст таблицы
					 *
					 */
					operator string() const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Document() noexcept;
					/**
					 * @brief Конструктор
					 *
					 * @param settings настройки контейнера
					 *
					 */
					Document(const settings_t & settings) noexcept;
					/**
					 * @brief Деструктор
					 *
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
