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
 * @brief Заголовочный файл потокового чтения текста настроек TOML — класс Reader,
 *        принимающий текст кусками произвольного размера и выдающий события разбора
 *
 * \~english
 * @brief Header file of the streaming reading of a TOML settings text — the Reader class,
 *        which accepts the text by chunks of an arbitrary size and issues the parsing events
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML_READER__
#define __AWH_CODEC_TOML_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <deque>
#include <unordered_map>

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
			 * @brief Класс потокового чтения текста настроек TOML
			 *
			 * @details Текст принимается кусками произвольного размера, а выдача ведётся
			 * событиями по мере разбора: удерживать текст целиком чтению не требуется
			 *
			 * @par Порядок работы
			 *
			 * @warning Выдача разбора не зависит от того, как исходный текст нарезан на
			 * куски: одна и та же последовательность событий с одними и теми же местами
			 * получается при всякой нарезке. Договор этот проверяется дифференциальной
			 * сверкой подачи и нарушается легче, чем кажется
			 * @note Значение составное выдаётся не одним событием, а рядом их: открытие
			 * перечня либо встроенной таблицы, содержимое, закрытие. Вложенность потому
			 * достаётся потребителю потоком, а не собранным заранее деревом
			 *
			 * \~english
			 * @brief Class of the streaming reading of a TOML settings text
			 * @details The text is accepted by chunks of an arbitrary size, while the issuance is conducted
			 * by events as the parsing goes on: the reading is not required to hold the text in full
			 * @par Order of the work
			 * @warning The output of the parsing does not depend on how the source text is cut into
			 * chunks: one and the same sequence of the events with one and the same places
			 * is obtained at any cutting. This contract is checked by a differential
			 * comparison of the feeding and is violated more easily than it seems
			 * @note A compound value is issued not by a single event but by a series of them: the opening
			 * of an array or of an inline table, the content, the closing. The nesting therefore
			 * goes to the consumer as a stream rather than as a tree assembled beforehand
			 *
			 * \~
			 *
			 * @code{.cpp}
			 * reader_t reader;
			 *
			 * reader.feed(chunk.data(), chunk.size(), last);
			 *
			 * while(reader.next()){
			 *     switch(static_cast <uint8_t> (reader.event())){
			 *         case static_cast <uint8_t> (event_t::TABLE): break;
			 *         case static_cast <uint8_t> (event_t::KEY): break;
			 *         case static_cast <uint8_t> (event_t::VALUE): break;
			 *     }
			 * }
			 * @endcode
			 *
			 *
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора текста настроек
					 *
					 * @details Настройки задают пределы разбора и объём выдачи, но не
					 * устройство записи: описание у TOML единственное, и наречий здесь нет
					 *
					 * \~english
					 * @brief Settings of the parsing of a settings text
					 * @details The settings give the limits of the parsing and the volume of the output, but not
					 * the arrangement of the notation: the specification of TOML is a single one, and there are no dialects here
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Наибольшая допустимая длина логической строки в байтах, ноль - без предела
						 *
						 * @note Предел считается на запись целиком - вместе со всеми строками
						 * многострочного значения, - иначе многострочная запись давала бы
						 * обход предела
						 *
						 * \~english
						 * Largest admissible length of a logical line in bytes, zero — without a limit
						 * @note The limit is counted over the record as a whole — together with all the lines of
						 * a multiline value — otherwise a multiline record would give
						 * a bypass of the limit
						 *
						 * \~
						 */
						uint32_t maxLine;
						// Наибольшая допустимая длина составной части имени ключа в байтах, ноль - без предела
						uint32_t maxKey;
						/**
						 * \~russian
						 * Наибольшая допустимая глубина вложенности значений
						 *
						 * @note Считается вложенностью перечней и встроенных таблиц друг в
						 * друга. Ноль запрещает вложенные значения вовсе, оставляя простые
						 *
						 * \~english
						 * Largest admissible depth of the nesting of the values
						 * @note Counted by the nesting of the arrays and of the inline tables into one
						 * another. Zero prohibits the nested values altogether, leaving the simple ones
						 *
						 * \~
						 */
						uint32_t maxDepth;
						// Наибольшее допустимое количество составных частей имени ключа, ноль - без предела
						uint32_t maxParts;
						/**
						 * \~russian
						 * Признак проверки повторного объявления ключей и таблиц
						 *
						 * @note Описание повтор запрещает, и проверка эта по умолчанию
						 * включена. Стоит она памяти - разбор удерживает имена уже
						 * объявленного, - и отключают её там, где текст заведомо свой
						 *
						 * \~english
						 * Flag of the check of a repeated declaration of the keys and of the tables
						 * @note The specification prohibits a repetition, and this check is enabled by
						 * default. It costs memory — the parsing holds the names of what has already been
						 * declared — and it is disabled where the text is known to be one's own
						 *
						 * \~
						 */
						bool duplicates;
						/**
						 * \~russian
						 * Признак признания знаков Юникода в именах без кавычек
						 *
						 * @note Описание версии 1.0.0 отводит именам без кавычек лишь знаки
						 * US-ASCII, и умолчанием берётся оно: имя «сервер» такому разбору
						 * положено записывать в кавычках. Черновик следующей версии знаки
						 * Юникода в именах дозволяет, и признак этот его включает - выбор
						 * оставлен потребителю, а не зашит
						 *
						 * \~english
						 * Flag of the recognition of the Unicode characters in the names without quotes
						 * @note The specification of the version 1.0.0 allots only the US-ASCII characters to the names without quotes,
						 * and it is taken by default: the name «сервер» for such a parsing
						 * ought to be written in quotes. The draft of the next version permits the Unicode
						 * characters in the names, and this flag enables it — the choice
						 * is left to the consumer rather than being hardwired
						 *
						 * \~
						 */
						bool unicode;
						// Признак выдачи событий примечаний
						bool emitComments;
						// Признак выдачи событий пустых строк
						bool emitBlanks;
						// Кодировка, навязанная извне вопреки метке порядка байтов
						encoding_t encoding;
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
					 * @brief Указание на последовательность знаков в хранилище записи
					 *
					 * @note Хранилище по ходу разбора записи прирастает и вправе быть
					 *       перенесено в памяти: удерживать указатели в него до окончания
					 *       разбора записи нельзя, а указания смещением переносимы
					 *
					 * \~english
					 * @brief Pointer to a sequence of characters in the storage of a record
					 * @note The storage grows in the course of the parsing of a record and has the right to be
					 *       moved in the memory: the pointers into it cannot be held until the end of
					 *       the parsing of the record, while the pointers by an offset are movable
					 *
					 * \~
					 */
					typedef struct Span {
						// Смещение начала последовательности в хранилище
						uint32_t offset;
						// Длина последовательности знаков
						uint32_t length;
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
						Span() noexcept : offset(0), length(0) {}
					} span_t;
					/**
					 * \~russian
					 * @brief Событие разбора, собранное для выдачи
					 *
					 * @details Запись разбирается целиком, а события её складываются в
					 * очередь: значение составное даёт их несколько, и выдавать их по
					 * мере разбора значило бы оставлять потребителя с событиями записи,
					 * которая ошибочна в своей второй половине
					 *
					 * \~english
					 * @brief Parsing event assembled for the issuance
					 * @details A record is parsed in full, while its events are put into a
					 * queue: a compound value gives several of them, and to issue them as
					 * the parsing goes on would mean to leave the consumer with the events of a record
					 * which is erroneous in its second half
					 *
					 * \~
					 */
					typedef struct Item {
						// Вид собранного события
						event_t event;
						// Тип значения события
						type_t type;
						// Запись строкового значения
						string_t quoting;
						// Система счисления записи целого числа
						radix_t radix;
						// Логическое значение
						bool boolean;
						// Признак того, что примечание дописано к готовой строке
						bool trailing;
						// Целое число со знаком
						int64_t integer;
						// Число с плавающей точкой
						double real;
						// Отметка времени
						stamp_t stamp;
						// Указание на содержимое события в хранилище записи
						span_t content;
						// Номер первой составной части имени ключа события
						uint32_t part;
						// Количество составных частей имени ключа события
						uint32_t parts;
						// Положение события в исходном тексте
						location_t location;
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
						 event(event_t::NONE), type(type_t::NONE), quoting(string_t::BASIC),
						 radix(radix_t::DECIMAL), boolean(false), trailing(false), integer(0),
						 real(0.0), part(0), parts(0) {}
					} item_t;
					/**
					 * \~russian
					 * @brief Составная часть имени ключа, собранная для выдачи
					 *
					 * \~english
					 * @brief Component part of the name of a key assembled for the issuance
					 *
					 * \~
					 */
					/**
					 * \~russian
					 * @brief Роды имён, объявляемых текстом настроек
					 *
					 * @details Правила описания об объявлении имён сводятся к роду уже
					 * объявленного: таблицу, заведённую заголовком исподволь, дозволено объявить
					 * явно, а заведённую составным именем ключа - нет; набором таблиц дозволено
					 * дополнить лишь набор таблиц; под парой не заводится ничего, а встроенная
					 * таблица не дополняется вовсе
					 *
					 * \~english
					 * @brief Kinds of the names declared by a settings text
					 * @details The rules of the specification about the declaration of the names come down to the kind of what has already been
					 * declared: a table created by a header implicitly is permitted to be declared
					 * explicitly, while one created by a compound name of a key — is not; an array of tables is permitted
					 * to be supplemented only by an array of tables; nothing is created under a pair, while an inline
					 * table is not supplemented at all
					 *
					 * \~
					 */
					enum class kind_t : uint8_t {
						NONE     = 0x00, // Имя не объявлено
						TABLE    = 0x01, // Имя объявлено заголовком таблицы
						ARRAY    = 0x02, // Имя объявлено заголовком набора таблиц
						IMPLICIT = 0x03, // Имя заведено исподволь объемлющим именем заголовка
						DOTTED   = 0x04, // Имя заведено исподволь составным именем ключа
						VALUE    = 0x05, // Имя объявлено парой с простым значением либо перечнем
						INLINE   = 0x06  // Имя объявлено парой со встроенной таблицей
					};
					/**
					 * \~russian
					 * @brief Имя, объявляемое разбираемой записью
					 *
					 * \~english
					 * @brief Name declared by the record being parsed
					 *
					 * \~
					 */
					typedef struct Pending {
						// Объявляемое полное имя
						string name;
						// Род объявляемого имени
						kind_t kind;
						/**
						 * \~russian
						 * Признак того, что имя заведено исподволь
						 *
						 * @note Заведённое исподволь уже объявленного не подменяет: таблица,
						 * объявленная заголовком, объемлющим именем ключа в заведённую исподволь
						 * не превращается
						 *
						 * \~english
						 * Flag of the name having been created implicitly
						 * @note What has been created implicitly does not replace what has already been declared: a table
						 * declared by a header does not turn into one created implicitly by an enclosing name of a key
						 *
						 * \~
						 */
						bool implied;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param name    объявляемое полное имя
						 * @param kind    род объявляемого имени
						 * @param implied признак того, что имя заведено исподволь
						 *
						 * \~english
						 * @brief Constructor
						 * @param name    full name being declared
						 * @param kind    kind of the name being declared
						 * @param implied flag of the name having been created implicitly
						 *
						 * \~
						 */
						Pending(const string_view name, const kind_t kind, const bool implied) noexcept :
						 name(name), kind(kind), implied(implied) {}
					} pending_t;
					typedef struct Segment {
						// Указание на имя части в хранилище записи
						span_t name;
						// Запись имени части в исходном тексте
						naming_t naming;
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
						Segment() noexcept : naming(naming_t::BARE) {}
					} segment_t;
				private:
					// Состояние разбора текста настроек
					state_t _state;
					// Код ошибки последней операции разбора
					error_t _error;
					/**
					 * \~russian
					 * Признак того, что разбираемой записи не хватает продолжения текста
					 *
					 * @note Разбор записи ведётся по накопленному тексту, и исчерпание его
					 * посреди записи означает не ошибку, а нужду в следующем куске: запись
					 * откатывается к своему началу и разбирается заново по приходе
					 * продолжения. Ошибкой исчерпание становится лишь на последнем куске
					 *
					 * \~english
					 * Flag of the record being parsed lacking a continuation of the text
					 * @note The parsing of a record is conducted over the accumulated text, and its exhaustion
					 * in the middle of a record means not an error but a need for the next chunk: the record
					 * is rolled back to its beginning and is parsed anew upon the arrival of
					 * the continuation. The exhaustion becomes an error only on the last chunk
					 *
					 * \~
					 */
					bool _hungry;
					/**
					 * \~russian
					 * Признак того, что исходный текст поступил целиком
					 *
					 * @note Признак этот запоминается подачей и берётся выдачей событий:
					 * разбор продолжается по мере обхода событий, и без него продолжение
					 * это считало бы текст незавершённым - последняя запись оставалась бы
					 * неразобранной, а состояние разбора незавершённым
					 *
					 * \~english
					 * Flag of the source text having arrived in full
					 * @note This flag is remembered by the feeding and is taken by the issuance of the events:
					 * the parsing continues as the events are traversed, and without it that continuation
					 * would consider the text unfinished — the last record would remain
					 * unparsed, while the state of the parsing would remain unfinished
					 *
					 * \~
					 */
					bool _final;
					/**
					 * \~russian
					 * Отложенный код ошибки приведения к кодировке UTF-8
					 *
					 * @note Приведение переносит в хранилище всё, что успело проверить, и
					 * отказ его выдаётся лишь по исчерпании приведённого начала: иначе
					 * разбор того же текста целиком и кусками расходился бы событиями
					 *
					 * \~english
					 * Postponed error code of the conversion to the UTF-8 encoding
					 * @note The conversion transfers into the storage everything it has managed to check, and
					 * its refusal is issued only upon the exhaustion of the converted beginning: otherwise
					 * the parsing of the same text in full and by chunks would diverge in the events
					 *
					 * \~
					 */
					error_t _decoding;
				private:
					// Приведение исходного текста к кодировке UTF-8
					decoder_t _decoder;
				private:
					// Настройки разбора текста настроек
					settings_t _settings;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					location_t _errorLocation;
					// Положение начала текущего события в исходном тексте
					location_t _location;
				private:
					// Приведённый к кодировке UTF-8 исходный текст
					string _buffer;
					// Хранилище содержимого событий разбираемой записи
					string _store;
				private:
					// Положение разбираемого знака в приведённом тексте
					size_t _offset;
					// Положение начала разбираемой записи в приведённом тексте
					size_t _start;
					/**
					 * \~russian
					 * Положение, с которого ищется знак конца записи, откаченной нуждою в
					 * продолжении текста
					 *
					 * @note Держится ради подачи мелкими кусками: без него запись
					 *       разбиралась бы заново на всяком куске, и разбор её обходился
					 *       бы квадратом длины своей
					 *
					 * \~english
					 * Position from which the ending character of a record rolled back by a need for
					 * a continuation of the text is sought
					 * @note It is kept for the sake of a feeding by small chunks: without it a record
					 *       would be parsed anew at every chunk, and its parsing would cost
					 *       the square of its own length
					 *
					 * \~
					 */
					size_t _probed;
					/**
					 * \~russian
					 * Счёт встроенных таблиц записи, собственного имени не имеющих
					 *
					 * @note Ключи такой таблицы повторами друг другу приходятся наравне с
					 *       ключами именованной, а учёт их ведётся по имени: имя ей даётся
					 *       здесь своё, живущее лишь до конца записи. Знаком «собака» оно и
					 *       начинается - настоящее полное имя начинается разделителем частей,
					 *       и столкнуться им не с чем
					 *
					 * \~english
					 * Count of the inline tables of a record that have no name of their own
					 * @note The keys of such a table are repetitions to one another on a par with
					 *       the keys of a named one, while their accounting is conducted by a name: a name of its own is given to it
					 *       here, living only until the end of the record. It begins with the «at» character —
					 *       a real full name begins with the separator of the parts,
					 *       and there is nothing for it to collide with
					 *
					 * \~
					 */
					uint32_t _anonymous;
					// Смещение начала приведённого текста от начала исходного в байтах
					uint64_t _base;
					// Номер разбираемой строки исходного текста, считая с единицы
					uint32_t _line;
					// Положение начала разбираемой строки в приведённом тексте
					size_t _bol;
				private:
					/**
					 * \~russian
					 * Хранилище собираемого содержимого строкового значения
					 *
					 * @note Держится полем ради переиспользования занятой памяти: очистка
					 *       её за хранилищем сохраняет, и всякое следующее строковое
					 *       значение обходится без выделения
					 *
					 * \~english
					 * Storage of the content of a string value being assembled
					 * @note It is kept as a member for the sake of the reuse of the occupied memory: its clearing
					 *       preserves it behind the storage, and every next string
					 *       value makes do without an allocation
					 *
					 * \~
					 */
					string _content;
					/**
					 * \~russian
					 * Хранилище собираемого полного имени ключа разбираемой пары
					 *
					 * @note Держится полем по той же причине, что и хранилище содержимого:
					 *       очистка сохраняет за ним занятую память, и всякая следующая пара
					 *       обходится без выделения
					 *
					 * \~english
					 * Storage of the full name of the key of the pair being parsed
					 * @note It is kept as a member for the same reason as the storage of the content:
					 *       the clearing preserves the occupied memory behind it, and every next pair
					 *       makes do without an allocation
					 *
					 * \~
					 */
					string _name;
				private:
					// Очередь собранных событий разбираемой записи
					vector <item_t> _items;
					// Составные части имён ключей разбираемой записи
					vector <segment_t> _segments;
					// Номер выдаваемого события в очереди
					size_t _current;
				private:
					// Составные части имени ключа текущего события
					vector <part_t> _path;
					// Значение текущего события разбора
					value_t _value;
					// Примечание текущего события разбора
					comment_t _comment;
				private:
					// Имя текущей таблицы текста настроек
					string _table;
					/**
					 * \~russian
					 * Указатель родов уже объявленных имён
					 *
					 * @details Правила описания об объявлении имён сводятся к роду уже
					 * объявленного, и держать их порознь перечнями значило бы проверять одно и
					 * то же имя в нескольких местах: указатель этот сводит проверку к одному
					 * обращению
					 *
					 * @note Заполняется лишь при включённой проверке повторов: в прочих случаях
					 * удерживать имена незачем, а на большом тексте настроек указатель этот
					 * памяти стоит
					 *
					 * \~english
					 * Index of the kinds of the already declared names
					 * @details The rules of the specification about the declaration of the names come down to the kind of what has already been
					 * declared, and to keep them apart as lists would mean to check one and
					 * the same name in several places: this index reduces the check to a single
					 * call
					 * @note Filled in only when the check of the repetitions is enabled: in the other cases
					 * there is no point in holding the names, while on a large settings text this index
					 * costs memory
					 *
					 * \~
					 */
					unordered_map <string_view, kind_t> _names;
					/**
					 * \~russian
					 * Хранилище имён объявленных ключей и таблиц
					 *
					 * @details Имена хранятся блоками, а перечень объявленного держит
					 * ссылки на них: имя, хранимое собственной последовательностью знаков,
					 * обходилось выделением памяти на каждый объявленный ключ - на файле
					 * настроек в шестнадцать мегабайт их набирался миллион
					 *
					 * @note Хранилище взято двусторонней очередью намеренно: перечень
					 * блоков при дописывании нового не перемещает прежние, и ссылки на их
					 * содержимое остаются годными
					 *
					 * \~english
					 * Storage of the names of the declared keys and tables
					 * @details The names are stored in blocks, while the list of what has been declared keeps
					 * the references to them: a name stored as a sequence of characters of its own
					 * cost an allocation of the memory for every declared key — on a settings
					 * file of sixteen megabytes a million of them accumulated
					 * @note The storage has been taken as a deque deliberately: the list
					 * of the blocks does not move the previous ones at the appending of a new one, and the references to their
					 * content remain valid
					 *
					 * \~
					 */
					deque <string> _blocks;
					/**
					 * \~russian
					 * Имена, объявляемые разбираемой записью
					 *
					 * @details Запись, которой не хватило продолжения текста, откатывается
					 * к своему началу и разбирается заново - и всё, что она успела
					 * объявить, объявляется вторично. Имена поэтому копятся здесь и
					 * переносятся в перечень объявленного лишь по окончании разбора записи
					 *
					 * @note Договор независимости выдачи от нарезки текста нарушается ровно
					 * так: разбор, оставляющий последствия прежде окончания записи, при
					 * подаче кусками отвергает то, что при подаче целиком принимает
					 *
					 * \~english
					 * Names declared by the record being parsed
					 * @details A record that has lacked a continuation of the text is rolled back
					 * to its beginning and is parsed anew — and everything it has managed to
					 * declare is declared a second time. The names are therefore accumulated here and
					 * are transferred into the list of what has been declared only upon the end of the parsing of the record
					 * @note The contract of the independence of the output from the cutting of the text is violated exactly
					 * that way: a parsing that leaves consequences before the end of a record, at
					 * a feeding by chunks rejects what it accepts at a feeding in full
					 *
					 * \~
					 */
					deque <pending_t> _pending;
					/**
					 * \~russian
					 * Указатель имён, объявляемых разбираемой записью
					 *
					 * @details Всякое объявляемое имя сличается со всеми, объявленными записью
					 * прежде, и перебор их обращал бы разбор в квадратичный: встроенная таблица из
					 * восьми тысяч ключей разбиралась перебором семьдесят миллисекунд
					 *
					 * @note Заводится указатель лишь по превышении порога числа имён: обычная запись
					 * объявляет их считанные единицы, и указатель стоил бы ей выделения памяти на
					 * всякое объявленное имя - вдвое против самого учёта
					 *
					 * @note Указатель держит ссылки на имена очереди, а очередь взята двусторонней
					 * намеренно: дописывание в неё прежних имён не перемещает, и ссылки остаются
					 * годными
					 *
					 * \~english
					 * Index of the names declared by the record being parsed
					 * @details Every name being declared is compared with all the ones declared by the record
					 * before, and a traversal of them would turn the parsing into a quadratic one: an inline table of
					 * eight thousand keys was parsed by a traversal for seventy milliseconds
					 * @note The index is created only upon an excess of the threshold of the number of the names: an ordinary record
					 * declares a mere few of them, and the index would cost it an allocation of the memory for
					 * every declared name — twice as much as the accounting itself
					 * @note The index keeps the references to the names of the deque, while the deque has been taken as a double-ended one
					 * deliberately: an appending into it does not move the previous names, and the references remain
					 * valid
					 *
					 * \~
					 */
					unordered_map <string_view, uint32_t> _staged;
					/**
					 * \~russian
					 * Количество имён, объявляемых разбираемой записью
					 *
					 * @note Очередь имён между записями не освобождается, а лишь укорачивается
					 * этим счётом: места её достаются следующей записи вместе с уже занятой под
					 * имена памятью, и выделение приходится на всякое место лишь единожды.
					 * Длина самой очереди означает число заведённых мест, а не число имён
					 *
					 * \~english
					 * Number of the names declared by the record being parsed
					 * @note The deque of the names is not released between the records but is only shortened
					 * by this count: its places go to the next record together with the memory already occupied
					 * by the names, and an allocation falls on every place only once.
					 * The length of the deque itself means the number of the created places rather than the number of the names
					 *
					 * \~
					 */
					size_t _staging;
					// Имя таблицы, объявляемой разбираемой записью
					string _pendingTable;
					// Признак того, что разбираемой записью объявляется таблица
					bool _declaring;
					// Признак того, что разбираемой записью объявляется набор таблиц
					bool _appending;
				private:
					/**
					 * \~russian
					 * @brief Метод запоминания ошибки разбора вместе с местом её обнаружения
					 *
					 * @param error  код ошибки разбора
					 * @param offset положение обнаружения ошибки в приведённом тексте
					 * @return       признак отказа для выхода из разбора
					 *
					 * \~english
					 * @brief Method of remembering a parsing error together with the place of its detection
					 * @param error  error code of the parsing
					 * @param offset position of the detection of the error in the converted text
					 * @return       flag of a refusal for exiting the parsing
					 *
					 * \~
					 */
					bool failure(const error_t error, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод перемещения разбора к заданному положению текста
					 *
					 * @details Перемещение ведёт счёт строк: знаки конца строки считаются
					 * при проходе через них, а не поиском назад по накопленному тексту
					 *
					 * @param offset положение, к которому перемещается разбор
					 *
					 * \~english
					 * @brief Method of moving the parsing to a given position of the text
					 * @details The move keeps the count of the lines: the line ending characters are counted
					 * at the pass through them rather than by a search backwards over the accumulated text
					 * @param offset position to which the parsing is moved
					 *
					 * \~
					 */
					void step(const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения знака приведённого текста
					 *
					 * @param offset положение знака в приведённом тексте
					 * @return       положение знака в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the position of a character of the converted text
					 * @param offset position of the character in the converted text
					 * @return       position of the character in the source text
					 *
					 * \~
					 */
					location_t locate(const size_t offset) const noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления последовательности знаков в хранилище записи
					 *
					 * @param text добавляемая последовательность знаков
					 * @return     указание на добавленную последовательность
					 *
					 * \~english
					 * @brief Method of adding a sequence of characters into the storage of a record
					 * @param text sequence of characters being added
					 * @return     pointer to the added sequence
					 *
					 * \~
					 */
					span_t keep(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения последовательности знаков хранилища записи
					 *
					 * @param span указание на последовательность знаков
					 * @return     последовательность знаков хранилища
					 *
					 * \~english
					 * @brief Method of getting a sequence of characters of the storage of a record
					 * @param span pointer to the sequence of characters
					 * @return     sequence of characters of the storage
					 *
					 * \~
					 */
					string_view get(const span_t & span) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод разбора очередной записи текста настроек
					 *
					 * @details Запись разбирается целиком либо не разбирается вовсе:
					 * незавершённая запись оставляется до прихода следующего куска
					 *
					 * @param end признак того, что текст поступил целиком
					 * @return    признак того, что запись разобрана
					 *
					 * \~english
					 * @brief Method of parsing the next record of a settings text
					 * @details A record is parsed in full or is not parsed at all:
					 * an unfinished record is left until the arrival of the next chunk
					 * @param end flag of the text having arrived in full
					 * @return    flag of the record having been parsed
					 *
					 * \~
					 */
					bool record(const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора объявления таблицы
					 *
					 * @param end признак того, что текст поступил целиком
					 * @return    признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a table declaration
					 * @param end flag of the text having arrived in full
					 * @return    flag of a successful parsing
					 *
					 * \~
					 */
					bool table(const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора пары имени ключа и значения
					 *
					 * @param end признак того, что текст поступил целиком
					 * @return    признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a pair of the name of a key and a value
					 * @param end flag of the text having arrived in full
					 * @return    flag of a successful parsing
					 *
					 * \~
					 */
					bool pair(const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора имени ключа
					 *
					 * @param end   признак того, что текст поступил целиком
					 * @param count количество разобранных составных частей имени
					 * @return      признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing the name of a key
					 * @param end   flag of the text having arrived in full
					 * @param count number of the parsed component parts of the name
					 * @return      flag of a successful parsing
					 *
					 * \~
					 */
					bool name(const bool end, uint32_t & count) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора значения
					 *
					 * @param end    признак того, что текст поступил целиком
					 * @param depth  текущая глубина вложенности значения
					 * @param prefix полное имя пары значения, пустой указатель - имени нет
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a value
					 * @param end    flag of the text having arrived in full
					 * @param depth  current depth of the nesting of the value
					 * @param prefix full name of the pair of the value, an empty pointer — there is no name
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool content(const bool end, const uint32_t depth, const string * prefix) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора перечня значений
					 *
					 * @param end   признак того, что текст поступил целиком
					 * @param depth текущая глубина вложенности значения
					 * @return      признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing an array of the values
					 * @param end   flag of the text having arrived in full
					 * @param depth current depth of the nesting of the value
					 * @return      flag of a successful parsing
					 *
					 * \~
					 */
					bool array(const bool end, const uint32_t depth) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора встроенной таблицы
					 *
					 * @param end    признак того, что текст поступил целиком
					 * @param depth  текущая глубина вложенности значения
					 * @param prefix полное имя встроенной таблицы, пустой указатель - имени нет
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing an inline table
					 * @param end    flag of the text having arrived in full
					 * @param depth  current depth of the nesting of the value
					 * @param prefix full name of the inline table, an empty pointer — there is no name
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool inlined(const bool end, const uint32_t depth, const string * prefix) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора строкового значения
					 *
					 * @param end     признак того, что текст поступил целиком
					 * @param quoting запись разобранного строкового значения
					 * @param result  указание на разобранное содержимое
					 * @return        признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a string value
					 * @param end     flag of the text having arrived in full
					 * @param quoting notation of the parsed string value
					 * @param result  pointer to the parsed content
					 * @return        flag of a successful parsing
					 *
					 * \~
					 */
					bool literal(const bool end, string_t & quoting, span_t & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора отметки времени из записи значения
					 *
					 * @param text разбираемая запись значения
					 * @param item собираемое событие значения
					 * @return     признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a timestamp from the record of a value
					 * @param text record of the value being parsed
					 * @param item event of the value being assembled
					 * @return     flag of a successful parsing
					 *
					 * \~
					 */
					bool stamped(const string_view text, item_t & item) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора числа из записи значения
					 *
					 * @param text разбираемая запись значения
					 * @param item собираемое событие значения
					 * @return     признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a number from the record of a value
					 * @param text record of the value being parsed
					 * @param item event of the value being assembled
					 * @return     flag of a successful parsing
					 *
					 * \~
					 */
					bool numeric(const string_view text, item_t & item) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора числа, логического значения либо отметки времени
					 *
					 * @param end  признак того, что текст поступил целиком
					 * @param item собираемое событие значения
					 * @return     признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a number, a logical value or a timestamp
					 * @param end  flag of the text having arrived in full
					 * @param item event of the value being assembled
					 * @return     flag of a successful parsing
					 *
					 * \~
					 */
					bool scalar(const bool end, item_t & item) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод пропуска пробельных знаков строки
					 *
					 * \~english
					 * @brief Method of skipping the whitespace characters of a line
					 *
					 * \~
					 */
					void spaces() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора остатка строки за завершённой записью
					 *
					 * @param end признак того, что текст поступил целиком
					 * @return    признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing the remainder of a line after a completed record
					 * @param end flag of the text having arrived in full
					 * @return    flag of a successful parsing
					 *
					 * \~
					 */
					bool tail(const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора примечания, начатого текущим знаком
					 *
					 * @details Разбор ведётся от знака начала примечания, на который указывает
					 * текущее положение, и до места конца содержимого его. Собранное событие
					 * уходит в очередь выдачи, если выдача примечаний настройками разрешена
					 *
					 * @param position положение конца содержимого примечания
					 * @param trailing признак примечания, дописанного к готовой строке
					 * @return         признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a comment begun by the current character
					 * @details The parsing is conducted from the character of the beginning of the comment pointed at by
					 * the current position and up to the place of the end of its content. The assembled event
					 * goes into the queue of the issuance if the issuance of the comments is permitted by the settings
					 * @param position position of the end of the content of the comment
					 * @param trailing flag of a comment appended to a ready line
					 * @return         flag of a successful parsing
					 *
					 * \~
					 */
					bool commented(const size_t position, const bool trailing) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки завершённости строки в накопленном тексте
					 *
					 * @param end признак того, что текст поступил целиком
					 * @return    признак того, что строка накоплена целиком
					 *
					 * \~english
					 * @brief Method of checking the completeness of a line in the accumulated text
					 * @param end flag of the text having arrived in full
					 * @return    flag of the line having been accumulated in full
					 *
					 * \~
					 */
					bool completed(const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки предела длины разбираемой записи
					 *
					 * @param offset положение конца разбираемой записи
					 * @return       признак того, что предел не превышен
					 *
					 * \~english
					 * @brief Method of checking the limit of the length of the record being parsed
					 * @param offset position of the end of the record being parsed
					 * @return       flag of the limit not having been exceeded
					 *
					 * \~
					 */
					bool oversize(const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи отложенного отказа приведения к кодировке UTF-8
					 *
					 * @details Отказ приведения откладывается до исчерпания уже приведённого
					 * начала текста: приведение переносит в хранилище всё, что успело
					 * проверить, и выдать отказ сразу значило бы отнять у разбора события,
					 * которые подача целиком выдаёт
					 *
					 * @note Выдача эта нужна обоим путям разбора - и подаче куска, и
					 *       переходу к следующему событию: приведённое начало текста
					 *       разбирается тем из них, которому досталось, и отказ, выдаваемый
					 *       лишь одним, при иной нарезке пропадал бы вовсе
					 *
					 * @return признак того, что отложенного отказа нет
					 *
					 * \~english
					 * @brief Method of issuing the postponed refusal of the conversion to the UTF-8 encoding
					 * @details The refusal of the conversion is postponed until the exhaustion of the already converted
					 * beginning of the text: the conversion transfers into the storage everything it has managed
					 * to check, and to issue the refusal at once would mean to take away from the parsing the events
					 * which a feeding in full issues
					 * @note This issuance is needed by both paths of the parsing — both by the feeding of a chunk and by
					 *       the transition to the next event: the converted beginning of the text
					 *       is parsed by whichever of them it has gone to, and a refusal issued
					 *       by only one of them would disappear altogether at another cutting
					 * @return flag of there being no postponed refusal
					 *
					 * \~
					 */
					bool postponed() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения рода уже объявленного имени
					 *
					 * @note Имена, объявляемые разбираемой записью, берутся наравне с объявленными
					 *       прежде: запись вправе объявить имя дважды сама
					 *
					 * @param name искомое полное имя ключа либо таблицы
					 * @return     род объявленного имени
					 *
					 * \~english
					 * @brief Method of getting the kind of an already declared name
					 * @note The names declared by the record being parsed are taken on a par with the ones declared
					 *       before: a record has the right to declare a name twice itself
					 * @param name full name of the key or of the table being sought
					 * @return     kind of the declared name
					 *
					 * \~
					 */
					kind_t lookup(const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска имени среди объявляемых разбираемой записью
					 *
					 * @param name искомое полное имя ключа либо таблицы
					 * @return     номер имени в очереди объявляемых либо количество их
					 *
					 * \~english
					 * @brief Method of searching for a name among the ones declared by the record being parsed
					 * @param name full name of the key or of the table being sought
					 * @return     number of the name in the deque of the ones being declared or their number
					 *
					 * \~
					 */
					size_t staged(const string_view name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта имени, объявляемого разбираемой записью
					 *
					 * @param name объявляемое полное имя ключа либо таблицы
					 * @param kind род объявляемого имени
					 * @param implied признак того, что имя заведено исподволь
					 *
					 * \~english
					 * @brief Method of accounting a name declared by the record being parsed
					 * @param name full name of the key or of the table being declared
					 * @param kind kind of the name being declared
					 * @param implied flag of the name having been created implicitly
					 *
					 * \~
					 */
					void stage(const string_view name, const kind_t kind, const bool implied) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки занятости объемлющих имён
					 *
					 * @details Под парой не заводится ничего, а встроенная таблица не дополняется
					 * вовсе: описание запрещает и то, и другое
					 *
					 * @param name   проверяемое полное имя ключа либо таблицы
					 * @param offset положение объявления в приведённом тексте
					 * @return       признак того, что объемлющие имена свободны
					 *
					 * \~english
					 * @brief Method of checking the occupancy of the enclosing names
					 * @details Nothing is created under a pair, while an inline table is not supplemented
					 * at all: the specification prohibits both the one and the other
					 * @param name   full name of the key or of the table being checked
					 * @param offset position of the declaration in the converted text
					 * @return       flag of the enclosing names being free
					 *
					 * \~
					 */
					bool occupied(const string & name, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки повторного объявления имени
					 *
					 * @param name   объявляемое полное имя ключа либо таблицы
					 * @param kind   род объявляемого имени
					 * @param error  код ошибки повторного объявления
					 * @param offset положение объявления в приведённом тексте
					 * @return       признак того, что имя объявляется впервые
					 *
					 * \~english
					 * @brief Method of checking a repeated declaration of a name
					 * @param name   full name of the key or of the table being declared
					 * @param kind   kind of the name being declared
					 * @param error  error code of a repeated declaration
					 * @param offset position of the declaration in the converted text
					 * @return       flag of the name being declared for the first time
					 *
					 * \~
					 */
					bool declare(const string & name, const kind_t kind, const error_t error, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса имени в хранилище объявленных имён
					 *
					 * @param name переносимое имя ключа либо таблицы
					 * @return     ссылка на имя, перенесённое в хранилище
					 *
					 * \~english
					 * @brief Method of transferring a name into the storage of the declared names
					 * @param name name of the key or of the table being transferred
					 * @return     reference to the name transferred into the storage
					 *
					 * \~
					 */
					string_view intern(const string_view name) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки полного имени из составных частей
					 *
					 * @param part   номер первой составной части имени
					 * @param parts  количество составных частей имени
					 * @param result собираемое полное имя
					 *
					 * \~english
					 * @brief Method of assembling a full name out of the component parts
					 * @param part   number of the first component part of the name
					 * @param parts  number of the component parts of the name
					 * @param result full name being assembled
					 *
					 * \~
					 */
					void assemble(const uint32_t part, const uint32_t parts, string & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия разобранного начала накопленного текста
					 *
					 * @note Без изъятия текст настроек в несколько мегабайт удерживается в
					 *       памяти целиком, хотя разбор давно ушёл вперёд
					 *
					 * \~english
					 * @brief Method of withdrawing the parsed beginning of the accumulated text
					 * @note Without the withdrawal a settings text of several megabytes is held in
					 *       the memory in full, although the parsing has long gone forward
					 *
					 * \~
					 */
					void compact() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора текста настроек
					 *
					 * @return настройки разбора текста настроек
					 *
					 * \~english
					 * @brief Method of getting the settings of the parsing of a settings text
					 * @return settings of the parsing of a settings text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора текста настроек
					 *
					 * @param settings устанавливаемые настройки разбора
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing of a settings text
					 * @param settings settings of the parsing being set
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи очередного куска исходного текста
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of feeding the next chunk of the source text
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод перехода к следующему событию разбора
					 *
					 * @return признак того, что событие получено
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 * @return flag of an event having been obtained
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
					 * @brief Метод получения состояния разбора текста настроек
					 *
					 * @return состояние разбора текста настроек
					 *
					 * \~english
					 * @brief Method of getting the state of the parsing of a settings text
					 * @return state of the parsing of a settings text
					 *
					 * \~
					 */
					state_t state() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения имени ключа текущего события
					 *
					 * @details Имя выдаётся составными частями: запись «a.b.c» даёт три
					 * части. Части ссылаются на память разбора и живут до следующего
					 * события
					 *
					 * @return составные части имени ключа текущего события
					 *
					 * \~english
					 * @brief Method of getting the name of the key of the current event
					 * @details The name is issued as component parts: the record «a.b.c» gives three
					 * parts. The parts refer to the memory of the parsing and live until the next
					 * event
					 * @return component parts of the name of the key of the current event
					 *
					 * \~
					 */
					const vector <part_t> & path() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения текущего события
					 *
					 * @return значение текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the value of the current event
					 * @return value of the current parsing event
					 *
					 * \~
					 */
					const value_t & value() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения примечания текущего события
					 *
					 * @return примечание текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the comment of the current event
					 * @return comment of the current parsing event
					 *
					 * \~
					 */
					const comment_t & comment() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места текущего события в исходном тексте
					 *
					 * @return положение начала текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the current event in the source text
					 * @return position of the beginning of the current event in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки разбора
					 *
					 * @return положение обнаружения ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of a parsing error
					 * @return position of the detection of the error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения определённой кодировки исходного текста
					 *
					 * @return определённая кодировка исходного текста
					 *
					 *
					 * \~english
					 * @brief Method of getting the determined encoding of the source text
					 * @return determined encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса разбора в исходное состояние
					 *
					 * \~english
					 * @brief Method of resetting the parsing into the initial state
					 *
					 * \~
					 */
					void clear() noexcept;
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
					 * @brief Конструктор
					 *
					 * @param settings настройки разбора текста настроек
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the parsing of a settings text
					 *
					 * \~
					 */
					explicit Reader(const settings_t & settings) noexcept;
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
					~Reader() noexcept;
			} reader_t;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_TOML_READER__
