/**
 * @file writer.hpp
 * @date 2026-08-14
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
 * @brief Заголовочный файл записи текста JSON — сборка исходящего текста значение за
 *        значением с потоковым изъятием собранного
 *
 * \~english
 * @brief Header file of the writing of a JSON text — the assembly of the outgoing text value by
 *        value with the streaming extraction of what has been assembled
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_JSON_WRITER__
#define __AWH_CODEC_JSON_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <sys/log.hpp>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
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
		 * @brief Пространство имён контейнера JSON
		 *
		 * \~english
		 * @brief JSON container namespace
		 *
		 * \~
		 */
		namespace json {
			/**
			 * \~russian
			 * @brief Запись текста JSON
			 *
			 * @details Текст собирается значение за значением, а собранное вправе изыматься
			 * по мере накопления: сборщик отдаёт накопленное и остаётся готовым продолжать.
			 * Оттого документ, в память не помещающийся, записывается тем же способом, что
			 * и малый
			 *
			 * @note Сборщик стережёт правильность строения: закрытие вместилища, какое не
			 * открывалось, и значение, поданное там, где ожидается имя поля, отвергаются.
			 * Собранный текст оттого правилен по строению всегда
			 *
			 * \~english
			 * @brief Writing of a JSON text
			 * @details The text is assembled value by value, while what has been assembled may be extracted
			 * as it accumulates: the assembler gives away what has been accumulated and remains ready to continue.
			 * Whereby a document not fitting into the memory is written by the same means as
			 * a small one
			 * @note The assembler guards the correctness of the structure: the closing of a container which has not
			 * been opened, and a value fed where the name of a field is expected, are rejected.
			 * The assembled text is thereby always correct by its structure
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста
					 *
					 * \~english
					 * @brief Settings of the writing of a text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Вид оформления собираемого текста
						format_t format;
						// Правило экранирования при записи
						escape_t escape;
						/**
						 * \~russian
						 * Правило обращения с последовательностью, кодировке UTF-8 не отвечающей
						 *
						 * @note Умолчанием стоит замена знаком U+FFFD: текст обязан остаться
						 * годным для разбора при любом содержимом строки, а знак замены
						 * предписан самой кодировкой и в тексте виден глазом
						 *
						 * \~english
						 * Rule of the handling of a sequence not conforming to the UTF-8 encoding
						 * @note The replacement by the character U+FFFD stands as the default: a text must remain
						 * suitable for the parsing at any content of a string, while the replacement character
						 * is prescribed by the encoding itself and is visible to the eye in the text
						 *
						 * \~
						 */
						malformed_t malformed;
						/**
						 * \~russian
						 * Количество пробелов отступа при оформлении с отступами
						 *
						 * @note Ноль означает отступ знаком табуляции
						 *
						 * \~english
						 * Number of the spaces of the indentation at the formatting with the indentations
						 * @note Zero means an indentation by a tabulation character
						 *
						 * \~
						 */
						uint8_t indent;
						/**
						 * \~russian
						 * Признак записи NaN и бесконечности словами вместо отказа
						 *
						 * @note Стандарт таких чисел не знает вовсе, и запись их делает
						 * текст негодным для строгого разбора
						 *
						 * \~english
						 * Flag of the writing of NaN and of the infinity by the words instead of a refusal
						 * @note The standard does not know such numbers at all, and the writing of them makes
						 * the text unsuitable for a strict parsing
						 *
						 * \~
						 */
						bool allowInfinityAndNan;
						/**
						 * \~russian
						 * Признак разделения документов переводом строки при потоке NDJSON
						 *
						 * \~english
						 * Flag of the separation of the documents by a line feed at an NDJSON stream
						 *
						 * \~
						 */
						bool stream;
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
					// Настройки записи текста
					settings_t _settings;
				private:
					// Собираемый текст
					string _result;
				private:
					/**
					 * \~russian
					 * Стек видов открытых вместилищ
					 *
					 * \~english
					 * Stack of the kinds of the opened containers
					 *
					 * \~
					 */
					vector <kind_t> _nesting;
				private:
					// Признак того, что вместилище ещё не получило ни одного значения
					bool _empty;
					// Признак того, что имя поля объекта записано, а значение ещё нет
					bool _keyed;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Кода отказа у записи нет вовсе, и наружу идёт голая ложь: журнал
					 *       здесь ЕДИНСТВЕННЫЙ способ узнать, чем запись не устроила
					 *
					 * \~english
					 * Object of the keeping of the work log
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * @brief Метод отказа записи с сообщением о доводе его в журнал
					 *
					 * @details Способ этот зовётся лишь там, где отказ ВОЗНИКАЕТ. Проброс чужого
					 * отказа через него не идёт: о беде сообщено там, где она случилась, и
					 * второе сообщение лишь запутало бы читающего журнал
					 *
					 * @param error код отказа записи
					 * @return       всегда ложь, ради возврата им из места отказа
					 *
					 * \~english
					 * @brief Method of the refusal of the writing with a report of its reason to the log
					 *
					 * @param error the code of the refusal of the writing
					 * @return       always false, for the returning by it from the place of the refusal
					 *
					 * \~
					 */
					bool refuse(const error_t error) noexcept;
				private:
					// Код отказа последней операции записи
					error_t _error;
				private:
					// Признак того, что хотя бы один документ уже записан
					bool _started;
				private:
					/**
					 * \~russian
					 * Признак того, что записанные документы разделены знаком перевода строки
					 *
					 * @details Заслон против склейки документов спрашивает ЭТОТ признак, а не
					 * настройку потока. Прежде он спрашивал настройку - и в тот миг, когда
					 * начинался второй документ, - тогда как разделитель пишется завершением
					 * ПЕРВОГО. Включение потока после завершения первого документа заслон
					 * проходило, а разделителя в тексте не было: замер дал `{"a":1}{` при
					 * успехе записи и пустом коде отказа - два документа, склеенные в текст,
					 * разбор какого распадётся на границе между ними
					 *
					 * \~english
					 * Sign that the written documents are separated by a line feed character
					 * @details The guard against the gluing of the documents asks THIS sign rather than
					 * the setting of the stream. Formerly it asked the setting — and at the moment when
					 * the second document began — whereas the separator is written by the finishing
					 * of the FIRST one. The enabling of the stream after the finishing of the first document passed
					 * the guard, while there was no separator in the text: the measurement gave `{"a":1}{` at
					 * a success of the writing and an empty code of the refusal — two documents glued into a text
					 * whose parsing would fall apart at the boundary between them
					 *
					 * \~
					 */
					bool _separated;
				private:
					// Количество байтов, изъятых из сборщика за всё время работы
					uint64_t _taken;
				private:
					/**
					 * \~russian
					 * @brief Метод записи разделителя перед очередным значением
					 *
					 * @details Записывает запятую, перевод строки и отступ - в объёме,
					 * заданном настройками оформления
					 *
					 * @return признак допустимости записи значения в этом месте
					 *
					 * \~english
					 * @brief Method of the writing of a separator before the next value
					 * @details Writes a comma, a line feed and an indentation — in the volume
					 * set by the settings of the formatting
					 * @return sign of the admissibility of the writing of a value in this place
					 *
					 * \~
					 */
					bool separate() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строки с экранированием
					 *
					 * @param text записываемая строка
					 *
					 * \~english
					 * @brief Method of the writing of a string with an escaping
					 * @param text string being written
					 *
					 * \~
					 */
					void quoted(const string & text) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод переноса готовой записи числа в текст документа
					 *
					 * @details Запись переносится без сличения со стандартом: сюда попадает лишь
					 * то, что писатель произвёл сам, а произвести негодную запись преобразование
					 * числа не может. Сличается только запись, пришедшая извне
					 *
					 * @param value переносимая запись числа
					 * @param size  длина переносимой записи числа
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the transfer of a ready record of a number into the text of a document
					 * @details The record is transferred without a comparison with the standard: only that which
					 * the writer produced itself gets here, and the conversion of a number cannot produce
					 * an unfit record. Only a record which came from the outside is compared
					 * @param value record of the number being transferred
					 * @param size length of the record of the number being transferred
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool produced(const char * value, const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод очистки собранного текста документа
					 *
					 * @details Очистка возвращает сборщик к пустому тексту, а выделенную память
					 * удерживает
					 *
					 * @note Имя общее с записью разметки и таблицы намеренно: возвращение к
					 *       пустому тексту у всех трёх кодеков зовётся `clear()`, а `reset()`
					 *       у них оставлено чтению
					 *
					 * \~english
					 * @brief Method of the clearing of the assembled text of a document
					 * @details The clearing returns the assembler to an empty text while holding
					 * the allocated memory
					 * @note The name is common with the writing of a markup and of a table deliberately:
					 * the returning to an empty text is called `clear()` in all the three codecs, while
					 * `reset()` is left to the reading in them
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия объекта
					 *
					 * @return признак успешности записи
					 *
					 * \~english
					 * @brief Method of the opening of an object
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool object() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия массива
					 *
					 * @return признак успешности записи
					 *
					 * \~english
					 * @brief Method of the opening of an array
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool array() noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого вместилища
					 *
					 * @return признак успешности записи
					 *
					 * \~english
					 * @brief Method of the closing of an opened container
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool close() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи имени поля объекта
					 *
					 * @warning Повтор имени поля запись НЕ СТЕРЕЖЁТ, а разбор его по
					 *          умолчанию ОТВЕРГАЕТ: правило `duplicate_t::ERROR` стоит
					 *          умолчанием у чтения, и текст `{"имя":1,"имя":2}`, записью
					 *          выданный без единого возражения, собственным разбором при
					 *          настройках по умолчанию не принимается вовсе. Замер: запись
					 *          отвечала успехом, разбор - кодом `DUPLICATE_KEY`. Звучащему,
					 *          повторы допускающему, надлежит либо не писать их, либо
					 *          разбирать правилом `FIRST`, `LAST` либо `KEEP`
					 *
					 * @note Запись разметки XML повтор свойства узла стережёт и отвергает
					 *       кодом `DUPLICATE_ATTRIBUTE`, держа имена открытого узла при
					 *       себе. Здесь того же не делается: полей у объекта бывает сколько
					 *       угодно, и хранилище имён под каждый открытый объект легло бы на
					 *       горячий путь записи. Решение о заслоне - за владельцем
					 *
					 * @param name записываемое имя поля объекта
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the name of a field of an object
					 * @warning The writing does NOT guard against a repetition of the name of a field, while
					 *          the parsing by default REJECTS it: the rule `duplicate_t::ERROR` stands as
					 *          the default of the reading, and the text `{"name":1,"name":2}`, issued by
					 *          the writing without a single objection, is not accepted at all by its own
					 *          parsing at the default settings
					 * @note The writing of an XML markup guards against a repetition of an attribute of a node
					 *       and rejects it by the code `DUPLICATE_ATTRIBUTE`. The same is not done here:
					 *       an object has as many fields as one likes, and a storage of the names for each
					 *       opened object would lie upon the hot path of the writing
					 * @param name name of the field of the object being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи пустого значения
					 *
					 * @return признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an empty value
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool null() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи логического значения
					 *
					 * @param value записываемое логическое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a logical value
					 * @param value logical value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @warning Негодная последовательность байтов UTF-8 в записываемом значении
					 * ЗАМЕЩАЕТСЯ знаком замены U+FFFD, а запись отвечает успехом: договор JSON
					 * велит тексту быть годным UTF-8, а замещение предписано самой кодировкой -
					 * годной началом последовательности считается наибольшая её часть, оттого
					 * `C3 28` даёт знак замены и скобку, а не два знака замены. Значение при этом
					 * меняется молча, и вызывающему, кому важна сохранность байтов, проверять
					 * их годность следует ДО записи
					 *
					 * @note Кодек XML в том же положении отвечает отказом `INVALID_ENCODING`:
					 * договор XML запрещает такие знаки прямо, тогда как JSON требует лишь
					 * годной кодировки текста. Расхождение это задано форматами, а не недоделкой
					 *
					 * @param value записываемое строковое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @warning An invalid sequence of UTF-8 bytes in the value being written is REPLACED
					 * with the replacement character U+FFFD, while the writing answers with a success: the json
					 * protocol orders a text to be a valid UTF-8, and the replacement is prescribed by the encoding
					 * itself — the largest part of a sequence that is valid as its beginning is considered parsed,
					 * therefore `C3 28` gives a replacement character and a bracket rather than two replacement characters.
					 * The value thereby changes silently, and a caller to whom the preservation of the bytes matters
					 * should check their validity BEFORE the writing
					 * @note The xml codec in the same position answers with an `INVALID_ENCODING` refusal:
					 * the xml protocol forbids such characters directly, whereas json requires only
					 * a valid encoding of the text. This divergence is set by the formats rather than by an omission
					 * @param value string value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения, поданного строкой языка Си
					 *
					 * @details Способ этот обязателен, а не удобства ради: без него запись
					 * строкового литерала уходила бы в запись ЛОГИЧЕСКОГО значения. Приведение
					 * `const char *` к `bool` язык числит стандартным, а к `std::string` -
					 * определяемым пользователем, и первое побеждает при выборе способа. Отказ
					 * при этом не выдаётся вовсе: `writer.value("да")` молча записывает `true`
					 *
					 * @param value записываемое строковое значение, ноль - пустое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value passed as a C string
					 * @details This method is obligatory rather than a convenience: without it the writing
					 * of a string literal would go into the writing of a LOGICAL value. The conversion
					 * of `const char *` to `bool` is counted by the language as a standard one, and to `std::string` —
					 * as a user-defined one, and the first one wins at the choice of the method. No failure
					 * is issued at that: `writer.value("yes")` silently writes `true`
					 * @param value string value being written, zero — empty value
					 * @return      sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const char * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа
					 *
					 * @param value записываемое целое число
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an integer
					 * @param value integer being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи беззнакового целого числа
					 *
					 * @param value записываемое беззнаковое целое число
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an unsigned integer
					 * @param value unsigned integer being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числа с плавающей запятой
					 *
					 * @details Записывается кратчайшей записью, читающейся обратно тем же
					 * числом
					 *
					 * @param value записываемое число с плавающей запятой
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a floating-point number
					 * @details It is written by the shortest record read back as the same
					 * number
					 * @param value floating-point number being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числа его готовой записью
					 *
					 * @details Запись переносится в текст как есть, минуя преобразование.
					 * Служит для переноса чисел, разобранных чтением, без потери точности
					 *
					 * @param value записываемая запись числа
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a number by its ready record
					 * @details The record is transferred into the text as it is bypassing the conversion.
					 * Serves for the transfer of the numbers parsed by the reading without a loss of the precision
					 * @param value record of the number being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool raw(const string & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод завершения документа при потоке NDJSON
					 *
					 * @return признак успешности записи
					 *
					 * \~english
					 * @brief Method of the completion of a document at an NDJSON stream
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool finish() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения собранного текста
					 *
					 * @return собранный текст
					 *
					 * \~english
					 * @brief Method of the extraction of the assembled text
					 * @return assembled text
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия собранного текста
					 *
					 * @details Отдаёт собранное и оставляет сборщик готовым продолжать: строение
					 * документа при этом сохраняется, а память освобождается
					 *
					 * @return изъятый собранный текст
					 *
					 * \~english
					 * @brief Method of the taking away of the assembled text
					 * @details Gives away what has been assembled and leaves the assembler ready to continue: the structure
					 * of the document is thereby preserved, while the memory is released
					 * @return taken away assembled text
					 *
					 * \~
					 */
					string take() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения размера собранного текста
					 *
					 * @return размер собранного текста в байтах
					 *
					 * \~english
					 * @brief Method of the extraction of the size of the assembled text
					 * @return size of the assembled text in bytes
					 *
					 * \~
					 */
					size_t size() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода отказа записи
					 *
					 * @return код отказа последней операции записи
					 *
					 * @note Прежде довод отказа уходил лишь в журнал строкою, и потребитель,
					 *       журнала не назначивший, признаком `false` довольствовался без
					 *       возможности узнать причину вовсе. Запись разметки XML отдаёт код
					 *       отказа с самого начала, и расходиться с нею здесь нечем
					 *
					 * \~english
					 * @brief Method of getting the error code of the writing
					 *
					 * @return error code of the last operation of the writing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения текущей глубины вложенности
					 *
					 * @return текущая глубина вложенности
					 *
					 * \~english
					 * @brief Method of the extraction of the current depth of the nesting
					 * @return current depth of the nesting
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения настроек записи текста
					 *
					 * @return настройки записи текста
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the writing of a text
					 * @return settings of the writing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи текста
					 *
					 * @param settings устанавливаемые настройки записи текста
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the writing of a text
					 * @param settings settings of the writing of a text being set
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
					explicit Writer(const log_t * log) noexcept;
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
					~Writer() noexcept {}
			} writer_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_JSON_WRITER__
