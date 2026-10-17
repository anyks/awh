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
#pragma once

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
 * Подключаем заголовочный файл проекта
 */
#include "../../sys/fs.hpp"

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
	 * @brief Пространство имён контейнеров данных
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
			 *          и по имени столбца. Годится там, где таблица помещается в память и нужна
			 *          вся сразу; для таблиц, в память не помещающихся, есть потоковое чтение.
			 *
			 * @par Порядок работы
			 *
			 * @note Поля хранятся единым хранилищем знаков, а записи - указаниями в него:
			 *       таблица на миллион полей иначе стоила бы миллиона отдельных строк со всеми
			 *       их заголовками и выделениями памяти
			 *
			 *  @code{.cpp}
			 *          document_t document();
			 *
			 *  document.parse(text);
			 *
			 *  for(size_t i = 0; i < document.rows(); i++)
			 *      const string_view value = document.get(i, "имя");
			 *
			 *  const string result = document.text();
			 *
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the CSV container
			 *
			 * @details Holds the table in full and gives access to the fields by the number of a record
			 *          and by the name of a column. Suitable where the table fits into the memory and is needed
			 *          all at once; for the tables that do not fit into the memory there is the streaming reading.
			 *
			 * @par Order of the work
			 *
			 * @note The fields are stored in a single storage of the characters, while the records — as pointers into it:
			 *       a table of a million fields would otherwise cost a million separate strings with all
			 *       their headers and memory allocations
			 *
			 *  @code{.cpp}
			 *          document_t document();
			 *
			 *  document.parse(text);
			 *
			 *  for(size_t i = 0; i < document.rows(); i++)
			 *      const string_view value = document.get(i, "name");
			 *
			 *  const string result = document.text();
			 *
			 *  @endcode
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				public:
					/**
					 * \~russian
					 * @brief Настройки контейнера
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать
					 *          таблицу с одним разделителем и записать её с другим - обыкновенное
					 *          дело, и связывать их незачем.
					 *
					 * \~english
					 * @brief Settings of the container
					 *
					 * @details The settings of the parsing and of the writing are kept separately: to read
					 *          a table with one separator and to write it with another one is an ordinary
					 *          matter, and there is no point in binding them together.
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
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Settings() noexcept = default;
					} settings_t;
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
					 *
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
					 * Объект для работы с файловой системой
					 *
					 * @note Изменяемость его намеренна: сохранение таблицы объявлено НЕИЗМЕНЯЮЩИМ
					 *       - оно таблицы не трогает, - а подмена целевого файла временным
					 *       константным ходом у файловой системы не является. Состоянием
					 *       таблицы объект этот не служит, и неизменность её он не нарушает
					 *
					 * \~english
					 * Object for working with the file system
					 *
					 * @note Its mutability is deliberate: the saving of the table is declared NON-MUTATING - it does
					 *       not touch the table, - and the replacement of the target file by a temporary one is not a
					 *       constant method of a file system. This object does not serve as a state of the table, and
					 *       it does not violate its immutability
					 *
					 * \~
					 */
					fs_t _fs;
				private:
					/**
					 * \~russian
					 * Кодировка, какою исходный текст таблицы прочитан
					 *
					 * @note Хранится полем оттого, что чтение живёт лишь внутри разбора: спросить
					 *       его после выхода нельзя, а потребителю кодировка нужна и после
					 *
					 * @warning Поле это обязано сбрасываться очисткой таблицы наравне с кодом
					 *          отказа: пережившая очистку кодировка отвечала бы о прежнем тексте
					 *
					 * \~english
					 * Encoding by which the source text of the table has been read
					 *
					 * @note It is kept as a field because the reading lives only inside the parsing: it cannot be
					 *       asked after the exit, while a consumer needs the encoding after it as well
					 *
					 * @warning This field is obliged to be reset by the clearing of the table on a par with the code
					 *          of the refusal: an encoding that has outlived the clearing would answer about a former
					 *          text
					 *
					 * \~
					 */
					encoding_t _encoding = encoding_t::NONE;
					// Настройки контейнера
					settings_t _settings;
				private:
					/**
					 * \~russian
					 * @brief Код отказа последней работы над таблицей
					 *
					 * @details Поле изменчиво нарочно: выдача текста таблицы и запись её в
					 *          файл идут работами неизменными, а сообщать отказ обязаны - иначе пустой
					 *          текст неотличим от таблицы пустой. Прежде код отведён был одному лишь
					 *          разбору, и отказ записи пропадал вовсе.
					 *
					 * \~english
					 * @brief Code of the failure of the last work upon the table
					 *
					 * @details The field is mutable on purpose: the issue of the table text and its
					 *          writing to a file go as immutable methods, yet both are obliged to
					 *          report a failure - otherwise an empty text is indistinguishable from an
					 *          empty table. Previously the code was allotted to the parsing alone, and
					 *          a failure of the writing vanished altogether.
					 *
					 * \~
					 */
					mutable error_t _error;
					/**
					 * \~russian
					 * @brief Положение ошибки в исходном тексте
					 *
					 * @details Поле изменчиво нарочно, наравне с кодом отказа: работы записи
					 *          идут неизменными, а положение сбрасывать обязаны - у отказа записи места
					 *          в исходном тексте нет вовсе, и положение прежнего разбора, его пережив,
					 *          складывалось бы с новым кодом в донесение стройное, но ложное.
					 *
					 * \~english
					 * @brief Position of the error in the source text
					 *
					 * @details The field is mutable on purpose, on par with the failure code: the
					 *          writing methods stay immutable as well, yet they are obliged to reset
					 *          the position - a failure of the writing has no place in the source text
					 *          at all, and the position of a previous parse, outliving it, would join
					 *          the new code into a tidy yet false report.
					 *
					 * \~
					 */
					mutable location_t _location;
				private:
					// Хранилище имён столбцов
					string _names;
					// Хранилище знаков полей таблицы
					string _storage;
				private:
					// Указания на имена столбцов в хранилище имён
					vector <span_t> _header;
					// Указания на поля таблицы в хранилище знаков
					vector <span_t> _fields;
					// Указания на начало каждой записи в перечне полей
					vector <uint32_t> _records;
				private:
					// Соответствие имён столбцов их номерам
					unordered_map <string_view, uint32_t> _columns;
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Код отказа остаётся доступен потребителю через error(): журнал
					 *          его не заменяет, а лишь оповещает о случившемся.
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 *
					 * @details The code of the refusal remains available to the consumer through error():
					 *          the log does not replace it but merely notifies about what has happened.
					 *
					 * \~
					 */
					void report() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого по указанию в хранилище знаков
					 *
					 * @param span указание на содержимое в хранилище знаков
					 * @return     содержимое, на которое указывает указание
					 *
					 * \~english
					 * @brief Method of getting the content by a pointer into the storage of the characters
					 *
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
					 *          хранилище имён при росте перемещается, обесценивая ссылки, по каким
					 *          ведётся поиск.
					 *
					 * \~english
					 * @brief Method of rebuilding the correspondence of the names of the columns to their numbers
					 *
					 * @details The rebuilding is performed after every change of the header:
					 *          the storage of the names is moved at a growth, invalidating the references by which
					 *          the search is conducted.
					 *
					 * \~
					 */
					void reindex() noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса заголовка из чтения в контейнер
					 *
					 * @details Перенос идёт единожды и лишь при заголовке ещё не снятом: зовётся
					 *          он по ходу подачи текста, до выдачи записей обработчику, - тем имена
					 *          столбцов и оказываются доступны ПРЯМО ИЗ ОБРАБОТЧИКА, как и обещано
					 *          договором потокового чтения. Прежде перенос стоял после цикла подачи, и
					 *          обещание это не исполнялось вовсе: обработчик видел заголовок пустым, а
					 *          разрешать поля по имени на потоковом пути было НЕЧЕМ - при том, что путь
					 *          этот заведён ради таблиц, в память не помещающихся, где иного случая
					 *          обратиться к столбцу по имени не представится.
					 *
					 * @note Заслон по непустоте заголовка снимает и повторный перенос, и плату
					 *       за перестроение соответствия: за весь проход они идут по разу
					 *
					 * @param reader чтение, заголовок которого переносится
					 *
					 * \~english
					 * @brief Method of the transfer of the header from the reading into the container
					 *
					 * @details The transfer goes once and only at a header not yet taken: it is called
					 *          along the course of the feeding of the text, before the issuance of the records to the handler —
					 *          thereby the names of the columns turn out to be available RIGHT FROM THE HANDLER, as is promised
					 *          by the contract of the streaming reading. Formerly the transfer stood after the loop of the feeding, and
					 *          that promise was not kept at all: the handler saw the header empty, and
					 *          there was NOTHING to resolve the fields by name with on the streaming path — while that path
					 *          is instituted for the sake of the tables not fitting into the memory, where another occasion
					 *          to address a column by name will not present itself.
					 *
					 * @note The guard by the non-emptiness of the header removes both the repeated transfer, and the cost
					 *       of the rebuilding of the correspondence: over the whole pass they go once
					 *
					 * @param reader reading whose header is being transferred
					 *
					 * \~
					 */
					void adopt(const reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора событий разбора в таблицу
					 *
					 * @details Собирает выданное чтением, не полагаясь на то, подан ли
					 *          текст целиком или кусками: тем одним методом обслуживаются и разбор
					 *          готового текста, и чтение файла по кускам.
					 *
					 * @param reader чтение, выдающее события разбора
					 *
					 * \~english
					 * @brief Method of assembling the parsing events into the table
					 *
					 * @details Assembles what has been issued by the reading without relying on whether
					 *          the text has been fed in full or by chunks: by that single method both the parsing
					 *          of a ready text and the reading of a file by chunks are served.
					 *
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
					 *
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
					 *
					 * @param text     text of the table being parsed
					 * @param settings settings of the container
					 * @return         result of the parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла
					 *
					 * @details Файл читается кусками и разбирается потоково: таблица в
					 *          несколько гигабайт целиком в память не поднимается, в память ложится
					 *          лишь разобранное.
					 *
					 * @param filename адрес файла таблицы для чтения
					 * @return         результат чтения
					 *
					 * \~english
					 * @brief Method of reading a table from a file
					 *
					 * @details The file is read by chunks and parsed in a streaming way: a table of
					 *          several gigabytes is not raised into the memory in full, only what has been parsed
					 *          goes into the memory.
					 *
					 * @param filename address of the file of the table to be read
					 * @return         result of the reading
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла записями
					 *
					 * @details Записи выдаются по одной, а в памяти держится лишь текущая:
					 *          файл любого размера проходит через чтение, не оседая в ней. Таблица
					 *          при этом не заполняется вовсе - тем и отличается настоящий метод от
					 *          чтения, удерживающего таблицу.
					 *
					 * @note Заголовок, если он объявлен настройками, обработчику не
					 *       выдаётся: имена столбцов берутся у header по окончании чтения либо
					 *       прямо из обработчика
					 *
					 * @warning Поля живут лишь на время вызова обработчика: буфер записи
					 *          переиспользуется, и сохранять ссылки на поля дольше вызова нельзя -
					 *          содержимое, нужное дольше, следует копировать
					  *
					 * @note Имя оставлено отличным от `load()` нарочно: `load()` вбирает
					 *       таблицу в контейнер, а чтение это лишь пропускает записи мимо,
					 *       ничего не удерживая. Прочие кодеки зовут вбирающую пару
					 *       `load()` и `save()`, и таблица тому договору отвечает
					 *
					 * @param filename адрес файла таблицы для чтения
					 * @param callback обработчик очередной записи, ложь прекращает чтение
					 * @return         результат чтения
					 *
					 * \~english
					 * @brief Method of reading a table from a file by records
					 *
					 * @details The records are issued one by one, while only the current one is kept in the memory:
					 *          a file of any size passes through the reading without settling in it. The table
					 *          is thereby not filled at all — that is what distinguishes the present method from
					 *          the reading that holds the table.
					 *
					 * @note The header, if it is declared by the settings, is not issued to the
					 *       handler: the names of the columns are taken from header upon the end of the reading or
					 *       right from the handler
					 *
					 * @warning The fields live only for the duration of the call of the handler: the buffer of a record
					 *          is reused, and the references to the fields cannot be preserved longer than the call —
					 *          the content needed for longer should be copied
					 *
					 * @note The name has been kept different from `load()` on purpose: `load()` takes a table into
					 *       the container, while this reading only lets the records pass by, holding nothing. The
					 *       other codecs name the taking pair `load()` and `save()`, and the table answers that
					 *       contract
					 *
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
					 * @details Записи выдаются по одной, а в контейнере не оседают вовсе:
					 *          тем разбор этот и отличается от разбора, таблицу удерживающего.
					 *
					 * @note Заголовок, объявлен он настройками или нет, обработчику не
					 *       выдаётся: имена столбцов берутся из `header()` по окончании
					 *       разбора либо прямо из обработчика
					 *
					 * @note Прекращение разбора обработчиком отказом НЕ считается: итог
					 *       выходит успешным, а канал отказа - пустым. Так же поступает
					 *       и потоковый разбор JSON
					 *
					 * @warning Поля живут лишь на время вызова обработчика: буфер записи
					 *          переиспользуется, и сохранять ссылки на поля дольше вызова нельзя -
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @param text     разбираемый текст таблицы
					 * @param callback обработчик очередной записи, ложь прекращает разбор
					 * @return         результат разбора
					 *
					 * \~english
					 * @brief Method of parsing the text of a table by records
					 *
					 * @details The records are issued one by one, and do not settle in the container at all:
					 *          that is what distinguishes the present parsing from the parsing that holds the table.
					 *
					 * @note The header, whether it is declared by the settings or not, is not issued to the
					 *       handler: the names of the columns are taken from `header()` upon the end of the
					 *       parsing or right from the handler
					 *
					 * @note The termination of the parsing by the handler is NOT considered a refusal: the result
					 *       comes out successful, and the channel of the refusal — empty. The streaming parsing of JSON
					 *       acts the same way
					 *
					 * @warning The fields live only for the duration of the call of the handler: the buffer of a record
					 *          is reused, and the references to the fields cannot be preserved longer than the call —
					 *          the content needed for longer should be copied
					 *
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
					 * @see Договор этот закреплён проверкою `CodecContract.SavingKeepsThePreviousFileOnWriteFailure`:
					 *      неделимость записи: отказ оставляет прежний файл нетронутым
					 *
					 * @param filename адрес файла таблицы для записи
					 * @return         результат записи
					 *
					 * \~english
					 * @brief Method of writing a table into a file
					 *
					 * @see This contract is enforced by the `CodecContract.SavingKeepsThePreviousFileOnWriteFailure` check:
					 *      the write operation is atomic—a failure leaves the original file untouched.
					 *
					 * @param filename address of the file of the table to be written
					 * @return         result of the writing
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода отказа последней работы над таблицей
					 *
					 * @details Разбор текста, чтение файла, выдача текста и запись его в файл
					 *          ставят код этот заново каждая. Прежде отведён он был одному лишь
					 *          разбору, и отказ записи пропадал вовсе: замер дал таблицу, поле которой
					 *          настройками укрыть нечем, - `text()` отвечал пустой строкой, `save()`
					 *          ложью, а `error()` при том молчал.
					 *
					 * @note Договор этот ОБЩИЙ у кодеков: владеющие значения разметки XML и
					 *       документа JSON отвечают тем же кодом и тем же порядком
					 *
					 * @note Правка таблицы (`append`, `header`) код этот СБРАСЫВАЕТ, своего не
					 *       ставя. Замер 17.09.2026: разбор незакрытой обвязки отказывал кодом, а
					 *       первая же дописка записи сменяла его на «нет ошибки». Правка деревьев
					 *       XML и JSON ведёт себя так же
					 *
					 * @return код отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the code of the refusal of the last operation over the table
					 *
					 * @details The parsing of a text, the reading of a file, the issuance of a text and the writing
					 *          of it into a file each set this code anew. Formerly it was allotted to the parsing alone,
					 *          and the refusal of the writing was lost entirely: a measurement gave a table whose field
					 *          had nothing to cover it with by the settings, - `text()` answered with an empty string,
					 *          `save()` with a lie, while `error()` kept silent upon it.
					 *
					 * @note This contract is COMMON among the codecs: the owning values of an XML markup
					 *       and of a JSON document answer with the same code and in the same order
					 *
					 * @note An editing of the table (`append`, `header`) RESETS this code, setting none of its
					 *       own. A measurement of 2026-09-17: the parsing of an unclosed quoting refused with
					 *       a code, and the very first appending of a record changed it to «no error». An editing of
					 *       the XML and JSON trees behaves in the same way
					 *
					 * @return code of the refusal of the last operation
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения ошибки разбора
					 *
					 * @note Имя названо явным нарочно: у чтения `location()` означает место
					 *       ТЕКУЩЕГО события, а не отказа, и одно имя о двух значениях
					 *       заводило потребителя в западню при переходе с уровня на уровень.
					 *       Кодеки XML, TOML и INI зовут его так же
					 *
					 * @return положение ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of a parsing error
					 *
					 * @note The name has been spelled out on purpose: at the reading `location()` means the place of
					 *       the CURRENT event, and not of a refusal, and one name about two senses would lead a
					 *       consumer into a trap upon a passage from one level onto another. The XML, TOML and INI
					 *       codecs name it in the same way
					 *
					 * @return position of the error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кодировки исходного текста таблицы
					 *
					 * @details Выдаётся кодировка, какою текст ПРОЧИТАН - та, из которой шёл
					 *          перевод в UTF-8, распознанная по метке порядка байтов либо навязанная
					 *          настройками чтения. Кодека, объявляющего кодировку внутри себя, у таблицы
					 *          нет, оттого иного ответа тут не бывает.
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки: потребитель, читающий
					 *       несколько кодеков, спрашивает кодировку одинаково
					 *
					 * @warning До первого разбора выдаётся кодировка неопределённая
					 *
					 * @see Договор этот закреплён проверкою `CodecContract.ByteOrderMarkIsRecognisedAndKeptOutOfTheContent`:
					 *      распознавание метки порядка байтов да невхождение её в содержимое
					 *
					 * @return кодировка исходного текста таблицы
					 *
					 * \~english
					 * @brief Method of the extraction of the encoding of the source text of the table
					 *
					 * @details The encoding is issued by which the text was READ - the one from which the translation
					 *          into UTF-8 came, recognised by the byte order mark or imposed by the settings of the
					 *          reading. A codec declaring an encoding inside itself does not exist among the tables,
					 *          and therefore there is no other answer here.
					 *
					 * @note This method is introduced as common to all the codecs of the framework: a consumer
					 *       reading several codecs asks the encoding in the same way
					 *
					 * @warning Before the first parsing an undefined encoding is issued
					 *
					 * @see This agreement is enforced by the `CodecContract.ByteOrderMarkIsRecognisedAndKeptOutOfTheContent` check:
					 *      the byte order mark is recognized and excluded from the content
					 *
					 * @return encoding of the source text of the table
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества записей таблицы
					 *
					 * @details Заголовок записью не считается: количество это отвечает
					 *          количеству записей со значениями.
					 *
					 * @return количество записей таблицы
					 *
					 * \~english
					 * @brief Method of getting the number of the records of the table
					 *
					 * @details The header is not counted as a record: this number corresponds to
					 *          the number of the records with the values.
					 *
					 * @return number of the records of the table
					 *
					 * \~
					 */
					size_t rows() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества записей таблицы
					 *
					 * @details Ход этот - посредник к `rows()`, заведённый ради общего облика
					 *          кодеков: у документа JSON да дерева разметки `size()` выдаёт ДЕТЕЙ КОРНЯ, и
					 *          у таблицы детьми этими служат записи её. Без него потребитель, пишущий один
					 *          обход на три кодека, у таблицы спотыкался бы на имени.
					 *
					 * @details Заголовок записью не считается, ровно как и у `rows()`.
					 *
					 * @note Хода `nodes()` у таблицы нет вовсе и быть не может: арены узлов она не
					 *       держит, а поля лежат нарезкой общего хранилища знаков. Заводить его
					 *       посредником к чему бы то ни было значило бы выдумать величину
					 *
					 * @see Договор этот закреплён проверкою `CodecContract.ChildCountIsSeparatedFromStorageCount`:
					 *      счёт записей таблицы наравне с детьми корня у деревьев
					 *
					 * @return количество записей таблицы
					 *
					 * \~english
					 * @brief Method of getting the number of the records of the table
					 *
					 * @details This call is a mediator to `rows()`, introduced for the sake of the common
					 *          appearance of the codecs: for a JSON document and a markup tree `size()` gives THE
					 *          CHILDREN OF THE ROOT, and for a table those children are its records. Without it a
					 *          consumer writing one walk for three codecs would stumble at the name of a table.
					 *
					 * @details A header is not counted as a record, exactly as at `rows()`.
					 *
					 * @note There is no call `nodes()` for a table and there cannot be: it holds no arena of the
					 *       nodes, while its fields lie as a cutting of the common storage of the characters. To
					 *       introduce it as a mediator to anything whatever would mean inventing a quantity
					 *
					 * @see This contract is enforced by the `CodecContract.ChildCountIsSeparatedFromStorageCount` check:
					 *      the count of table entries is treated on par with the count of a root node's children in trees
					 *
					 * @return number of the records of the table
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки таблицы на пустоту
					 *
					 * @details Пустой считается таблица без записей И без заголовка: таблица,
					 *          несущая один лишь заголовок, записей не имеет, а содержимое имеет, и
					 *          записывается она непустым текстом.
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки: потребитель, читающий
					 *       несколько кодеков, спрашивает пустоту одинаково
					 *
					 * @warning Одного размера у таблицы нет вовсе - есть `rows()`, `cols()` и
					 *          `size(row)`, - и общего хода `size()` у неё оттого не заведено.
					 *          Пустота же вопрос однозначный, и она общею сделана
					 *
					 * @return признак отсутствия содержимого в таблице
					 *
					 * \~english
					 * @brief Method of checking the table for emptiness
					 *
					 * @details A table without records AND without a header is considered empty: a table carrying
					 *          one only header has no records, yet it has a content, and it is written out as a text
					 *          that is not empty.
					 *
					 * @note This method is introduced as common to all the codecs of the framework: a consumer
					 *       reading several codecs asks the emptiness in the same way
					 *
					 * @warning A table has no single size at all - there are `rows()`, `cols()` and `size(row)`, -
					 *          and therefore no common method `size()` has been introduced for it. The emptiness,
					 *          though, is a question with one answer, and it has been made a common one
					 *
					 * @return flag of the absence of the content in the table
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества столбцов таблицы
					 *
					 * @details Количеством столбцов считается наибольшее количество полей
					 *          среди записей: записи с разным числом полей договор дозволяет, и
					 *          брать здесь количество полей первой записи значило бы терять поля.
					 *
					 * @return количество столбцов таблицы
					 *
					 * \~english
					 * @brief Method of getting the number of the columns of the table
					 *
					 * @details The number of the columns is the largest number of the fields
					 *          among the records: the protocol permits the records with a differing number of fields, and
					 *          to take the number of the fields of the first record here would mean to lose the fields.
					 *
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
					 *
					 * @param row number of the record, counting from zero
					 * @return    number of the fields of the record
					 *
					 * \~
					 */
					size_t size(const size_t row) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения имён столбцов
					 *
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @return имена столбцов в порядке объявления
					 *
					 * \~english
					 * @brief Method of getting the names of the columns
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
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
					 *
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
					 *
					 * @param name name of the column being sought
					 * @return     number of the column or the sign of the absence
					 *
					 * \~
					 */
					uint32_t column(const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого поля по номеру записи и столбца
					 *
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @warning Поле ПУСТОЕ от поля ОТСУТСТВУЮЩЕГО выдача не отличает: и то и
					 *          другое отвечает пустотою. Различаются они ходом `size(row)`, дающим число
					 *          полей записи, и иного пути у потребителя нет. Двусмысленность эта у
					 *          таблицы законна - пустое поле есть законное значение стандарта, а не
					 *          отсутствие его, - но полагаться на пустоту как на признак отсутствия
					 *          нельзя, и договор этот закреплён проверкой
					 *
					 * @param row номер записи, считая с нуля
					 * @param col номер столбца, считая с нуля
					 * @return    содержимое поля, пустое при его отсутствии
					 *
					 * \~english
					 * @brief Method of getting the content of a field by the number of the record and of the column
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
					 * @warning An EMPTY field is not distinguished from an ABSENT one: both answer with emptiness.
					 *          They are distinguished by the method `size(row)` giving the number of the fields of the
					 *          record, and a consumer has no other way. This ambiguity is lawful among a table - an
					 *          empty field is a lawful value of the standard, and not its absence, - but relying upon
					 *          an emptiness as upon a sign of an absence is impossible, and this contract is fixed by
					 *          a check
					 *
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
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @param row  номер записи, считая с нуля
					 * @param name имя столбца
					 * @return     содержимое поля, пустое при его отсутствии
					 *
					 * \~english
					 * @brief Method of getting the content of a field by the number of the record and the name of the column
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
					 * @param row  number of the record, counting from zero
					 * @param name name of the column
					 * @return     content of the field, empty in its absence
					 *
					 * \~
					 */
					string_view get(const size_t row, const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения записи целиком
					 *
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @param row номер записи, считая с нуля
					 * @return    поля записи в порядке следования
					 *
					 * \~english
					 * @brief Method of getting a record in full
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
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
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @param col номер столбца, считая с нуля
					 * @return    поля столбца в порядке следования записей
					 *
					 * \~english
					 * @brief Method of getting a column in full
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
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
					 * @warning Выданное живёт лишь до ближайшего изменения таблицы: виды
					 *          указывают в общее хранилище знаков, а оно при росте ПЕРЕЕЗЖАЕТ. Беда эта
					 *          молчалива - вид не падает, он начинает читать чужое содержимое, - и
					 *          содержимое, нужное дольше, следует копировать
					 *
					 * @param name имя столбца
					 * @return     поля столбца в порядке следования записей
					 *
					 * \~english
					 * @brief Method of getting a column in full by its name
					 *
					 * @warning What is issued lives only until the nearest change of the table: the views
					 *          point into the common storage of the characters, and it MOVES at a growth. That trouble is
					 *          silent — the view does not fall, it begins to read a foreign content — and
					 *          the content needed for longer should be copied
					 *
					 * @param name name of the column
					 * @return     fields of the column in the order of the succession of the records
					 *
					 * \~
					 */
					vector <string_view> col(const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода приведения содержимого поля к числу либо логическому значению
					 *
					 * @tparam T тип получаемого значения
					 *
					 * \~english
					 * @brief A method template for converting field content into a number or a boolean value
					 *
					 * @tparam T type of the value being obtained
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод приведения содержимого поля к числу либо логическому значению
					 *
					 * @note Имя хода и порядок доводов приведены к общему виду 17.09.2026
					 *       принятым решением. Прежде ход этот звался `numeric` и принимал
					 *       приёмник ПОСЛЕДНИМ - `numeric(row, col, result)`, - тогда как
					 *       шесть прочих кодеков рамки зовут его `value` и принимают приёмник
					 *       ПЕРВЫМ: `value(result, key, section)` у INI, `value(result, path)`
					 *       у TOML, `value(result, local, uri)` у разметки XML. Общий вопрос
					 *       обязан носить общее имя, а имя `numeric` отведено низовому разбору
					 *       записи в число (`awh::codec::numeric`) - публичному ходу документа
					 *       оно не принадлежит вовсе
					 *
					 * @note Число, в затребованный вид не помещающееся, отвергается, а не
					 *       усекается: «300» к `uint8_t` и «1e308» к `float` отвечают отказом
					 *       равно. Кодек JSON здесь ведёт себя иначе - там приведение
					 *       упирается в предел вида, - и расхождение это намеренно: там
					 *       выдаётся само значение, отказать которым нечем, а здесь выдаётся
					 *       признак успеха, и он обязан быть правдив
					 *
					 * @note Расхождение это СЛИЧЕНО замером и пересличено 31.08.2026. У JSON
					 *       приведение отвечает ЯЗЫКУ везде, где у языка ответ определён:
					 *       непомещающееся целое заворачивается по кругу - `300` видом
					 *       `uint8_t` даёт 44, `-1` видом `uint64_t` даёт
					 *       18446744073709551615, - а предел подставляется лишь там, где
					 *       поведение языка неопределено, то есть у настоящей дроби: `300.5`
					 *       видом `uint8_t` даёт 255. CSV в тех же местах отвечает отказом
					 *
					 * @note Написание числа на извлечение НЕ влияет ни у одного кодека рамки:
					 *       «300» и «300.0» суть одно число и извлекаются одинаково - у CSV
					 *       оба дают 300 видом `uint64_t` и оба отвергаются видом `uint8_t`,
					 *       у JSON оба дают 300 и 44 соответственно. Решение это принято
					 *       от 30.08.2026, и прежде ему отвечал один лишь JSON: у CSV
					 *       написание «300.0» видом целым отвергалось, хотя число в вид
					 *       помещалось, - отказ тот правилу кодека не отвечал вовсе
					 *
					 * @warning Записи «inf» и «nan» переносятся в дробный вид как есть: они
					 *          пределом не ограничены, и терять им нечего
					 *
					 * @note Переменная-приёмник при ОТКАЗЕ не трогается вовсе и сохраняет то,
					 *       что несла до вызова. Уклад этот един у всех трёх кодеков и снят
					 *       замером 01.09.2026 по двадцати путям отказа: негодная запись,
					 *       непомещение в вид, утрата разрядов выше двух в пятьдесят третьей,
					 *       исчезновение числа в нуль, вместилище вместо числа. Полагаться на
					 *       приёмник, не проверив признак успеха, всё равно нельзя - но
					 *       читающий его увидит СВОЁ прежнее значение, а не полуснятое
					 *
					 * @param result полученное значение
					 * @param row    номер записи, считая с нуля
					 * @param col    номер столбца, считая с нуля
					 * @return       результат приведения
					 *
					 * \~english
					 * @brief Method of converting the content of a field to a number or to a logical value
					 *
					 * @note The name of the method and the order of the arguments were brought to the common
					 *       shape on 2026-09-17 by an adopted decision. Formerly this method was called `numeric` and
					 *       took the receiver LAST - `numeric(row, col, result)`, - whereas the six other codecs of
					 *       the framework call it `value` and take the receiver FIRST: `value(result, key, section)`
					 *       at INI, `value(result, path)` at TOML, `value(result, local, uri)` at the XML markup. A
					 *       common question is obliged to carry a common name, and the name `numeric` is allotted to
					 *       the low-level conversion of a writing into a number (`awh::codec::numeric`) - to a public
					 *       method of a document it does not belong at all
					 *
					 * @note A number that does not fit into the kind asked is refused, and not truncated: «300» to
					 *       `uint8_t` and «1e308» to `float` answer with a refusal alike. The JSON codec behaves
					 *       otherwise here - there the conversion is capped by the bound of the kind, - and this
					 *       divergence is deliberate: there the value itself is issued, by which there is nothing to
					 *       refuse, while here a sign of success is issued, and it is obliged to be truthful
					 *
					 * @note This divergence is COMPARED by a measurement and compared anew on 2026-08-31. At JSON the
					 *       conversion answers the LANGUAGE everywhere the answer of the language is defined: an
					 *       integer that does not fit is wrapped around - `300` by the kind `uint8_t` gives 44, `-1`
					 *       by the kind `uint64_t` gives 18446744073709551615, - while a bound is put in only where
					 *       the behaviour of the language is undefined, that is, at a real fraction: `300.5` by the
					 *       kind `uint8_t` gives 255. CSV answers with a refusal at those same places
					 *
					 * @note The spelling of a number does NOT affect the extraction among any codec of the framework:
					 *       «300» and «300.0» are one and the same number and are extracted alike - at CSV both give
					 *       300 by the kind `uint64_t` and both are refused by the kind `uint8_t`, at JSON both give
					 *       300 and 44 respectively. The decision was taken on 2026-08-30, and formerly only JSON
					 *       answered it: at CSV the writing «300.0» by an integer kind was refused although the
					 *       number fitted into the kind, - that refusal answered no rule of the codec at all
					 *
					 * @warning The writings «inf» and «nan» are carried into a fractional kind as they are: they are
					 *          not bounded by a limit, and they have nothing to lose
					 *
					 * @note The receiver variable at a REFUSAL is not touched at all and keeps what it carried before
					 *       the call. The order is one among all the three codecs and was taken by a measurement of
					 *       2026-09-01 over twenty ways of a refusal: an unfit writing, a number not fitting into the
					 *       kind, a loss of the digits above two at the fifty-third, a vanishing of a number into
					 *       zero, a container in place of a number. Relying upon the receiver without checking the
					 *       sign of success is still impossible - but the one reading it will see HIS OWN former
					 *       value, and not a half-taken one
					 *
					 * @param result obtained value
					 * @param row    number of the record, counting from zero
					 * @param col    number of the column, counting from zero
					 * @return       result of the conversion
					 *
					 * \~
					 */
					bool value(T & result, const size_t row, const size_t col) const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода приведения содержимого поля к числу либо логическому значению по имени столбца
					 *
					 * @tparam T тип получаемого значения
					 *
					 * \~english
					 * @brief A method template for converting field content to a number or a boolean value, based on the column name
					 *
					 * @tparam T type of the value being obtained
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод приведения содержимого поля к числу либо логическому значению по имени столбца
					 *
					 * @details Ход этот парен ходу `get(row, name)`: имя столбца отыскивается
					 *          в заглавии таблицы, а найденный номер передаётся ходу по номеру. Всё, что
					 *          сказано о приведении у хода по номеру, верно и здесь.
					 *
					 * @note Столбец, имени какого таблица не несёт, отвечает ОТКАЗОМ, а не
					 *       пустым значением: `get` в том же положении выдаёт пустое
					 *       содержимое, ибо выдаёт вид, отказать которым нечем, а здесь
					 *       выдаётся признак успеха, и он обязан быть правдив. Тем же
					 *       правилом живёт и приведение поля, содержимое какого числом не
					 *       является
					 *
					 * @warning Заглавие у таблицы берётся лишь тогда, когда чтение заказано с
					 *          ним: таблица, прочтённая без заглавия, имён столбцов не знает
					 *          вовсе, и ход этот ответит отказом на всякое имя
					 *
					 * @param result полученное значение
					 * @param row    номер записи, считая с нуля
					 * @param name   имя столбца
					 * @return       результат приведения
					 *
					 * \~english
					 * @brief Method of converting the content of a field to a number or to a logical value by the name of the column
					 *
					 * @details This method is paired with `get(row, name)`: the name of a column is looked up in the
					 *          header of the table, and the number found is passed to the method by a number.
					 *          Everything said about the conversion at the method by a number holds here as well.
					 *
					 * @note A column whose name the table does not carry answers with a REFUSAL rather than
					 *       with an empty value: `get` in the same position issues an empty content, for it issues
					 *       a view, by which there is nothing to refuse, whereas here a sign of success is issued,
					 *       and it is obliged to be truthful. The conversion of a field whose content is not a number
					 *       lives by that same rule
					 *
					 * @warning The header is taken from a table only when the reading has been ordered with it: a
					 *          table read without a header knows no names of the columns at all, and this method will
					 *          answer with a refusal to any name
					 *
					 * @param result obtained value
					 * @param row    number of the record, counting from zero
					 * @param name   name of the column
					 * @return       result of the conversion
					 *
					 * \~
					 */
					bool value(T & result, const size_t row, const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки заголовка таблицы
					 *
					 * @details Заголовок задаётся отдельно от записей: таблица, прочитанная
					 *          без заголовка, вправе получить имена столбцов извне.
					 *
					 * @note Отказ называет причину кодом, и причин у него три: пустое имя
					 *       столбца - `EMPTY_HEADER`, повтор имени - `DUPLICATE_HEADER`,
					 *       переполнение хранилища имён - `STORAGE_EXHAUSTED`. Прежде код
					 *       ставила лишь последняя, а первые две отвергали МОЛЧА: правило
					 *       одно с разбором заголовка из текста, а причину называл лишь он
					 *
					 * @note Пустой перечень имён отказом НЕ является: он снимает заголовок,
					 *       и таблица остаётся при одних записях
					 *
					 * @param names имена столбцов в порядке следования
					 * @return      результат установки
					 *
					 * \~english
					 * @brief Method of setting the header of the table
					 *
					 * @details The header is given separately from the records: a table read
					 *          without a header has the right to receive the names of the columns from the outside.
					 *
					 * @note The refusal names its cause by a code, and it has three causes: an empty name
					 *       of a column — `EMPTY_HEADER`, a repetition of a name — `DUPLICATE_HEADER`,
					 *       an overflow of the storage of the names — `STORAGE_EXHAUSTED`. Formerly only the last
					 *       one put the code, while the first two refused IN SILENCE: the rule is one with the
					 *       parsing of a header from a text, and only it named a cause
					 *
					 * @note An empty list of the names is NOT a refusal: it removes the header, and the table is
					 *       left with its records only
					 *
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
					 * @note Содержимое, записи не подлежащее, отвергается ЗДЕСЬ отказом
					 *       `UNWRITABLE_FIELD`, а не при записи текста: прежде добавление
					 *       принимало его молча, и таблица оставалась непригодной к записи.
					 *       Разделитель, кавычка и перевод строки внутри поля отказом НЕ
					 *       являются - запись ограждает их кавычками
					 *
					 * @param fields поля добавляемой записи
					 * @return       результат добавления записи
					 *
					 * \~english
					 * @brief Method of adding a record to the end of the table
					 *
					 * @note Content that is not amenable to the writing is refused HERE by the refusal
					 *       `UNWRITABLE_FIELD`, and not upon the writing of a text: formerly the adding took it in
					 *       silence, and the table stayed unfit for the writing. A separator, a quote and a line feed
					 *       inside a field are NOT a refusal - the writing fences them off with quotes
					 *
					 * @param fields fields of the record being added
					 * @return       result of the adding of the record
					 *
					 * \~
					 */
					bool append(const vector <string> & fields) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления записи в конец таблицы
					 *
					 * @note Содержимое, записи не подлежащее, отвергается ЗДЕСЬ отказом
					 *       `UNWRITABLE_FIELD`, а не при записи текста: прежде добавление
					 *       принимало его молча, и таблица оставалась непригодной к записи.
					 *       Разделитель, кавычка и перевод строки внутри поля отказом НЕ
					 *       являются - запись ограждает их кавычками
					 *
					 * @param fields поля добавляемой записи
					 * @return       результат добавления записи
					 *
					 * \~english
					 * @brief Method of adding a record to the end of the table
					 *
					 * @note Content that is not amenable to the writing is refused HERE by the refusal
					 *       `UNWRITABLE_FIELD`, and not upon the writing of a text: formerly the adding took it in
					 *       silence, and the table stayed unfit for the writing. A separator, a quote and a line feed
					 *       inside a field are NOT a refusal - the writing fences them off with quotes
					 *
					 * @param fields fields of the record being added
					 * @return       result of the adding of the record
					 *
					 * \~
					 */
					bool append(const vector <string_view> & fields) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста таблицы
					 *
					 * @details Настройки записи берутся хранимые; иные ставятся ходом
					 *          `settings()` прежде сборки.
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки: потребитель, пишущий
					 *       обобщённо, зовёт `dump()` без довода, не зная кодека вовсе
					 *
					 * @warning При отказе сборки выдаётся ПУСТОЙ текст, а код отказа спрашивается
					 *          ходом `error()`. Пустая таблица от отказа отличается только им
					 *
					 * @return собранный текст таблицы
					 *
					 * \~english
					 * @brief Method of the assembling of the text of the table
					 *
					 * @details The settings of the writing are taken as stored; the other ones are set by the method
					 *          `settings()` before the assembling.
					 *
					 * @note This method is introduced as common to all the codecs of the framework: a consumer
					 *       writing in a generalised way calls `dump()` without an argument, knowing no codec at all
					 *
					 * @warning Upon a failure of the assembling an EMPTY text is issued, and the code of the refusal
					 *          is asked by the method `error()`. An empty table differs from a refusal by that only
					 *
					 * @return assembled text of the table
					 *
					 * \~
					 */
					string dump() const noexcept;
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
					/**
					 * \~russian
					 * @brief Псевдоним сброса состояния документа (см. clear())
					 *
					 * \~english
					 * @brief Alias of the reset of the state of a document (see clear())
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения настроек контейнера
					 *
					 * @return настройки контейнера
					 *
					 * \~english
					 * @brief Method of getting the settings of the container
					 *
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
					 *
					 * @param settings settings of the container
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Оператор вывода таблицы последовательностью знаков
					 *
					 * @return собранный текст таблицы
					 *
					 * \~english
					 * @brief Operator of the output of the table as a sequence of characters
					 *
					 * @return assembled text of the table
					 *
					 * \~
					 */
					operator string() const noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Document() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки контейнера
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param settings settings of the container
					 *
					 * \~
					 */
					explicit Document(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Document() noexcept = default;
			} document_t;
		};
	};
};
