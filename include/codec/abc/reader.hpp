/**
 * @file reader.hpp
 * @date 2026-08-18
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
 * @brief Заголовочный файл поточного чтения бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the streaming reading of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_READER__
#define __AWH_CODEC_ABC_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 */
#include "../../sys/macro_push.hpp"

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
	 * Подписываемся на стандартное пространство имён
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
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Класс поточного чтения бинарной записи
			 *
			 * @details Чтение выдаёт события по мере разбора записи, не удерживая её целиком.
			 * Разбор ведётся без рекурсии: вложенность хранится стеком вместимых, а не стеком
			 * вызовов
			 *
			 * @details **Независимость от нарезки на куски.** Единица записи, поданная не
			 * целиком, разбор не сдвигает: недостающие октеты дожидаются следующей подачи, а
			 * выдача событий от того, как запись нарезана, не зависит ничем
			 *
			 * \~english
			 * @brief Class of the streaming reading of a binary record
			 * @details The reading issues the events as the record is parsed without holding it in full.
			 * The parsing is conducted without a recursion: the nesting is held by a stack of the containers rather than by the stack
			 * of the calls
			 * @details **Independence from the cutting into chunks.** A unit of the record submitted not
			 * in full does not shift the parsing: the missing octets wait for the next submission, while
			 * the issuance of the events does not depend in any way on how the record is cut
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора записи
					 *
					 * \~english
					 * @brief Settings of the parsing of a record
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Признак разбора потока документов
						 *
						 * @note Поток этот - последовательность документов подряд, без всякого
						 * разделителя: длина у двоичной записи объявлена, и конец документа
						 * опознаётся ею, а не знаком
						 *
						 * \~english
						 * Flag of the parsing of a stream of the documents
						 * @note This stream is a sequence of the documents in a row without any
						 * separator: the length of a binary record is declared, and the end of a document
						 * is recognised by it rather than by a character
						 *
						 * \~
						 */
						bool stream;
						// Признак проверки строк на соответствие кодировке UTF-8
						bool validate;
						// Наибольшая допустимая длина строкового значения в октетах, ноль - без предела
						uint64_t maxString;
						// Наибольшая допустимая длина двоичного значения в октетах, ноль - без предела
						uint64_t maxBlob;
						// Наибольшая допустимая глубина вложенности, ноль - предел модуля
						uint32_t maxDepth;
						// Наибольшее допустимое количество узлов документа, ноль - предел модуля
						uint32_t maxNodes;
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
					/**
					 * \~russian
					 * @brief Значение, выданное событием разбора
					 *
					 * @details Значение ссылается на память, принадлежащую разбору, и живёт не
					 * дольше следующей подачи куска записи
					 *
					 * @note Число выдаётся тем видом, каким оно записано: разбор его не
					 * преобразует и не огрубляет. Целое свыше родных видов и десятичное
					 * выдаются октетами величины вместе со знаком и порядком
					 *
					 * \~english
					 * @brief Value issued by an event of the parsing
					 * @details The value refers to the memory belonging to the parsing and lives no
					 * longer than the next submission of a chunk of the record
					 * @note A number is issued by that kind by which it is recorded: the parsing does not
					 * convert and does not coarsen it. An integer above the native kinds and a decimal one
					 * are issued by the octets of the magnitude together with the sign and the exponent
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Value {
						/**
						 * \~russian
						 * Содержимое значения
						 *
						 * @note У строки и у двоичных данных - их октеты; у опознавателя -
						 * шестнадцать его октетов; у целого свыше родных видов и у десятичного -
						 * октеты величины от младшего к старшему
						 *
						 * \~english
						 * Content of the value
						 * @note For a string and for binary data — their octets; for an identifier —
						 * its sixteen octets; for an integer above the native kinds and for a decimal one —
						 * the octets of the magnitude from the low one to the high one
						 *
						 * \~
						 */
						string_view data;
						// Вид значения
						type_t type;
						// Количество значений вместимого, ноль при неопределённой длине
						uint64_t count;
						// Целое без знака
						uint64_t number;
						// Целое со знаком, а у отметки времени - её значение
						int64_t integer;
						// Дробное число
						double real;
						// Десятичный порядок величины
						int64_t exponent;
						// Логическое значение
						bool boolean;
						// Признак того, что величина меньше нуля
						bool negative;
						// Признак неопределённой длины вместимого
						bool indefinite;
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
						Value() noexcept :
						 type(type_t::UNDEFINED), count(0), number(0), integer(0), real(0.0),
						 exponent(0), boolean(false), negative(false), indefinite(false) {}
					} value_t;
					/**
					 * \~russian
					 * @brief Обработчик прямой выдачи событий разбора
					 *
					 * \~english
					 * @brief Handler of the direct issuance of the events of the parsing
					 *
					 * \~
					 */
					typedef void (* handler_t) (void * context, Reader & reader, const event_t event);
				private:
					/**
					 * \~russian
					 * @brief Состояние разбора записи
					 *
					 * @details Состояние хранит то, чего не видно в поданном куске: единица,
					 * поданная наполовину, переводит разбор в отдельное состояние, а не требует
					 * недостающих октетов немедленно
					 *
					 * \~english
					 * @brief State of the parsing of a record
					 * @details The state holds that which is not visible in the submitted chunk: a unit
					 * submitted by half puts the parsing into a separate state rather than demanding
					 * the missing octets immediately
					 *
					 * \~
					 */
					enum class state_t : uint8_t {
						ITEM,             // Ожидается ведущий октет очередной единицы
						SEGMENT,          // Ожидаются октеты строки либо двоичных данных
						SINGLE,           // Ожидаются октеты одиночного значения
						EXTEND_EXPONENT,  // Ожидается десятичный порядок величины
						EXTEND_LENGTH,    // Ожидается длина октетов величины
						EXTEND_SIGN,      // Ожидается знак величины
						EXTEND_DATA,      // Ожидаются октеты величины
						FINISHED,         // Разбор завершён
						FAILED            // Разбор отвечен отказом
					};
					/**
					 * \~russian
					 * @brief Звено стека вместимых
					 *
					 * \~english
					 * @brief Link of the stack of the containers
					 *
					 * \~
					 */
					typedef struct Frame {
						// Признак того, что вместимое является отображением
						bool mapping;
						// Признак неопределённой длины вместимого
						bool indefinite;
						// Признак того, что следующим ожидается имя поля
						bool expectKey;
						// Количество оставшихся значений вместимого
						uint64_t remain;
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
						Frame() noexcept : mapping(false), indefinite(false), expectKey(false), remain(0) {}
					} frame_t;
					/**
					 * \~russian
					 * @brief Собранное событие разбора
					 *
					 * @details Содержимое хранится отрезком в буфере разбора, а не видом на
					 * него: буфер растёт дописыванием и вправе переехать в памяти
					 *
					 * \~english
					 * @brief Assembled event of the parsing
					 * @details The content is held by a segment in the buffer of the parsing rather than by a view
					 * of it: the buffer grows by an appending and may move in the memory
					 *
					 * \~
					 */
					typedef struct Record {
						// Вид события разбора
						event_t event;
						// Вид значения
						type_t type;
						// Отрезок содержимого значения в буфере разбора
						span_t span;
						// Количество значений вместимого
						uint64_t count;
						// Целое без знака
						uint64_t number;
						// Целое со знаком
						int64_t integer;
						// Дробное число
						double real;
						// Десятичный порядок величины
						int64_t exponent;
						// Логическое значение
						bool boolean;
						// Признак того, что величина меньше нуля
						bool negative;
						// Признак неопределённой длины вместимого
						bool indefinite;
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
						Record() noexcept :
						 event(event_t::NONE), type(type_t::UNDEFINED), count(0), number(0),
						 integer(0), real(0.0), exponent(0), boolean(false), negative(false),
						 indefinite(false) {}
					} record_t;
				private:
					// Настройки разбора записи
					settings_t _settings;
				private:
					// Состояние разбора записи
					state_t _state;
				private:
					// Код отказа разбора
					error_t _error;
				private:
					// Положение разбора в поданной записи
					location_t _location;
				private:
					// Буфер накопленных октетов поданной записи
					vector <uint8_t> _buffer;
				private:
					// Смещение разбора в буфере накопленных октетов
					size_t _offset;
				private:
					// Смещение начала буфера от начала поданной записи
					uint64_t _origin;
				private:
					// Стек вместимых разбора
					vector <frame_t> _stack;
				private:
					// Очередь собранных событий разбора
					deque <record_t> _events;
				private:
					// Событие, выданное последним переходом
					record_t _current;
				private:
					// Количество разобранных узлов документа
					uint32_t _nodes;
				private:
					// Количество недостающих октетов ожидаемой записи
					uint64_t _pending;
				private:
					// Вид значения, чьи октеты ожидаются
					type_t _awaited;
				private:
					// Разновидность ожидаемого расширения
					extend_t _extend;
				private:
					// Десятичный порядок ожидаемой величины
					int64_t _exponent;
				private:
					// Признак того, что величина ожидаемого расширения меньше нуля
					bool _negative;
				private:
					// Признак завершённости хотя бы одного документа
					bool _document;
				private:
					// Обработчик прямой выдачи событий разбора
					handler_t _handler;
				private:
					// Опора обработчика прямой выдачи событий разбора
					void * _context;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа разбора
					 *
					 * @param error код отказа разбора
					 * @return      признак успешности разбора, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a refusal of the parsing
					 * @param error error code of the parsing
					 * @return sign of the success of the parsing, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи собранного события разбора
					 *
					 * @param record собранное событие разбора
					 *
					 * \~english
					 * @brief Method of the issuance of an assembled event of the parsing
					 * @param record assembled event of the parsing
					 *
					 * \~
					 */
					void emit(const record_t & record) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи события завершённого значения
					 *
					 * @details Работа сама решает, событием значения выдать запись либо
					 * событием имени поля, и сама закрывает вместимые, чьи значения исчерпаны
					 *
					 * @param record собранное событие разбора
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the issuance of an event of a completed value
					 * @details The work itself decides whether to issue the record by an event of a value or
					 * by an event of a name of a field, and itself closes the containers whose values are exhausted
					 * @param record assembled event of the parsing
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool settle(record_t & record) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия исчерпанных вместимых
					 *
					 * @return признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the closing of the exhausted containers
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool unwind() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора накопленных октетов записи
					 *
					 * @return признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the accumulated octets of a record
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool process() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора очередной единицы проволочной записи
					 *
					 * @param done признак того, что единицу разобрать не удалось
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the next unit of the wire record
					 * @param done sign that the unit could not be parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool item(bool & done) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния разбора
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of the parsing
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод прекращения разбора
					 *
					 *
					 * \~english
					 * @brief Method of the termination of the parsing
					 *
					 * \~
					 */
					void abort() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи куска разбираемой записи
					 *
					 * @param buffer буфер подаваемой записи
					 * @param size   размер буфера подаваемой записи
					 * @param last   признак того, что кусок последний
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the feeding of a chunk of a record being parsed
					 * @param buffer buffer of the record being fed
					 * @param size size of the buffer of the record being fed
					 * @param last flag that the chunk is the last one
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool last = false) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему собранному событию
					 *
					 * @return признак наличия события
					 *
					 * \~english
					 * @brief Method of the transition to the next assembled event
					 * @return sign of the presence of an event
					 *
					 * \~
					 */
					bool next() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида текущего события
					 *
					 * @return вид текущего события разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the current event
					 * @return kind of the current event of the parsing
					 *
					 * \~
					 */
					event_t event() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения текущего события
					 *
					 * @return значение текущего события разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the value of the current event
					 * @return value of the current event of the parsing
					 *
					 * \~
					 */
					value_t value() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки обработчика прямой выдачи событий разбора
					 *
					 * @param callback устанавливаемый обработчик, ноль - снятие обработчика
					 * @param context  опора обработчика
					 *
					 * \~english
					 * @brief Method of the setting of the handler of the direct issuance of the events of the parsing
					 * @param callback handler being set, zero — removal of the handler
					 * @param context support of the handler
					 *
					 * \~
					 */
					void handler(handler_t callback, void * context) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа разбора
					 *
					 * @return код отказа разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the parsing
					 * @return error code of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения положения разбора в поданной записи
					 *
					 * @return положение разбора в поданной записи
					 *
					 * \~english
					 * @brief Method of the extraction of the position of the parsing in the submitted record
					 * @return position of the parsing in the submitted record
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения глубины вложенности разбора
					 *
					 * @return глубина вложенности разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the depth of the nesting of the parsing
					 * @return depth of the nesting of the parsing
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения настроек разбора записи
					 *
					 * @return настройки разбора записи
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the parsing of a record
					 * @return settings of the parsing of a record
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записи
					 *
					 * @param settings устанавливаемые настройки разбора записи
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a record
					 * @param settings settings of the parsing of a record being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
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
					Reader() noexcept;
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
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_READER__
