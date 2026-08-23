/**
 * @file writer.hpp
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
 * @brief Заголовочный файл записи текста настроек TOML — класс Writer, собирающий текст
 *        из объявлений таблиц, пар «ключ = значение», перечней, встроенных таблиц и
 *        примечаний по правилам описания версии 1.0.0
 *
 * \~english
 * @brief Header file of the writing of a TOML settings text — the Writer class, which assembles the text
 *        out of the table declarations, the pairs «key = value», the arrays, the inline tables and the
 *        comments by the rules of the specification of the version 1.0.0
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML_WRITER__
#define __AWH_CODEC_TOML_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
		 * @brief Пространство имён контейнера TOML
		 *
		 *
		 * \~english
		 * @brief TOML container namespace
		 *
		 * \~
		 */
		namespace toml {
			/**
			 * \~russian
			 * @brief Класс записи текста настроек
			 *
			 * @details Собирает текст настроек из объявлений таблиц, пар «ключ = значение»,
			 * перечней, встроенных таблиц и примечаний. Порядок вызовов повторяет порядок
			 * событий чтения: имя ключа, за ним его значение, а значение составное - парой
			 * открытия и закрытия, - отчего перезапись прочитанного сводится к пересылке
			 * событий
			 *
			 * @par Порядок работы
			 *
			 * @par Намеренные решения
			 * @li **Запись отвечает отказом, а не молчаливой подменой.** Имя либо значение,
			 * которое разбор прочитал бы иначе, чем оно записано, в текст не попадает: там,
			 * где ограждение настройками не дозволено, запись выдаёт отказ. Единственное
			 * послабление задаётся настройкой @c promote и касается лишь выбора записи
			 * строки, содержимого её не меняя
			 * @li **Учёт повторов имён запись не ведёт.** Реестра объявленных имён у неё
			 * нет вовсе: повтор ловит дерево настроек, а завести реестр здесь значило бы
			 * держать второй свод правил рядом с тем, что уже ведёт разбор. Потребитель,
			 * зовущий запись напрямую, за неповторимость имён отвечает сам
			 * @li **Текст, оборванный отказом, не выдаётся вовсе.** Отказ, случившийся
			 * после того, как операция уже дописала начало своё, оставляет строку
			 * оборванной, и признак этот липкий: строку вправе завершить знаком конца
			 * строки следующая удачная операция, и по одному лишь виду собранного текста
			 * рваность после того неразличима. Снимается признак лишь сбросом записи
			 * @li **Перечень пишется в одну строку, пока не велено иначе.** Многострочная
			 * запись перечня заводится указанием при его открытии: собранному
			 * потребителем перечню многострочность не нужна, а перезаписи прочитанного она
			 * возвращает вид, выбранный человеком
			 * @warning Значение, записи которого выбранная потребителем ограда не несёт -
			 * дословная строка с одинарной кавычкой, строка с управляющим знаком, - либо
			 * записывается оградой, его несущей, либо отвергается: обрезать содержимое
			 * запись не вправе
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.comment("собрано автоматически");
			 *  writer.table("server");
			 *  writer.key("host");
			 *  writer.text("127.0.0.1");
			 *  writer.key("port");
			 *  writer.integer(8080);
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the writing of a settings text
			 * @details Assembles a settings text out of the table declarations, the pairs «key = value»,
			 * the arrays, the inline tables and the comments. The order of the calls repeats the order
			 * of the events of the reading: the name of a key, after it its value, while a compound value — by a pair
			 * of an opening and a closing — from which a rewriting of what has been read comes down to a forwarding of
			 * the events
			 * @par Order of the work
			 * @par Deliberate decisions
			 * @li **The writing answers with a refusal rather than with a silent substitution.** A name or a value
			 * which the parsing would read otherwise than it is written does not get into the text: there
			 * where the fencing is not permitted by the settings the writing issues a refusal. The only
			 * relaxation is given by the @c promote setting and concerns only the choice of the notation
			 * of a string without changing its content
			 * @li **An array is written in a single line until it is ordered otherwise.** The multiline
			 * notation of an array is established by an indication at its opening: an array assembled by
			 * the consumer does not need the multilineness, while for a rewriting of what has been read it
			 * returns the form chosen by the human
			 * @warning A value the notation of which the fence chosen by the consumer does not carry —
			 * a literal string with a single quote, a string with a control character — is either
			 * written by a fence that carries it or is rejected: the writing has no right to
			 * truncate the content
			 *
			 *  @code{.cpp}
			 *  writer_t writer(log);
			 *
			 *  writer.comment("built automatically");
			 *  writer.table("server");
			 *  writer.key("host");
			 *  writer.text("127.0.0.1");
			 *  writer.key("port");
			 *  writer.integer(8080);
			 *
			 *  const string & text = writer.text();
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
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
					const Logging * _log;
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста настроек
					 *
					 * \~english
					 * @brief Settings of the writing of a settings text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Наибольшая допустимая длина логической строки в байтах,
						 * ноль - без предела
						 *
						 * @note Предел тот же, что и у разбора: строку длиннее записать
						 * значило бы собрать текст, который читающий отвергнет
						 *
						 * \~english
						 * Largest admissible length of a logical line in bytes,
						 * zero — without a limit
						 * @note The limit is the same as that of the parsing: to write a longer line would
						 * mean to assemble a text which the reader will reject
						 *
						 * \~
						 */
						uint32_t maxLine;
						/**
						 * Наибольшая допустимая длина составной части имени ключа в байтах,
						 * ноль - без предела
						 */
						uint32_t maxKey;
						/**
						 * \~russian
						 * Наибольшая допустимая глубина вложенности значений
						 *
						 * @note Смысл тот же, что и у разбора: считается вложенностью
						 * перечней и встроенных таблиц друг в друга, а ноль запрещает
						 * вложенные значения вовсе, оставляя простые. Без предела здесь
						 * не бывает - разойтись с разбором значило бы собрать текст,
						 * который читающий отвергнет
						 *
						 * \~english
						 * Largest admissible depth of the nesting of the values
						 * @note The meaning is the same as that of the parsing: it is counted by the nesting of the
						 * arrays and of the inline tables into one another, while zero prohibits the
						 * nested values altogether, leaving the simple ones. There is no «without a limit» here —
						 * to diverge from the parsing would mean to assemble a text
						 * which the reader will reject
						 *
						 * \~
						 */
						uint32_t maxDepth;
						/**
						 * Наибольшее допустимое количество составных частей имени ключа,
						 * ноль - без предела
						 */
						uint32_t maxParts;
						/**
						 * \~russian
						 * Признак того, что читающий признаёт знаки Юникода в имени без кавычек
						 *
						 * @note Описание версии 1.0.0 отводит имени без кавычек лишь знаки
						 * US-ASCII, и умолчанием признак снят: имя с прочими знаками
						 * ограждается тогда кавычками. Взводят его по читающему, а не по
						 * желанию пишущего - иначе собранный текст читающему не прочесть
						 *
						 * \~english
						 * Flag of the reader recognizing the Unicode characters in a name without quotes
						 * @note The specification of the version 1.0.0 allots only the US-ASCII characters to a name without quotes,
						 * and by default the flag is not set: a name with the other characters
						 * is then fenced with quotes. It is raised by the reader rather than by
						 * the wish of the writer — otherwise the assembled text cannot be read by the reader
						 *
						 * \~
						 */
						bool unicode;
						/**
						 * \~russian
						 * Признак дозволения смены записи строки, содержимого не несущей
						 *
						 * @note Дословная строка управляющих последовательностей не
						 * признаёт, и одинарную кавычку записать ею нечем. При взведённом
						 * признаке запись берёт ближайшую ограду, содержимое несущую, при
						 * снятом - отвечает отказом. Содержимое при этом не меняется ни в
						 * том, ни в другом случае: меняется лишь выбранная человеком ограда
						 *
						 * \~english
						 * Flag of the permission of a change of the notation of a string that does not carry the content
						 * @note A literal string does not recognize the escape sequences,
						 * and there is nothing to write a single quote with it. When the flag is
						 * raised the writing takes the nearest fence that carries the content, when it is
						 * not — it answers with a refusal. The content is thereby not changed in
						 * either case: only the fence chosen by the human changes
						 *
						 * \~
						 */
						bool promote;
						// Флаг записи пробелов вокруг знака равенства
						bool spaces;
						/**
						 * \~russian
						 * Флаг записи отступа перед парами объявленной таблицы
						 *
						 * @note Отступ этот украшающий: описание пробельную обвязку строки
						 * отбрасывает, и на разбор он не влияет
						 *
						 * \~english
						 * Flag of the writing of an indent before the pairs of a declared table
						 * @note This indent is an adorning one: the specification discards the whitespace padding of a line,
						 * and it does not affect the parsing
						 *
						 * \~
						 */
						bool indent;
						// Флаг записи пустой строки перед объявлением таблицы
						bool separated;
						// Вид знака конца строки собираемого текста
						newline_t newline;
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
					 * @brief Виды окружения записываемого значения
					 *
					 * \~english
					 * @brief Kinds of the surrounding of the value being written
					 *
					 * \~
					 */
					enum class context_t : uint8_t {
						ROOT   = 0x00, // Запись ведётся строками текста настроек
						KEYED  = 0x01, // Записано имя ключа, ожидается его значение
						ARRAY  = 0x02, // Запись ведётся внутри перечня значений
						INLINE = 0x03  // Запись ведётся внутри встроенной таблицы
					};
					/**
					 * \~russian
					 * @brief Уровень вложенности собираемого значения
					 *
					 * \~english
					 * @brief Level of the nesting of the value being assembled
					 *
					 * \~
					 */
					typedef struct Level {
						// Вид окружения записываемого значения
						context_t context;
						// Количество значений, записанных на этом уровне
						uint32_t count;
						/**
						 * \~russian
						 * Признак многострочной записи перечня
						 *
						 * @note У встроенной таблицы признак этот всегда снят: описание
						 *       переноса строки внутри неё не дозволяет
						 *
						 * \~english
						 * Flag of the multiline notation of an array
						 * @note For an inline table this flag is always unset: the specification does not permit
						 *       a line break inside it
						 *
						 * \~
						 */
						bool multiline;
						/**
						 * \~russian
						 * Признак того, что разделитель очередного значения уже записан
						 *
						 * @note Взводится примечанием, дописанным к значению перечня:
						 *       запятая ставится перед примечанием, а не после него, и
						 *       записать её второй раз значило бы собрать пустое значение
						 *
						 * \~english
						 * Flag of the separator of the next value having already been written
						 * @note Raised by a comment appended to a value of an array:
						 *       the comma is put before the comment rather than after it, and
						 *       to write it a second time would mean to assemble an empty value
						 *
						 * \~
						 */
						bool separated;
						/**
						 * \~russian
						 * Признак того, что на уровне записано примечание
						 *
						 * @note Держится ради закрывающей скобки перечня: примечание длится
						 *       до конца строки, и скобка, за ним записанная, досталась бы
						 *       содержимому его
						 *
						 * \~english
						 * Flag of a comment having been written at the level
						 * @note It is kept for the sake of the closing bracket of an array: a comment lasts
						 *       until the end of the line, and the bracket written after it would go
						 *       to its content
						 *
						 * \~
						 */
						bool remarked;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param context   вид окружения записываемого значения
						 * @param multiline признак многострочной записи перечня
						 *
						 * \~english
						 * @brief Constructor
						 * @param context   kind of the surrounding of the value being written
						 * @param multiline flag of the multiline notation of an array
						 *
						 * \~
						 */
						Level(const context_t context, const bool multiline) noexcept :
						 context(context), count(0), multiline(multiline), separated(false), remarked(false) {}
					} level_t;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					// Признак того, что таблица текста настроек уже объявлена
					bool _tabled;
					/**
					 * \~russian
					 * Признак того, что к последней записанной строке можно дописать примечание
					 *
					 * @details Дописывается примечание к строке пары либо объявления таблицы:
					 * пустая строка примечанием перестала бы быть пустой, а к строке
					 * примечания дописывать нечего - оно и так примечание
					 *
					 * \~english
					 * Flag of a comment being appendable to the last written line
					 * @details A comment is appended to the line of a pair or of a table declaration:
					 * an empty line would cease to be an empty one from a comment, while to the line of
					 * a comment there is nothing to append — it is a comment as it is
					 *
					 * \~
					 */
					bool _trailable;
					/**
					 * \~russian
					 * Признак текста, отказом оборванного
					 * @details Отказ, случившийся после того, как операция уже дописала начало
					 * своё, оставляет строку оборванной, и выдавать собранный текст после него
					 * нельзя. Признак липкий: строку эту вправе завершить знаком конца строки
					 * следующая удачная операция, и по одному лишь виду собранного текста
					 * рваность после того неразличима
					 *
					 * \~english
					 * Flag of a text cut off by a refusal
					 * @details A refusal that occurred after the operation had already appended its own
					 * beginning leaves the line cut off, and the assembled text must not be issued after
					 * it. The flag is sticky: that line may be terminated with an end-of-line character
					 * by the next successful operation, and after that the tornness is indistinguishable
					 * by the appearance of the assembled text alone
					 *
					 * \~
					 */
					bool _torn;
				private:
					// Собираемый текст настроек
					string _text;
				private:
					// Длина собираемой логической строки в байтах
					size_t _length;
					/**
					 * \~russian
					 * Длина записи, знаком конца строки завершённой
					 *
					 * @details Примечание, дописываемое к готовой строке, снимает знак её
					 * конца и дописывает содержимое к той же записи: длина записи ей
					 * продолжается, а не считается заново
					 *
					 * @note Считать её заново по собранному тексту нельзя: разбор меряет
					 * пределом длины запись целиком - вместе со всеми строками
					 * многострочного значения, - а по тексту видна лишь последняя из них
					 *
					 * \~english
					 * Length of a record completed by a line ending character
					 * @details A comment appended to a ready line removes the character of its
					 * end and appends the content to the same record: the length of the record is
					 * continued by it rather than being counted anew
					 * @note It cannot be counted anew over the assembled text: the parsing measures a record as a whole
					 * by the limit of the length — together with all the lines of a
					 * multiline value — while over the text only the last of them is visible
					 *
					 * \~
					 */
					size_t _restore;
				private:
					// Стопа уровней вложенности собираемого значения
					vector <level_t> _levels;
				private:
					// Настройки записи текста настроек
					settings_t _settings;
				private:
					/**
					 * \~russian
					 * @brief Метод записи знака конца строки
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the line ending character
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool newline() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи знака конца строки внутри записи
					 *
					 * @details Строку собираемой записи знак этот не завершает: разбор
					 * меряет пределом длины запись целиком - вместе со всеми строками
					 * многострочного значения и многострочного перечня, - и сбрасывать
					 * счёт на нём значило бы собирать текст, читающим отвергаемый
					 *
					 * \~english
					 * @brief Method of writing a line ending character inside a record
					 * @details This character does not complete the line of the record being assembled: the parsing
					 * measures a record as a whole by the limit of the length — together with all the lines of
					 * a multiline value and of a multiline array — and to reset
					 * the count at it would mean to assemble a text rejected by the reader
					 *
					 * \~
					 */
					void fold() noexcept;
					/**
					 * \~russian
					 * @brief Метод дописывания последовательности знаков к собираемому тексту
					 *
					 * @param text дописываемая последовательность знаков
					 *
					 * \~english
					 * @brief Method of appending a sequence of characters to the text being assembled
					 * @param text sequence of characters being appended
					 *
					 * \~
					 */
					void append(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида окружения записываемого значения
					 *
					 * @return вид окружения, в котором ведётся запись
					 *
					 * \~english
					 * @brief Method of getting the kind of the surrounding of the value being written
					 * @return kind of the surrounding in which the writing is conducted
					 *
					 * \~
					 */
					context_t context() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки готовности записи очередной строки текста
					 *
					 * @details Строкой текста записываются объявление таблицы, пара,
					 * примечание и пустая строка: посреди собираемого значения им места нет
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the readiness of the writing of the next line of the text
					 * @details A table declaration, a pair, a comment and an empty line are written as a line of the text:
					 * there is no place for them in the middle of the value being assembled
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool ready() noexcept;
					/**
					 * \~russian
					 * @brief Метод запоминания отказа записи вместе с кодом ошибки
					 *
					 * @details Отказ, случившийся после того, как операция уже дописала начало
					 * своё, оставляет строку оборванной, и выдавать собранный текст после него
					 * нельзя: свой же разбор целым его не признает. Судится это здесь
					 * единственным телом - перечень мест, где отказ приходит после дописывания,
					 * разошёлся бы с кодом при первой же правке любого из них
					 *
					 * @param error код ошибки записи
					 * @return      признак отказа для выхода из записи
					 *
					 * \~english
					 * @brief Method of remembering a refusal of the writing together with the error code
					 * @details A refusal that occurred after the operation had already appended its own
					 * beginning leaves the line cut off, and the assembled text must not be issued after
					 * it: its own parsing will not recognize it as whole. This is judged here by a single
					 * body — a list of the places where a refusal arrives after an appending would
					 * diverge from the code at the very first edit of any of them
					 * @param error error code of the writing
					 * @return      flag of a refusal for the exit from the writing
					 *
					 * \~
					 */
					bool refuse(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки возможности открыть составное значение
					 *
					 * @details Проверяет и окружение записи, и глубину вложенности: перечень и
					 * встроенная таблица открываются по одним и тем же правилам, и держать их
					 * двумя телами значило бы развести своды правил при первой же правке
					 * одного из них
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of the checking of the possibility to open a compound value
					 * @details Checks both the context of the writing and the depth of the nesting: an array
					 * and an inline table are opened by one and the same rules, and keeping them
					 * as two bodies would mean parting the sets of the rules at the very first edit
					 * of one of them
					 * @return result of the performing of the operation
					 *
					 * \~
					 */
					bool nestable() noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки пригодности примечания внутри перечня значений
					 *
					 * @details Проверяет и место записи, и содержимое примечания: записывать
					 * его дозволено лишь внутри перечня, собираемого несколькими строками
					 *
					 * @param text содержимое записываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of checking the suitability of a comment inside an array of the values
					 * @details Checks both the place of the writing and the content of the comment: it is permitted to write
					 * it only inside an array assembled in several lines
					 * @param text content of the comment being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool remarkable(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи отступа перед парой объявленной таблицы
					 *
					 * \~english
					 * @brief Method of writing an indent before a pair of a declared table
					 *
					 * \~
					 */
					void indent() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи разделителя очередного значения перечня
					 *
					 * @details Ставит запятую перед очередным значением уровня и перенос
					 * строки с отступом, когда перечень пишется многострочным
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the separator of the next value of an array
					 * @details Puts a comma before the next value of the level and a line
					 * break with an indent when the array is written as a multiline one
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool separate() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения записи значения
					 *
					 * @details Записывает знак конца строки, когда значение записано парой
					 * верхнего уровня, и учитывает записанное значение уровнем вложенности
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of completing the writing of a value
					 * @details Writes a line ending character when the value has been written as a pair
					 * of the top level, and accounts the written value by the level of the nesting
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool complete() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи составного имени ключа
					 *
					 * @note Имя принимается указателем с количеством частей, а не
					 *       перечнем: имя из одной части передаётся тогда без построения
					 *       перечня, а построение это обходилось выделением памяти на
					 *       каждую записанную пару - на файле в две сотни пар их
					 *       набиралось столько же
					 *
					 * @param parts указатель на составные части имени ключа
					 * @param count количество составных частей имени ключа
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a compound name of a key
					 * @note The name is accepted as a pointer with a number of the parts rather than as a
					 *       list: a name of a single part is then passed without a construction of a
					 *       list, while that construction cost an allocation of the memory for
					 *       every written pair — on a file of two hundred pairs as many of them
					 *       accumulated
					 * @param parts pointer to the component parts of the name of the key
					 * @param count number of the component parts of the name of the key
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool naming(const part_t * parts, const size_t count) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени ключа пары со знаком равенства
					 *
					 * @param parts указатель на составные части имени ключа
					 * @param count количество составных частей имени ключа
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the name of the key of a pair with the equals sign
					 * @param parts pointer to the component parts of the name of the key
					 * @param count number of the component parts of the name of the key
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool keyed(const part_t * parts, const size_t count) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи составной части имени ключа
					 *
					 * @param part записываемая составная часть имени ключа
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a component part of the name of a key
					 * @param part component part of the name of the key being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool naming(const part_t & part) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения выбранной оградой
					 *
					 * @param text    записываемое строковое значение
					 * @param quoting запись строкового значения
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a string value by the chosen fence
					 * @param text    string value being written
					 * @param quoting notation of the string value
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool quoted(const string_view text, const string_t quoting) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки возможности записи строки выбранной оградой
					 *
					 * @param text    проверяемое строковое значение
					 * @param quoting проверяемая запись строкового значения
					 * @return        результат проверки
					 *
					 * \~english
					 * @brief Method of checking the possibility of writing a string by the chosen fence
					 * @param text    string value being checked
					 * @param quoting notation of the string value being checked
					 * @return        result of the check
					 *
					 * \~
					 */
					bool carried(const string_view text, const string_t quoting) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи отметки времени
					 *
					 * @param stamp записываемая отметка времени
					 * @param type  тип записываемой отметки времени
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a timestamp
					 * @param stamp timestamp being written
					 * @param type  type of the timestamp being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool stamped(const stamp_t & stamp, const type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи объявления таблицы
					 *
					 * @param parts указатель на составные части имени таблицы
					 * @param count количество составных частей имени таблицы
					 * @param array признак объявления очередной таблицы набора таблиц
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a table declaration
					 * @param parts pointer to the component parts of the name of the table
					 * @param count number of the component parts of the name of the table
					 * @param array flag of the declaration of the next table of an array of tables
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool declare(const part_t * parts, const size_t count, const bool array) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек записи
					 *
					 * @return текущие настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of getting the current settings of the writing
					 * @return current settings of the writing of a settings text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи
					 *
					 * @param settings настройки записи текста настроек
					 *
					 * \~english
					 * @brief Method of setting the settings of the writing
					 * @param settings settings of the writing of a settings text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи объявления таблицы
					 *
					 * @param path записываемое составное имя таблицы
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a table declaration
					 * @param path compound name of the table being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool table(const vector <part_t> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи объявления таблицы
					 *
					 * @param name записываемое имя таблицы
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a table declaration
					 * @param name name of the table being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool table(const string_view name) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи объявления очередной таблицы набора таблиц
					 *
					 * @param path записываемое составное имя набора таблиц
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the declaration of the next table of an array of tables
					 * @param path compound name of the array of tables being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool arrayTable(const vector <part_t> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи объявления очередной таблицы набора таблиц
					 *
					 * @param name записываемое имя набора таблиц
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the declaration of the next table of an array of tables
					 * @param name name of the array of tables being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool arrayTable(const string_view name) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи имени ключа пары
					 *
					 * @details За именем ключа обязано следовать его значение: строкой
					 * текста запись при незаписанном значении не продолжается
					 *
					 * @param path записываемое составное имя ключа
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the name of the key of a pair
					 * @details The name of a key is obliged to be followed by its value: the writing is not continued
					 * by a line of the text while the value has not been written
					 * @param path compound name of the key being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool key(const vector <part_t> & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени ключа пары
					 *
					 * @param name записываемое имя ключа
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the name of the key of a pair
					 * @param name name of the key being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool key(const string_view name) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи значения
					 *
					 * @details Записывает значение всякого простого типа по признакам его
					 * записи, сохранённым при чтении: оградой строки, системой счисления
					 * числа и видом отметки времени
					 *
					 * @param value записываемое значение
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a value
					 * @details Writes a value of any simple type by the attributes of its
					 * notation preserved at the reading: by the fence of a string, by the numeral system
					 * of a number and by the kind of a timestamp
					 * @param value value being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool value(const content_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param text    записываемое строковое значение
					 * @param quoting запись строкового значения
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a string value
					 * @param text    string value being written
					 * @param quoting notation of the string value
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool text(const string_view text, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи логического значения
					 *
					 * @param value записываемое логическое значение
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a logical value
					 * @param value logical value being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool boolean(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа
					 *
					 * @param value записываемое целое число
					 * @param radix система счисления записи числа
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing an integer
					 * @param value integer being written
					 * @param radix numeral system of the notation of the number
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool integer(const int64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числа с плавающей точкой
					 *
					 * @details Число записывается по правилам местности «C» и от
					 * установленной в приложении местности не зависит: разделитель дробной
					 * части местности сделал бы текст настроек непереносимым
					 *
					 * @param value записываемое число с плавающей точкой
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a floating-point number
					 * @details The number is written by the rules of the «C» locale and does not depend on the
					 * locale set in the application: the decimal separator
					 * of a locale would make the settings text non-portable
					 * @param value floating-point number being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool real(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи отметки времени
					 *
					 * @param stamp записываемая отметка времени
					 * @param type  тип записываемой отметки времени
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a timestamp
					 * @param stamp timestamp being written
					 * @param type  type of the timestamp being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool stamp(const stamp_t & stamp, const type_t type) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи начала перечня значений
					 *
					 * @param multiline признак записи перечня несколькими строками
					 * @return          результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the beginning of an array of the values
					 * @param multiline flag of the writing of the array in several lines
					 * @return          result of performing the operation
					 *
					 * \~
					 */
					bool arrayOpen(const bool multiline = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи конца перечня значений
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the end of an array of the values
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool arrayClose() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи начала встроенной таблицы
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the beginning of an inline table
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool inlineOpen() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи конца встроенной таблицы
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing the end of an inline table
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool inlineClose() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи примечания
					 *
					 * @details Примечание, занимающее несколько строк, записывается
					 * несколькими строками примечания: знак его начала ставится к каждой
					 *
					 * @param text содержимое записываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a comment
					 * @details A comment occupying several lines is written as
					 * several comment lines: the character of its beginning is put to each of them
					 * @param text content of the comment being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool comment(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод дописывания примечания к последней записанной строке
					 *
					 * @details Примечание дописывается в конец уже записанной строки - туда,
					 * где его оставил человек, писавший файл настроек руками. Служит
					 * перезаписи файла без обеднения его примечаниями
					 *
					 * @param text содержимое дописываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of appending a comment to the last written line
					 * @details The comment is appended to the end of an already written line — there
					 * where it has been left by the human who wrote the settings file by hand. Serves
					 * the rewriting of a file without impoverishing it of the comments
					 * @param text content of the comment being appended
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool trailing(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи примечания внутри перечня значений
					 *
					 * @details Примечание записывается своей строкой перечня, между
					 * значениями его. Перечень, примечание несущий, записывается
					 * несколькими строками по устройству: примечание оканчивается концом
					 * строки, и в одну строку такой перечень не собрать
					 *
					 * @param text содержимое записываемого примечания
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a comment inside an array of the values
					 * @details The comment is written as its own line of the array, between
					 * its values. An array that carries a comment is written
					 * in several lines by its arrangement: a comment ends with the end of a
					 * line, and such an array cannot be assembled into a single line
					 * @param text content of the comment being written
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool remark(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод дописывания примечания к значению перечня
					 *
					 * @details Примечание дописывается в конец строки последнего записанного
					 * значения перечня - вместе с запятой, его отделяющей. Служит перезаписи
					 * перечня без обеднения его примечаниями
					 *
					 * @note Запятая ставится лишь тогда, когда за примечанием следует
					 * очередное значение перечня: описание запятую в конце перечня дозволяет,
					 * но приписывать её перезаписью значило бы менять текст, ею не имевший
					 *
					 * @param text      содержимое дописываемого примечания
					 * @param separator признак того, что за примечанием следует значение
					 * @return          результат выполнения операции
					 *
					 * \~english
					 * @brief Method of appending a comment to a value of an array
					 * @details The comment is appended to the end of the line of the last written
					 * value of the array — together with the comma separating it. Serves the rewriting
					 * of an array without impoverishing it of the comments
					 * @note The comma is put only when the comment is followed by
					 * the next value of the array: the specification permits a comma at the end of an array,
					 * but to ascribe it by a rewriting would mean to change a text that did not have it
					 * @param text      content of the comment being appended
					 * @param separator flag of the comment being followed by a value
					 * @return          result of performing the operation
					 *
					 * \~
					 */
					bool remarked(const string_view text, const bool separator) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи пустой строки
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing an empty line
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool blank() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон типа записываемого числа
					 *
					 * @tparam T тип записываемого числа
					 *
					 * \~english
					 * @brief Template of the type of the number being written
					 * @tparam T type of the number being written
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод записи пары с числовым значением
					 *
					 * @param key   имя записываемого ключа
					 * @param value значение записываемой пары
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of writing a pair with a numeric value
					 * @param key   name of the key being written
					 * @param value value of the pair being written
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool number(const string_view key, const T value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки записи
					 *
					 * @return код ошибки последней операции записи
					 *
					 * \~english
					 * @brief Method of getting the error code of the writing
					 * @return error code of the last operation of the writing
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения собранного текста настроек
					 *
					 * @details Текст выдаётся собранным лишь целиком: незакрытый перечень
					 * либо имя ключа без значения оставляют его недописанным, и выдача
					 * такого текста ответила бы отказом. Отказом отвечает и текст, чья
					 * последняя строка знаком конца строки не завершена: отказ, случившийся
					 * после того, как операция уже дописала начало своё, оставляет строку
					 * оборванной
					 *
					 * @return собранный текст настроек
					 *
					 * \~english
					 * @brief Method of getting the assembled settings text
					 * @details The text is issued as assembled only in full: an unclosed array
					 * or the name of a key without a value leave it unfinished, and the issuance
					 * of such a text would answer with a refusal. A text whose last line is not
					 * terminated by an end-of-line character answers with a refusal as well: a failure
					 * that occurred after the operation had already appended its beginning leaves
					 * the line cut off
					 * @return assembled settings text
					 *
					 * \~
					 */
					const string & text() noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса записи в исходное состояние
					 *
					 * \~english
					 * @brief Method of resetting the writing into the initial state
					 *
					 * \~
					 */
					void clear() noexcept;
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
					Writer(const Logging * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки записи текста настроек
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the writing of a settings text
					 *
					 * \~
					 */
					Writer(const Logging * log, const settings_t & settings) noexcept;
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
					~Writer() noexcept;
			} writer_t;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_TOML_WRITER__
