/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения текста CSV — класс Reader, принимающий текст
 *        кусками произвольного размера и выдающий события разбора
 *
 * \~english
 * @brief Header file of the streaming reading of a CSV text — the Reader class, which accepts the text
 *        by chunks of an arbitrary size and issues the parsing events
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV_READER__
#define __AWH_CODEC_CSV_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

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
			 * @brief Класс потокового чтения текста CSV
			 *
			 * @details Текст принимается кусками произвольного размера, а выдача ведётся
			 * событиями по мере разбора: удерживать текст целиком чтению не требуется
			 *
			 * @par Порядок работы
			 *
			 * @warning Выдача разбора не зависит от того, как исходный текст нарезан на
			 * куски: одна и та же последовательность событий с одними и теми же местами
			 * получается при всякой нарезке. Договор этот проверяется дифференциальной
			 * сверкой подачи и нарушается легче, чем кажется - всякий знак, требующий
			 * взгляда на следующий за ним, нарушает его при нарезке ровно между ними
			 * @note Поле выдаётся своим событием, а запись - отдельным событием конца.
			 * Собирать запись целиком чтению не нужно, и запись из тысячи полей проходит
			 * через него, не оседая в памяти
			 *
			 *  @code{.cpp}
			 *  reader_t reader(log);
			 *
			 *  reader.feed(chunk.data(), chunk.size(), last);
			 *
			 *  while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *          case static_cast <uint8_t> (event_t::HEADER): break;
			 *          case static_cast <uint8_t> (event_t::FIELD): break;
			 *          case static_cast <uint8_t> (event_t::RECORD): break;
			 *      }
			 *  }
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the streaming reading of a CSV text
			 * @details The text is accepted by chunks of an arbitrary size, while the issuance is conducted
			 * by events as the parsing goes on: the reading is not required to hold the text in full
			 * @par Order of the work
			 * @warning The output of the parsing does not depend on how the source text is cut into
			 * chunks: one and the same sequence of the events with one and the same places
			 * is obtained at any cutting. This contract is checked by a differential
			 * comparison of the feeding and is violated more easily than it seems — every character requiring
			 * a look at the one following it violates it at a cutting exactly between them
			 * @note A field is issued by its own event, while a record — by a separate event of the end.
			 * The reading does not need to assemble a record in full, and a record of a thousand fields passes
			 * through it without settling in the memory
			 *
			 *  @code{.cpp}
			 *  reader_t reader(log);
			 *
			 *  reader.feed(chunk.data(), chunk.size(), last);
			 *
			 *  while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *          case static_cast <uint8_t> (event_t::HEADER): break;
			 *          case static_cast <uint8_t> (event_t::FIELD): break;
			 *          case static_cast <uint8_t> (event_t::RECORD): break;
			 *      }
			 *  }
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
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
					 * @brief Настройки разбора текста
					 *
					 * \~english
					 * @brief Settings of the parsing of a text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Знак-разделитель полей, ноль - определять по содержимому
						 *
						 * @note Умолчанием берётся запятая, названная договором. Ноль
						 * включает определение по содержимому: разбор откладывает выдачу
						 * до тех пор, пока разделитель не определится
						 *
						 * \~english
						 * Separator character of the fields, zero — determine by the content
						 * @note By default the comma named by the protocol is taken. Zero
						 * enables the determination by the content: the parsing postpones the issuance
						 * until the separator is determined
						 *
						 * \~
						 */
						char separator;
						// Знак кавычек, обрамляющих поле
						char quote;
						/**
						 * \~russian
						 * Знак, начинающий строку примечания, ноль - примечаний нет
						 *
						 * @note Договор примечаний не описывает вовсе, потому умолчанием
						 * их нет: строка, начинающаяся с решётки, обыкновенная запись
						 *
						 * \~english
						 * Character beginning a comment line, zero — there are no comments
						 * @note The protocol does not describe the comments at all, therefore by default
						 * there are none: a line beginning with a hash is an ordinary record
						 *
						 * \~
						 */
						char comment;
						// Способ записи кавычки внутри поля, заключённого в кавычки
						escape_t escape;
						// Признак наличия заголовка в тексте
						header_t header;
						// Обращение с обвязкой вокруг поля
						trim_t trim;
						// Обращение с записью, число полей которой расходится с заголовком
						ragged_t ragged;
						// Кодировка, навязанная извне вопреки метке порядка байтов
						encoding_t encoding;
						/**
						 * \~russian
						 * Признак строгого следования RFC 4180
						 *
						 * @note Строгий разбор отвечает отказом на одиночную кавычку
						 * внутри поля без кавычек, на знаки за закрывающей кавычкой и на
						 * одиночный перевод строки в качестве конца записи. Обиход этого
						 * не соблюдает, потому умолчанием разбор нестрогий
						 *
						 * \~english
						 * Flag of the strict following of RFC 4180
						 * @note The strict parsing answers with a refusal to a single quote
						 * inside a field without quotes, to the characters after a closing quote and to
						 * a single line feed as the end of a record. The custom does not
						 * observe this, therefore by default the parsing is a non-strict one
						 *
						 * \~
						 */
						bool strict;
						// Признак выдачи событий пустых строк
						bool emitBlanks;
						// Признак выдачи событий примечаний
						bool emitComments;
						/**
						 * \~russian
						 * Признак проверки повторного объявления имён полей в заголовке
						 *
						 * @note Проверка стоит памяти - разбор удерживает имена, - и
						 * отключают её там, где заголовок заведомо свой
						 *
						 * \~english
						 * Flag of the check of a repeated declaration of the names of the fields in the header
						 * @note The check costs memory — the parsing holds the names — and
						 * it is disabled where the header is known to be one's own
						 *
						 * \~
						 */
						bool duplicates;
						// Наибольшая допустимая длина поля в байтах, ноль - без предела
						uint32_t maxField;
						// Наибольшая допустимая длина записи в байтах, ноль - без предела
						uint32_t maxRecord;
						// Наибольшее допустимое количество полей в записи, ноль - без предела
						uint32_t maxFields;
						// Количество первых записей, по которым определяется разделитель
						uint32_t detect;
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
						Settings() noexcept;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Состояние разбора текста
					 *
					 * @details Состояние хранит и то, что обыкновенно достаётся взглядом на
					 * следующий знак: знак, чьё значение зависит от следующего за ним, лишь
					 * переводит разбор в отдельное состояние. Иначе нарезка текста ровно
					 * между такими знаками меняла бы выдачу
					 *
					 * \~english
					 * @brief State of the parsing of a text
					 * @details The state stores also what is ordinarily obtained by a look at the
					 * next character: a character whose meaning depends on the one following it merely
					 * moves the parsing into a separate state. Otherwise a cutting of the text exactly
					 * between such characters would change the output
					 *
					 * \~
					 */
					enum class state_t : uint8_t {
						RECORD_START   = 0x00, // Начало записи, поле ещё не начато
						FIELD_START    = 0x01, // Начало поля, вид его ещё не определён
						UNQUOTED       = 0x02, // Внутри поля без кавычек
						QUOTED         = 0x03, // Внутри поля в кавычках
						QUOTE_IN_FIELD = 0x04, // Кавычка внутри поля в кавычках, значение её решает следующий знак
						ESCAPE         = 0x05, // Знак отмены внутри поля в кавычках
						AFTER_CR       = 0x06, // Возврат каретки, принадлежность его решает следующий знак
						COMMENT        = 0x07, // Внутри строки примечания
						FAILED         = 0x08, // Разбор прекращён ошибкой
						/**
						 * \~russian
						 * Знак отмены внутри поля без кавычек
						 *
						 * @details Стоит последним, а не рядом с состоянием знака отмены внутри
						 * поля в кавычках, ибо перечень этот вызывающему виден: перенумеровать
						 * прежние состояния значило бы переменить договор ради порядка чтения
						 *
						 * \~english
						 * Escape character inside a field without quotes
						 * @details It stands last rather than beside the state of the escape character inside
						 * a field in quotes, for this list is visible to the caller: to renumber the previous
						 * states would mean to change the contract for the sake of the order of reading
						 *
						 * \~
						 */
						ESCAPE_UNQUOTED = 0x09
					};
					/**
					 * \~russian
					 * @brief Событие разбора, собранное для выдачи
					 *
					 * \~english
					 * @brief Parsing event assembled for the issuance
					 *
					 * \~
					 */
					typedef struct Item {
						// Вид собранного события
						event_t event;
						// Указание на содержимое события в хранилище записи
						span_t content;
						// Указание на имя поля в хранилище заголовка
						span_t name;
						// Положение события в исходном тексте
						location_t location;
						// Признак того, что поле было заключено в кавычки
						bool quoted;
						// Признак того, что содержимое поля было изменено разбором
						bool modified;
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
						Item() noexcept :
						 event(event_t::NONE), quoted(false), modified(false) {}
					} item_t;
				private:
					// Настройки разбора текста
					settings_t _settings;
				private:
					// Состояние разбора текста
					state_t _state;
					// Код ошибки разбора
					error_t _error;
					// Кодировка исходного текста
					encoding_t _encoding;
				private:
					// Знак-разделитель полей, определённый или заданный
					char _separator;
				private:
					/**
					 * \~russian
					 * Количество полей, каким надлежит обладать всякой записи
					 *
					 * @note Заводится лишь при сверке количества полей и без заголовка:
					 *       назначает его первая запись, а ноль означает, что записей
					 *       ещё не было и назначать количество нечему
					 *
					 * \~english
					 * Number of the fields which every record ought to possess
					 * @note Established only at the check of the number of the fields and without a header:
					 *       it is assigned by the first record, while zero means that there have been
					 *       no records yet and there is nothing to assign the number by
					 *
					 * \~
					 */
					uint32_t _expected;
				private:
					// Признак того, что метка порядка байтов уже разобрана
					bool _marked;
					// Признак того, что текст подан целиком
					bool _last;
					// Признак того, что заголовок уже разобран
					bool _headed;
					// Признак того, что текущее поле было заключено в кавычки
					bool _quoted;
					// Признак того, что содержимое текущего поля изменено разбором
					bool _modified;
					// Признак того, что запись содержит хотя бы одно поле
					bool _started;
				private:
					// Приведение исходного текста к кодировке UTF-8
					decoder_t _decoder;
				private:
					// Хранилище знаков текущей записи
					string _storage;
					// Хранилище имён полей заголовка
					string _names;
					// Хранилище байтов, отложенных до определения разделителя
					string _pending;
					// Хранилище перекодированного куска текста
					string _decoded;
				private:
					// Указания на имена полей заголовка в хранилище имён
					vector <span_t> _header;
					// Имена полей заголовка для проверки повторного объявления
					unordered_set <string> _unique;
				private:
					// Очередь собранных событий разбора
					/**
					 * \~russian
					 * Очередь собранных событий разбора
					 *
					 * @note Очередь заведена перечнем с указанием на голову, а не двусторонней
					 *       очередью: последняя заводит по блоку памяти на каждые несколько
					 *       десятков событий и освобождает его по опустошении, отчего разбор
					 *       крупной таблицы стоил двадцати шести тысяч выделений памяти.
					 *       Перечень же выделенное удерживает и переиспользует: после первой
					 *       таблицы выделений не бывает вовсе
					 *
					 * \~english
					 * Queue of the assembled parsing events
					 * @note The queue has been made a list with a pointer to the head rather than a deque:
					 *       the latter creates a block of memory per every few
					 *       dozen events and releases it upon the emptying, from which the parsing
					 *       of a large table cost twenty-six thousand memory allocations.
					 *       A list, however, holds and reuses what has been allocated: after the first
					 *       table there are no allocations at all
					 *
					 * \~
					 */
					vector <item_t> _items;
					/**
					 * \~russian
					 * Указание на первое невыданное событие в очереди
					 *
					 * @note По выдаче последнего события очередь очищается целиком, а
					 *       выделенное под неё остаётся: тем и достигается переиспользование
					 *
					 * \~english
					 * Pointer to the first unissued event in the queue
					 * @note Upon the issuance of the last event the queue is cleared in full, while
					 *       what has been allocated for it remains: that is how the reuse is achieved
					 *
					 * \~
					 */
					size_t _head;
					// Событие разбора, выданное последним
					item_t _current;
				private:
					// Положение начала текущего поля в исходном тексте
					location_t _position;
					// Смещение от начала текста в байтах
					uint64_t _offset;
					// Номер текущей строки, считая с единицы
					uint32_t _line;
					// Положение в текущей строке, считая с единицы
					uint32_t _column;
					// Номер текущей записи, считая с единицы
					uint32_t _record;
					// Номер текущего поля в записи, считая с нуля
					uint32_t _field;
					// Количество полей, накопленное записью
					uint32_t _count;
					// Длина текущей записи в байтах
					uint32_t _length;
					// Смещение начала текущего поля в хранилище знаков записи
					uint32_t _begin;
					/**
					 * \~russian
					 * Смещение первого значащего знака текущего поля в хранилище записи
					 *
					 * @details Значащая часть поля - то, что останется от него по снятии
					 * обвязки, - отслеживается по ходу разбора, а не считается по его
					 * окончании: предел длины поля обязан отвечать отказом в том месте, где
					 * он переполнен, а не тогда, когда поле уже осело в памяти целиком
					 *
					 * \~english
					 * Offset of the first significant character of the current field in the storage of the record
					 * @details The significant part of a field — what will be left of it once the trimming
					 * is taken off — is tracked along the parsing rather than counted at its
					 * end: the limit of the length of a field must answer with a refusal at the place where
					 * it is overflown rather than when the field has already settled into the memory in full
					 *
					 * \~
					 */
					uint32_t _from;
					/**
					 * \~russian
					 * Смещение за последним значащим знаком текущего поля в хранилище записи
					 *
					 * @note Совпадение его с началом поля означает, что значащих знаков поле
					 * ещё не набрало: всё накопленное - обвязка
					 *
					 * \~english
					 * Offset past the last significant character of the current field in the storage of the record
					 * @note Its coincidence with the beginning of the field means that the field has not gathered
					 * significant characters yet: everything accumulated is the trimming
					 *
					 * \~
					 */
					uint32_t _till;
				private:
					// Признак того, что разделитель определён или задан
					bool _detected;
				private:
					/**
					 * \~russian
					 * Разметка знаков, разбор поля без кавычек прерывающих
					 *
					 * @details Разметка эта служит быстрому проходу по знакам, состояния
					 * разбора не меняющим: знак, в ней не отмеченный, содержимым поля и
					 * является, и потому целые куски такого содержимого переносятся в
					 * хранилище разом, минуя переключение состояний
					 *
					 * @note Разметка перестраивается при всякой смене настроек и по
					 *       определении разделителя: знаки, разбор прерывающие, зависят и
					 *       от разделителя, и от знака кавычек, и от способа отмены
					 *
					 * \~english
					 * Map of the characters interrupting the parsing of a field without quotes
					 * @details This map serves the fast pass over the characters not changing the states
					 * of the parsing: a character not marked in it is the content of the field
					 * itself, and therefore whole chunks of such content are transferred into
					 * the storage at once, bypassing the switching of the states
					 * @note The map is rebuilt at every change of the settings and upon the
					 *       determination of the separator: the characters interrupting the parsing depend both
					 *       on the separator, and on the quote character, and on the way of the escaping
					 *
					 * \~
					 */
					bool _breakUnquoted[256];
					// Разметка знаков, разбор поля в кавычках прерывающих
					bool _breakQuoted[256];
				private:
					/**
					 * \~russian
					 * @brief Метод перестроения разметки знаков, разбор прерывающих
					 *
					 * \~english
					 * @brief Method of rebuilding the map of the characters interrupting the parsing
					 *
					 * \~
					 */
					void marking() noexcept;
					/**
					 * \~russian
					 * @brief Метод быстрого прохода по знакам, состояния не меняющим
					 *
					 * @details Проходятся знаки, содержимым поля являющиеся: переносятся они
					 * в хранилище разом, а учёт положения ведётся сложением. Договор о
					 * независимости выдачи от нарезки текста проход этот не задевает:
					 * знаки, чьё значение зависит от следующего за ними, разметкой отмечены
					 * и проходом не берутся
					 *
					 * @param buffer буфер проходимых знаков текста
					 * @param size   размер буфера проходимых знаков текста
					 * @return       количество пройденных знаков
					 *
					 * \~english
					 * @brief Method of the fast pass over the characters not changing the state
					 * @details The characters being the content of a field are passed: they are transferred
					 * into the storage at once, while the accounting of the position is conducted by an addition. The contract about
					 * the independence of the output from the cutting of the text is not affected by this pass:
					 * the characters whose meaning depends on the one following them are marked in the map
					 * and are not taken by the pass
					 * @param buffer buffer of the characters of the text being passed
					 * @param size   size of the buffer of the characters of the text being passed
					 * @return       number of the passed characters
					 *
					 * \~
					 */
					size_t bulk(const char * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора приведённого куска текста
					 *
					 * @param text разбираемый приведённый кусок текста
					 * @return     признак продолжения разбора
					 *
					 * \~english
					 * @brief Method of parsing a converted chunk of a text
					 * @param text converted chunk of the text being parsed
					 * @return     flag of the continuation of the parsing
					 *
					 * \~
					 */
					bool process(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора очередного знака текста
					 *
					 * @param letter разбираемый знак
					 * @return       признак продолжения разбора
					 *
					 * \~english
					 * @brief Method of parsing the next character of a text
					 * @param letter character being parsed
					 * @return       flag of the continuation of the parsing
					 *
					 * \~
					 */
					bool parse(const char letter) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора знака состоянием разбора
					 *
					 * @details Знак, сменивший состояние, разбирается заново уже новым
					 * состоянием, и разбор потому вызывает себя повторно тем же знаком. Учёт
					 * пределов вынесен наружу именно поэтому: считаемый здесь, он рос бы на
					 * всякий заход
					 *
					 * @param letter разбираемый знак
					 * @return       признак продолжения разбора
					 *
					 * \~english
					 * @brief Method of parsing a character by the state of the parsing
					 * @details A character that has changed the state is parsed anew already by the new
					 * state, and therefore the parsing calls itself again with the same character. The accounting of the
					 * limits is taken outside exactly for this reason: counted here, it would grow at
					 * every entry
					 * @param letter character being parsed
					 * @return       flag of the continuation of the parsing
					 *
					 * \~
					 */
					bool step(const char letter) noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения разбора текста
					 *
					 * @details Вызывается по окончании текста и доводит до конца то, что
					 * осталось незавершённым: последнее поле без знака конца строки,
					 * последнюю запись и отложенное состояние возврата каретки
					 *
					 * \~english
					 * @brief Method of completing the parsing of a text
					 * @details Called upon the end of the text and brings to the end what has
					 * remained unfinished: the last field without a line ending character,
					 * the last record and the postponed state of a carriage return
					 *
					 * \~
					 */
					void finish() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод определения разделителя по отложенному тексту
					 *
					 * @details Разделитель определяется постоянством количества полей в
					 * записях, а не частотой знака: частота обманывается текстом, где
					 * запятых в значениях больше, чем разделителей
					 *
					 * @note Отказать определение не умеет: текст, разделителя не содержащий,
					 *       разбирается в один столбец с запятой, названной договором
					 *
					 * \~english
					 * @brief Method of determining the separator by the postponed text
					 * @details The separator is determined by the constancy of the number of the fields in the
					 * records rather than by the frequency of a character: the frequency is deceived by a text where
					 * there are more commas in the values than separators
					 *
					 * \~
					 */
					void detect() noexcept;
					/**
					 * \~russian
					 * @brief Метод подсчёта полей при заданном разделителе
					 *
					 * @param separator знак-разделитель для проверки
					 * @param counts    количество полей по записям
					 * @return          признак пригодности разделителя
					 *
					 * \~english
					 * @brief Method of counting the fields at a given separator
					 * @param separator separator character to be checked
					 * @param counts    number of the fields by the records
					 * @return          flag of the suitability of the separator
					 *
					 * \~
					 */
					bool count(const char separator, vector <uint32_t> & counts) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод завершения текущего поля
					 *
					 * @return признак продолжения разбора
					 *
					 * \~english
					 * @brief Method of completing the current field
					 * @return flag of the continuation of the parsing
					 *
					 * \~
					 */
					bool complete() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения текущей записи
					 *
					 * @return признак продолжения разбора
					 *
					 * \~english
					 * @brief Method of completing the current record
					 * @return flag of the continuation of the parsing
					 *
					 * \~
					 */
					bool commit() noexcept;
					/**
					 * \~russian
					 * @brief Метод занесения ошибки разбора
					 *
					 * @param error код ошибки разбора
					 * @return      признак продолжения разбора, всегда ложь
					 *
					 * \~english
					 * @brief Method of recording a parsing error
					 * @param error error code of the parsing
					 * @return      flag of the continuation of the parsing, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод занесения ошибки разбора с указанием места
					 *
					 * @details Место указывается там, где знак, отказ вызвавший, к мигу
					 * обнаружения отказа уже пройден: предел, сличаемый по накопленному,
					 * узнаёт о переполнении лишь по дописывании знака, а указывать обязан
					 * на сам знак - ровно так же, как это делает предел длины записи,
					 * сличаемый до его прохождения
					 *
					 * @param error    код ошибки разбора
					 * @param location место ошибки в исходном тексте
					 * @return         признак продолжения разбора, всегда ложь
					 *
					 * \~english
					 * @brief Method of recording a parsing error with an indication of the place
					 * @details The place is indicated there where the character that has caused the refusal has
					 * already been passed by the moment of the detection of the refusal: a limit compared against what has been
					 * accumulated learns of the overflow only upon the appending of the character, while it is obliged to point
					 * at the character itself — exactly as the limit of the length of a record does, being
					 * compared before the passing of it
					 * @param error    error code of the parsing
					 * @param location place of the error in the source text
					 * @return         flag of the continuation of the parsing, always false
					 *
					 * \~
					 */
					bool fail(const error_t error, const location_t & location) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод снятия обвязки с накопленного содержимого поля
					 *
					 * @param offset смещение начала содержимого в хранилище записи
					 * @return       отрезок содержимого без обвязки
					 *
					 * \~english
					 * @brief Method of removing the padding from the accumulated content of a field
					 * @param offset offset of the beginning of the content in the storage of the record
					 * @return       segment of the content without the padding
					 *
					 * \~
					 */
					span_t trim(const uint32_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта значащей части поля по вновь накопленным знакам
					 *
					 * @details Учёт ведётся по знакам, дописанным в хранилище с указанного
					 * места: обвязка предела не расходует, и потому отслеживаются границы
					 * значащей части, а не общая длина накопленного
					 *
					 * @param offset размер хранилища записи до дописывания знаков
					 *
					 * \~english
					 * @brief Method of the accounting of the significant part of a field by the newly accumulated characters
					 * @details The accounting is kept by the characters appended into the storage from the specified
					 * place: the trimming does not spend the limit, and therefore the boundaries of the significant
					 * part are tracked rather than the total length of what has been accumulated
					 * @param offset size of the storage of the record before the appending of the characters
					 *
					 * \~
					 */
					void significant(const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения длины поля, пределом ограничиваемой
					 *
					 * @details Длина эта считается по значащей части поля, если обвязка
					 * настройками снимается, и по накопленному целиком, если сохраняется
					 *
					 * @return длина текущего поля в байтах
					 *
					 * \~english
					 * @brief Method of the obtaining of the length of a field restricted by the limit
					 * @details This length is counted by the significant part of a field if the trimming
					 * is taken off by the settings, and by what has been accumulated in full if it is preserved
					 * @return length of the current field in bytes
					 *
					 * \~
					 */
					uint32_t extent() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки снятия обвязки у текущего поля
					 *
					 * @return признак того, что обвязка текущего поля снимается настройками
					 *
					 * \~english
					 * @brief Method of the check of the removal of the trimming of the current field
					 * @return sign that the trimming of the current field is taken off by the settings
					 *
					 * \~
					 */
					bool trimming() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки нахождения разбора внутри поля
					 *
					 * @details Хранилище записи копит не одни лишь поля: строка примечания
					 * ложится туда же, полем не являясь. Предел длины поля её не касается,
					 * и сличаться он обязан лишь пока разбор внутри поля
					 *
					 * @return признак того, что разбор находится внутри поля
					 *
					 * \~english
					 * @brief Method of the check of the presence of the parsing inside a field
					 * @details The storage of a record accumulates not the fields alone: a line of a comment
					 * lies down there as well without being a field. The limit of the length of a field does not concern it,
					 * and it must be compared only while the parsing is inside a field
					 * @return sign that the parsing is inside a field
					 *
					 * \~
					 */
					bool inside() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения длины поля, какою она станет при значащем знаке следом
					 *
					 * @details Пробельные знаки остаются обвязкою лишь до тех пор, пока за ними
					 * не встал знак значащий: встав, он обращает их в середину значащей части, и
					 * длина поля прирастает разом на всю их череду. Быстрый проход укорачивается
					 * потому по худшему случаю, а не по нынешней длине значащей части
					 *
					 * @return длина текущего поля в байтах по худшему случаю
					 *
					 * \~english
					 * @brief Method of the obtaining of the length of a field as it will become upon a significant character next
					 * @details Whitespace characters remain a trimming only for as long as a significant character has not
					 * stood after them: having stood, it turns them into the middle of the significant part, and
					 * the length of the field grows at once by their whole run. The fast pass is therefore shortened
					 * by the worst case rather than by the present length of the significant part
					 * @return length of the current field in bytes by the worst case
					 *
					 * \~
					 */
					uint32_t pending() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния разбора
					 *
					 * @details Настройки разбора сохраняются: сбрасывается лишь состояние,
					 * накопленное разбором поданного текста
					 *
					 * \~english
					 * @brief Method of resetting the state of the parsing
					 * @details The settings of the parsing are preserved: only the state accumulated
					 * by the parsing of the fed text is reset
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи очередного куска текста
					 *
					 * @note Пустой указатель на буфер приравнивается к пустой подаче независимо от
					 * заданного размера: проверка стоит в приведении текста, и разыменования не
					 * происходит. Отвечает разбор при этом по своему правилу о пустом тексте
					 *
					 * @param buffer буфер с куском текста
					 * @param size   размер куска текста в байтах
					 * @param last   признак того, что кусок является последним
					 * @return       признак успешного разбора поданного куска
					 *
					 * \~english
					 * @brief Method of feeding the next chunk of a text
					 * @note A null pointer to the buffer is equated to an empty feeding independently of
					 * the given size: the check stands in the conversion of the text, and no dereferencing
					 * takes place. The parsing answers thereby by its own rule about an empty text
					 * @param buffer buffer with the chunk of the text
					 * @param size   size of the chunk of the text in bytes
					 * @param last   flag of the chunk being the last one
					 * @return       flag of the successful parsing of the fed chunk
					 *
					 * \~
					 */
					bool feed(const char * buffer, const size_t size, const bool last = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи текста целиком
					 *
					 * @param text текст для разбора
					 * @return     признак успешного разбора
					 *
					 * \~english
					 * @brief Method of feeding a text in full
					 * @param text text to be parsed
					 * @return     flag of a successful parsing
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему событию разбора
					 *
					 * @return признак наличия события
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 * @return flag of the presence of an event
					 *
					 * \~
					 */
					bool next() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения вида текущего события разбора
					 *
					 * @return вид текущего события разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the kind of the current parsing event
					 * @return kind of the current parsing event
					 *
					 * \~
					 */
					event_t event() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения поля текущего события разбора
					 *
					 * @details Ссылки поля живут до следующего обращения к next: хранилище
					 * записи дописывается по ходу разбора и при росте перемещается
					 *
					 * @return поле текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the field of the current parsing event
					 * @details The references of a field live until the next call to next: the storage
					 * of the record is appended to in the course of the parsing and at a growth it is moved
					 * @return field of the current parsing event
					 *
					 * \~
					 */
					field_t field() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения текущего события разбора
					 *
					 * @return положение текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of the current parsing event
					 * @return position of the current event in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения отказа
					 *
					 * @details Место это выдаётся независимо от того, исчерпаны ли события
					 * разбора: `location()` отдаёт его лишь по исчерпании их, оставаясь до
					 * того местом текущего события, и обойтись одним им можно не всегда
					 *
					 * @note Доступ этот заведён ради единообразия с кодеками JSON и XML: там
					 *       место отказа иначе не получить вовсе
					 *
					 * @return положение обнаруженного отказа в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
					 * @details This place is issued independently of whether the events of the parsing are
					 * exhausted: `location()` issues it only upon their exhaustion, remaining until then
					 * the place of the current event, and it is not always possible to make do with it alone
					 * @note This accessor is provided for the sake of the uniformity with the codecs JSON and XML:
					 *       there the place of the error cannot be obtained otherwise at all
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
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
					 * @brief Метод получения кодировки исходного текста
					 *
					 * @return кодировка, определённая по метке порядка байтов
					 *
					 * \~english
					 * @brief Method of getting the encoding of the source text
					 * @return encoding determined by the byte order mark
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения знака-разделителя полей
					 *
					 * @details Разделитель, определённый по содержимому, известен лишь
					 * после того, как определение состоялось: до тех пор выводится ноль
					 *
					 * @return знак-разделитель полей
					 *
					 * \~english
					 * @brief Method of getting the separator character of the fields
					 * @details A separator determined by the content is known only
					 * after the determination has taken place: until then zero is output
					 * @return separator character of the fields
					 *
					 * \~
					 */
					char separator() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения имён полей заголовка
					 *
					 * @details Имена доступны лишь при включённом признаке заголовка и
					 * лишь после того, как заголовок разобран
					 *
					 * @return имена полей заголовка в порядке объявления
					 *
					 * \~english
					 * @brief Method of getting the names of the fields of the header
					 * @details The names are available only when the flag of the header is enabled and
					 * only after the header has been parsed
					 * @return names of the fields of the header in the order of the declaration
					 *
					 * \~
					 */
					vector <string_view> header() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора текста
					 *
					 * @return настройки разбора текста
					 *
					 * \~english
					 * @brief Method of getting the settings of the parsing of a text
					 * @return settings of the parsing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора текста
					 *
					 * @param settings настройки разбора текста
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of a text
					 * @param settings settings of the parsing of a text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
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
					Reader(const log_t * log) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки объекта ведения журнала работы
					 *
					 * @param log объект ведения журнала работы
					 *
					 * \~english
					 * @brief Method of the setting of the object of the keeping of the work log
					 *
					 * @param log the object of the keeping of the work log
					 *
					 * \~
					 */
					void setLogger(const log_t * log) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки разбора текста
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the parsing of a text
					 *
					 * \~
					 */
					Reader(const log_t * log, const settings_t & settings) noexcept;
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
					~Reader() noexcept {}
			} reader_t;
		}
	}
}

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_CSV_READER__
