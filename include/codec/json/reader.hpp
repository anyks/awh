/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения текста JSON — разбор подаваемого кусками
 *        текста с выдачей событий, не удерживающей документ целиком
 *
 * \~english
 * @brief Header file of the streaming reading of a JSON text — the parsing of a text fed by the chunks
 *        with an issuance of the events not holding the document in full
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_JSON_READER__
#define __AWH_CODEC_JSON_READER__

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
#include "encoding.hpp"

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
			 * @brief Потоковое чтение текста JSON
			 *
			 * @details Текст подаётся кусками произвольного размера, а разбор выдаёт события
			 * по мере продвижения, не удерживая документ целиком. Пригодно для чтения
			 * ответа службы по мере его прихода и для разбора документов, в память не
			 * помещающихся
			 *
			 * @details **Независимость выдачи от нарезки текста.** Выдача обязана быть
			 * одинаковой, как бы текст ни был нарезан на куски. Достигается это тем, что
			 * разбор **не заглядывает вперёд**: знак, чьё значение зависит от следующего за
			 * ним, переводит разбор в отдельное состояние. Таких мест у JSON много, и
			 * каждому отведено своё состояние:
			 *
			 * @li знак отмены внутри строки - `ESCAPE`;
			 * @li запись `\\uXXXX` - четыре состояния по числу шестнадцатеричных знаков;
			 * @li **суррогатная пара** - шесть состояний: за старшим суррогатом обязан
			 * следовать младший, а между ними вправе пройти граница куска;
			 * @li запись числа - состояния по частям: знак, целая часть, дробная, порядок;
			 * @li литералы `true`, `false` и `null` - разбираются по знаку;
			 * @li косая черта, открывающая примечание, - её значение решает следующий знак
			 *
			 * @note Разбор ведётся **без рекурсии**: вложенность хранится стеком видов
			 * вместилищ. Оттого документ произвольной глубины не роняет стек вызовов, а
			 * предел глубины стережёт лишь память
			 *
			 * \~english
			 * @brief Streaming reading of a JSON text
			 * @details The text is fed by the chunks of an arbitrary size, while the parsing issues the events
			 * as it advances without holding the document in full. Suitable for the reading of
			 * a response of a service as it arrives and for the parsing of the documents not fitting
			 * into the memory
			 * @details **Independence of the issuance from the cutting of the text.** The issuance must be
			 * the same however the text has been cut into the chunks. This is achieved by the fact that
			 * the parsing **does not look ahead**: a character whose meaning depends on the one following
			 * it moves the parsing into a separate state. JSON has many such places, and
			 * each one is given its own state:
			 * @li the escape character inside a string — `ESCAPE`;
			 * @li the record `\\uXXXX` — four states by the number of the hexadecimal characters;
			 * @li a **surrogate pair** — six states: a high surrogate must be
			 * followed by a low one, and a chunk boundary may pass between them;
			 * @li the record of a number — the states by the parts: the sign, the integer part, the fractional one, the exponent;
			 * @li the literals `true`, `false` and `null` — are parsed by a character;
			 * @li a solidus opening a comment — its meaning is decided by the next character
			 * @note The parsing is conducted **without a recursion**: the nesting is held by a stack of the kinds
			 * of the containers. Whereby a document of an arbitrary depth does not crash the stack of the calls, while
			 * the limit of the depth guards only the memory
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
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
						// Кодировка, навязанная извне вопреки метке порядка байтов
						encoding_t encoding;
						/**
						 * \~russian
						 * Признак строгого следования RFC 8259
						 *
						 * @note Строгий разбор отвечает отказом на знаки за окончанием
						 * документа и на метку порядка байтов в начале текста - стандарт
						 * её запрещает. Обиход этого не соблюдает, потому умолчанием
						 * разбор нестрогий
						 *
						 * \~english
						 * Flag of the strict following of RFC 8259
						 * @note The strict parsing answers with a refusal to the characters after the end of
						 * the document and to the byte order mark at the beginning of the text — the standard
						 * forbids it. The custom does not observe this, therefore by default
						 * the parsing is a non-strict one
						 *
						 * \~
						 */
						bool strict;
						/**
						 * \~russian
						 * Признак разрешения примечаний
						 *
						 * @note Стандарт примечаний не знает вовсе, а обиход настроек их
						 * требует: файлы настроек с примечаниями зовут JSONC
						 *
						 * \~english
						 * Flag of the permission of the comments
						 * @note The standard does not know the comments at all, while the custom of the settings
						 * demands them: the files of the settings with the comments are called JSONC
						 *
						 * \~
						 */
						bool allowComments;
						// Признак разрешения запятой перед закрывающей скобкой
						bool allowTrailingCommas;
						// Признак разрешения записей NaN, Infinity и -Infinity
						bool allowInfinityAndNan;
						// Признак разрешения строк в одинарных кавычках
						bool allowSingleQuotes;
						/**
						 * \~russian
						 * Признак разбора потока документов NDJSON
						 *
						 * @note Поток этот - последовательность документов, разделённых
						 * переводом строки. В обиходе служб он повсеместен, а стандартом
						 * JSON не описан вовсе
						 *
						 * \~english
						 * Flag of the parsing of an NDJSON stream of the documents
						 * @note This stream is a sequence of the documents separated by
						 * a line feed. In the practice of the services it is ubiquitous, while by the standard
						 * of JSON it is not described at all
						 *
						 * \~
						 */
						bool stream;
						// Признак выдачи событий примечаний
						bool emitComments;
						// Наибольшая допустимая длина строкового значения в байтах, ноль - без предела
						uint32_t maxString;
						// Наибольшая допустимая длина записи числа в байтах, ноль - без предела
						uint32_t maxNumber;
						// Наибольшая допустимая глубина вложенности, ноль - без предела
						uint32_t maxDepth;
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
					 * @details Значение ссылается на память, принадлежащую разбору, и живёт
					 * не дольше следующего события
					 *
					 * \~english
					 * @brief Value issued by an event of the parsing
					 * @details The value refers to the memory belonging to the parsing and lives
					 * no longer than the next event
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Value {
						/**
						 * \~russian
						 * Содержимое значения
						 *
						 * @note У строки и у имени поля - со снятым экранированием, у
						 * числа - записью как она есть, у логического значения - словами
						 * `true` либо `false`
						 *
						 * \~english
						 * Content of the value
						 * @note For a string and for the name of a field — with the escaping removed, for
						 * a number — by the record as it is, for a logical value — by the words
						 * `true` or `false`
						 *
						 * \~
						 */
						string_view text;
						// Вид значения
						kind_t kind;
						// Признак того, что содержимое было изменено разбором
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
						Value() noexcept : kind(kind_t::NONE), modified(false) {}
					} value_t;
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
						DOCUMENT_START  = 0x00, // Начало документа, значение ещё не начато
						VALUE_START     = 0x01, // Ожидается значение
						KEY_START       = 0x02, // Ожидается имя поля объекта либо закрывающая скобка
						AFTER_KEY       = 0x03, // Имя поля прочитано, ожидается двоеточие
						AFTER_VALUE     = 0x04, // Значение прочитано, ожидается запятая либо закрывающая скобка
						AFTER_COMMA     = 0x05, // Запятая прочитана, ожидается значение либо имя поля
						STRING          = 0x06, // Внутри строки
						ESCAPE          = 0x07, // Знак отмены внутри строки, значение решает следующий знак
						UNICODE_1       = 0x08, // Первый шестнадцатеричный знак записи \\uXXXX
						UNICODE_2       = 0x09, // Второй шестнадцатеричный знак записи \\uXXXX
						UNICODE_3       = 0x0A, // Третий шестнадцатеричный знак записи \\uXXXX
						UNICODE_4       = 0x0B, // Четвёртый шестнадцатеричный знак записи \\uXXXX
						SURROGATE_SLASH = 0x0C, // Прочитан старший суррогат, ожидается косая черта младшего
						SURROGATE_U     = 0x0D, // Ожидается буква u младшего суррогата
						SURROGATE_1     = 0x0E, // Первый шестнадцатеричный знак младшего суррогата
						SURROGATE_2     = 0x0F, // Второй шестнадцатеричный знак младшего суррогата
						SURROGATE_3     = 0x10, // Третий шестнадцатеричный знак младшего суррогата
						SURROGATE_4     = 0x11, // Четвёртый шестнадцатеричный знак младшего суррогата
						NUMBER_MINUS    = 0x12, // Прочитан знак минуса, ожидается цифра
						NUMBER_ZERO     = 0x13, // Прочитан ведущий нуль, цифра за ним запрещена стандартом
						NUMBER_INTEGER  = 0x14, // Внутри целой части числа
						NUMBER_POINT    = 0x15, // Прочитана точка, ожидается цифра дробной части
						NUMBER_FRACTION = 0x16, // Внутри дробной части числа
						NUMBER_EXPONENT = 0x17, // Прочитана буква порядка, ожидается знак либо цифра
						NUMBER_SIGN     = 0x18, // Прочитан знак порядка, ожидается цифра
						NUMBER_POWER    = 0x19, // Внутри цифр порядка
						LITERAL         = 0x1A, // Внутри литерала true, false, null, NaN либо Infinity
						SLASH           = 0x1B, // Прочитана косая черта, вид примечания решает следующий знак
						COMMENT_LINE    = 0x1C, // Внутри примечания до конца строки
						COMMENT_BLOCK   = 0x1D, // Внутри примечания, закрываемого звёздочкой с косой чертой
						COMMENT_STAR    = 0x1E, // Звёздочка внутри примечания, закрытие решает следующий знак
						DOCUMENT_END    = 0x1F, // Документ разобран, знаки за ним разрешены лишь потоку NDJSON
						FAILED          = 0x20  // Разбор прекращён отказом
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
						// Указание на содержимое события в хранилище знаков
						span_t content;
						// Положение события в исходном тексте
						location_t location;
						// Признак того, что содержимое было изменено разбором
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
						Item() noexcept : event(event_t::NONE), modified(false) {}
					} item_t;
				public:
					/**
					 * \~russian
					 * @brief Обработчик прямой выдачи событий разбора
					 *
					 * @details Вызывается на всяком собранном событии, минуя очередь выдачи.
					 * Вид события, указание на содержимое его и признак изменения содержимого
					 * передаются доводами, а не снимаются с чтения: снятие их обошлось бы
					 * вызовами через границу единиц трансляции на всякое событие
					 *
					 * @warning Обработчик подаёт события посреди разбора куска текста: подавать
					 * из него текст тому же чтению нельзя. Прекращение разбора требуется
					 * запрашивать вызовом `abort()`
					 *
					 * \~english
					 * @brief Handler of the direct issuance of the parsing events
					 * @details It is called on every assembled event, bypassing the queue of the issuance.
					 * The kind of the event, the pointer at its content and the flag of the modification of the content
					 * are passed as arguments rather than taken from the reader: taking them would cost
					 * calls across the boundary of the translation units on every event
					 * @warning The handler delivers the events in the middle of the parsing of a chunk of a text: feeding
					 * a text to the same reader from it is not allowed. The termination of the parsing is required
					 * to be requested by a call of `abort()`
					 *
					 * \~
					 */
					typedef void (* handler_t) (void * context, Reader & reader, const event_t event, const span_t content, const bool modified);
				private:
					// Настройки разбора текста
					settings_t _settings;
				private:
					// Состояние разбора текста
					state_t _state;
					// Код отказа разбора
					error_t _error;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Умолчание стоит прямо в объявлении: конструкторы, логгера не
					 *       принимающие, оставили бы поле неопределённым
					 *
					 * \~english
					 * Object of the keeping of the work log
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					// Признак того, что подан последний кусок текста
					bool _last;
					// Признак того, что разбираемая строка является именем поля объекта
					bool _keyed;
					// Признак того, что содержимое было изменено разбором
					bool _modified;
					// Признак того, что вместилище ещё не получило ни одного значения
					bool _empty;
					// Признак того, что прочитана запятая, а значение за ней ещё не начато
					bool _comma;
				private:
					/**
					 * \~russian
					 * Хранилище знаков собираемого значения
					 *
					 * @note Хранилище переиспользуется между значениями: выдача события
					 * возвращает его к началу, а память остаётся выделенной
					 *
					 * \~english
					 * Storage of the characters of the value being assembled
					 * @note The storage is reused between the values: the issuance of an event
					 * returns it to the beginning, while the memory remains allocated
					 *
					 * \~
					 */
					string _storage;
				private:
					/**
					 * \~russian
					 * Количество байтов, выброшенных из хранилища знаков за всё время разбора
					 *
					 * @details Хранилище очищается всякий раз, когда очередь событий выдана,
					 * и смещения в нём начинаются заново. Счёт выброшенного делает смещения
					 * сквозными: положение содержимого события в потоке разобранных знаков
					 * есть сумма счёта и смещения события в хранилище
					 *
					 * @note Счёт этот заведён ради потребителя, дерево собирающего: без
					 * сквозного положения ему пришлось бы переносить знаки всякого значения
					 * к себе по одному, а с ним хранилище переносится целыми кусками
					 *
					 * \~english
					 * Number of the bytes discarded from the storage of the characters over the whole time of the parsing
					 * @details The storage is cleared every time the queue of the events is issued,
					 * and the offsets in it begin anew. The counting of the discarded makes the offsets
					 * through-going: the position of the content of an event in the stream of the parsed characters
					 * is the sum of the counting and of the offset of the event in the storage
					 * @note This counting is created for the sake of the consumer assembling a tree: without
					 * a through-going position it would have to transfer the characters of every value
					 * to itself one by one, while with it the storage is transferred by whole blocks
					 *
					 * \~
					 */
					uint64_t _origin;
					// Остаток куска, не составивший целого знака кодировки
					string _pending;
					// Текст куска, приведённый к UTF-8
					string _decoded;
				private:
					/**
					 * \~russian
					 * Стек видов вместилищ, задающий вложенность
					 *
					 * @note Стек этот заменяет рекурсию: вид вместилища решает, ожидается
					 * ли следующим имя поля либо значение
					 *
					 * \~english
					 * Stack of the kinds of the containers setting the nesting
					 * @note This stack replaces the recursion: the kind of a container decides whether
					 * the name of a field or a value is expected next
					 *
					 * \~
					 */
					vector <kind_t> _nesting;
				private:
					/**
					 * \~russian
					 * Очередь собранных событий
					 *
					 * @note Очередь заведена перечнем с указанием на голову, а не двусторонней
					 * очередью: та отводит по блоку памяти на каждые несколько десятков
					 * событий и освобождает его по опустошении, отчего разбор крупного
					 * документа стоил бы десятков тысяч выделений памяти
					 *
					 * \~english
					 * Queue of the assembled events
					 * @note The queue is made as a list with a pointer to the head rather than as a deque:
					 * the latter allocates a block of the memory per every few dozens
					 * of the events and frees it upon the emptying, whereby the parsing of a large
					 * document would cost tens of thousands of the allocations of the memory
					 *
					 * \~
					 */
					vector <item_t> _items;
					// Указание на голову очереди собранных событий
					size_t _head;
					// Текущее выданное событие
					item_t _current;
				private:
					/**
					 * Обработчик прямой выдачи событий разбора
					 *
					 * @note Очередь выдачи стоит на пути события к потребителю и обходится
					 *       обработчиком: событие ложится в очередь лишь тогда, когда
					 *       обработчик не установлен
					 */
					handler_t _handler;
					// Указание, передаваемое обработчику прямой выдачи событий
					void * _context;
					/**
					 * Признак прекращения разбора по требованию потребителя
					 *
					 * @note Признак этот отдельный от состояния разбора намеренно: событие
					 *       выдаётся посреди разбора знака, и состояние по возвращении из
					 *       обработчика переписывается разбором того же знака
					 */
					bool _stopped;
					/**
					 * Признак удержания хранилища знаков
					 *
					 * @note Удержание затребует тот, кто забирает знаки себе целиком: очистка
					 *       хранилища по исчерпании событий заставляла бы его переносить знаки
					 *       копией на всяком куске
					 */
					bool _keeping;
				private:
					// Положение текущего события в исходном тексте
					location_t _position;
					// Смещение от начала текста в байтах
					uint64_t _offset;
					// Номер строки, считая с единицы
					uint32_t _line;
					// Положение в строке, считая с единицы
					uint32_t _column;
					// Длина собираемого значения в байтах
					uint32_t _length;
					// Собираемый знак Юникода записи \\uXXXX
					uint32_t _unicode;
					// Старший суррогат, ожидающий младшего
					uint32_t _surrogate;
					// Количество разобранных знаков литерала
					uint32_t _matched;
				private:
					// Разбираемый литерал
					const char * _literal;
					// Вид значения, каким завершится разбор литерала
					event_t _outcome;
				private:
					// Знак кавычек, каким открыта разбираемая строка
					char _quote;
					// Состояние, к какому разбор вернётся по окончании примечания
					state_t _resume;
					// Положение начала собираемого значения в исходном тексте
					location_t _mark;
				private:
					// Средство приведения исходного текста к кодировке UTF-8
					Decoder _decoder;
				private:
					/**
					 * \~russian
					 * Разметка знаков, прерывающих быстрый проход внутри строки
					 *
					 * @details Знак, в разметке не отмеченный, содержимым строки и является:
					 * такие знаки проходятся отрезком и переносятся в хранилище одним
					 * действием. Договор о независимости выдачи от нарезки текста проход
					 * этот не задевает: знаки, чьё значение зависит от следующего за ними,
					 * разметкой отмечены и проходом не берутся
					 *
					 * \~english
					 * Marking of the characters interrupting the fast pass inside a string
					 * @details A character not marked in the marking is the content of the string itself:
					 * such characters are passed by a segment and transferred into the storage by one
					 * action. The contract of the independence of the issuance from the cutting of the text does not
					 * affect this pass: the characters whose meaning depends on the one following them
					 * are marked by the marking and are not taken by the pass
					 *
					 * \~
					 */
					bool _breakString[256];
				private:
					/**
					 * Слова с образцами знаков, прерывающих быстрый проход по строке
					 *
					 * @details Быстрый проход идёт по восьми байтам разом, и знаки, его
					 * прерывающие, разыскиваются в слове целочисленными действиями, а не
					 * обращением к разметке на всякий байт. Во всяком байте образца лежит
					 * один и тот же разыскиваемый знак
					 *
					 * @note Второй образец занят знаком одинарных кавычек, а при запрете их
					 *       настройками повторяет первый: повторная проверка ничего не портит,
					 *       а ветвление внутри прохода обошлось бы дороже
					 */
					uint64_t _breakWord[2];
				private:
					/**
					 * \~russian
					 * @brief Метод заполнения разметки знаков, прерывающих быстрый проход
					 *
					 *
					 * \~english
					 * @brief Method of the filling of the marking of the characters interrupting the fast pass
					 *
					 * \~
					 */
					void marking() noexcept;
					/**
					 * \~russian
					 * @brief Метод быстрого прохода по знакам, состояния не меняющим
					 *
					 * @param buffer буфер разбираемого текста
					 * @param size   размер буфера разбираемого текста
					 * @return       количество пройденных байтов
					 *
					 * \~english
					 * @brief Method of the fast pass over the characters not changing the state
					 * @param buffer buffer of the text being parsed
					 * @param size size of the buffer of the text being parsed
					 * @return number of the passed bytes
					 *
					 * \~
					 */
					size_t bulk(const char * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора приведённого к UTF-8 текста
					 *
					 * @param text буфер разбираемого текста
					 * @param size размер буфера разбираемого текста
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a text brought to UTF-8
					 * @param text buffer of the text being parsed
					 * @param size size of the buffer of the text being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool process(const char * text, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора одного знака текста
					 *
					 * @param letter разбираемый знак
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of one character of a text
					 * @param letter character being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const char letter) noexcept;
					/**
					 * \~russian
					 * @brief Метод прекращения разбора отказом
					 *
					 * @param error код отказа разбора
					 * @return      признак успешности разбора, всегда ложь
					 *
					 * \~english
					 * @brief Method of the stopping of the parsing by a refusal
					 * @param error error code of the parsing
					 * @return sign of the success of the parsing, always a falsehood
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки собранного события в очередь выдачи
					 *
					 * @param event  вид собранного события
					 * @param offset смещение содержимого события в хранилище знаков
					 * @param length длина содержимого события в байтах
					 *
					 * \~english
					 * @brief Method of the placing of an assembled event into the queue of the issuance
					 * @param event kind of the assembled event
					 * @param offset offset of the content of the event in the storage of the characters
					 * @param length length of the content of the event in bytes
					 *
					 * \~
					 */
					void emit(const event_t event, const uint32_t offset, const uint32_t length) noexcept;
					/**
					 * \~russian
					 * @brief Метод перехода к состоянию за окончанием значения
					 *
					 * @return признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the transition to the state after the end of a value
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool settle() noexcept;
					/**
					 * \~russian
					 * @brief Метод начала разбора значения
					 *
					 * @param letter знак, начинающий значение
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the beginning of the parsing of a value
					 * @param letter character beginning the value
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool begins(const char letter) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора знака записи числа
					 *
					 * @param letter разбираемый знак
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a character of the record of a number
					 * @param letter character being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool number(const char letter) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния разбора
					 *
					 * @details Сброс возвращает разбор к началу текста, а выделенную память
					 * удерживает: следующий разбор не платит за её заведение заново
					 *
					 * \~english
					 * @brief Method of the reset of the state of the parsing
					 * @details The reset returns the parsing to the beginning of the text while holding
					 * the allocated memory: the next parsing does not pay for its creation anew
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи куска разбираемого текста
					 *
					 * @param buffer буфер подаваемого текста
					 * @param size   размер буфера подаваемого текста
					 * @param last   признак того, что кусок последний
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the feeding of a chunk of a text being parsed
					 * @param buffer buffer of the text being fed
					 * @param size size of the buffer of the text being fed
					 * @param last flag that the chunk is the last one
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool feed(const char * buffer, const size_t size, const bool last = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи текста целиком
					 *
					 * @param text подаваемый текст
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the feeding of a text as a whole
					 * @param text text being fed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
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
					 * @brief Метод установки обработчика прямой выдачи событий разбора
					 *
					 * @details Событие подаётся обработчику прямо из разбора, минуя очередь
					 * выдачи. Потоковое чтение событиями очередью при этом сохраняется:
					 * обработчик - дело добровольное, и без него разбор ведёт себя по-прежнему
					 *
					 * @note Очередь стоила трети всего времени сборки дерева: событие ложится
					 * в неё, а потом снимается с неё же копией
					 *
					 * @param callback устанавливаемый обработчик, ноль - снятие обработчика
					 * @param context  указание, передаваемое обработчику
					 *
					 * \~english
					 * @brief Method of setting the handler of the direct issuance of the parsing events
					 * @details The event is delivered to the handler straight from the parsing, bypassing the queue
					 * of the issuance. The streaming reading by the events through the queue is thereby preserved:
					 * the handler is a voluntary matter, and without it the parsing behaves as before
					 * @note The queue cost a third of the whole time of the assembly of the tree: the event is put
					 * into it, and then is taken from it by a copy
					 * @param callback handler being set, zero — removal of the handler
					 * @param context  pointer passed to the handler
					 *
					 * \~
					 */
					void handler(handler_t callback, void * context) noexcept;
					/**
					 * \~russian
					 * @brief Метод прекращения разбора по требованию потребителя
					 *
					 * @details Прекращает разбор прямо посреди куска текста. Применяется
					 * обработчиком прямой выдачи событий: вернуть отказ из него нечем, а подача
					 * текста обязана прекратиться немедля
					 *
					 * @note Кода отказа не устанавливает: причина прекращения известна тому,
					 * кто его затребовал, и своей причины у разбора здесь нет
					 *
					 * \~english
					 * @brief Method of the termination of the parsing at the demand of the consumer
					 * @details It terminates the parsing right in the middle of a chunk of a text. It is applied
					 * by the handler of the direct issuance of the events: there is nothing to return a failure by from it, and the feeding
					 * of the text must stop immediately
					 * @note It sets no error code: the reason of the termination is known to the one
					 * who demanded it, and the parsing has no reason of its own here
					 *
					 * \~
					 */
					void abort() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида текущего события
					 *
					 * @return вид текущего события
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the current event
					 * @return kind of the current event
					 *
					 * \~
					 */
					AWH_JSON_INLINE event_t event() const noexcept {
						// Выводим вид текущего события
						return this->_current.event;
					}
					/**
					 * \~russian
					 * @brief Метод извлечения значения текущего события
					 *
					 * @return значение текущего события
					 *
					 * \~english
					 * @brief Method of the extraction of the value of the current event
					 * @return value of the current event
					 *
					 * \~
					 */
					value_t value() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения указания на содержимое текущего события
					 *
					 * @details Указание отдаётся смещением и длиной в хранилище знаков разбора,
					 * а не готовым содержимым: потребителю, собирающему дерево, оно позволяет
					 * переносить хранилище целыми кусками вместо переноса всякого значения
					 * по одному
					 *
					 * @return указание на содержимое текущего события
					 *
					 * \~english
					 * @brief Method of the extraction of the pointer to the content of the current event
					 * @details The pointer is given away by an offset and a length in the storage of the characters of the parsing
					 * rather than by a ready content: for the consumer assembling a tree it allows
					 * transferring the storage by whole blocks instead of the transfer of every value
					 * one by one
					 * @return pointer to the content of the current event
					 *
					 * \~
					 */
					AWH_JSON_INLINE span_t content() const noexcept {
						// Выводим указание на содержимое текущего события
						return this->_current.content;
					}
					/**
					 * \~russian
					 * @brief Метод извлечения хранилища знаков разбора
					 *
					 * @return хранилище знаков разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the storage of the characters of the parsing
					 * @return storage of the characters of the parsing
					 *
					 * \~
					 */
					AWH_JSON_INLINE const string & storage() const noexcept {
						// Выводим хранилище знаков разбора
						return this->_storage;
					}
					/**
					 * \~russian
					 * @brief Метод установки удержания хранилища знаков
					 *
					 * @details Хранилище знаков очищается по исчерпании выданных событий: без
					 * того оно росло бы во весь разбираемый текст. Потребителю, забирающему
					 * знаки себе целиком, очистка эта не нужна вовсе - она лишь заставляет его
					 * переносить знаки копией на всяком куске
					 *
					 * @warning Удержание растит хранилище во весь разбираемый текст: потоковому
					 * разбору, разбирающему текст без конца, оно не годится
					 *
					 * @param mode устанавливаемый признак удержания хранилища знаков
					 *
					 * \~english
					 * @brief Method of setting the retention of the storage of the characters
					 * @details The storage of the characters is cleared upon the exhaustion of the issued events: without
					 * that it would grow to the whole text being parsed. To a consumer taking the characters
					 * for itself as a whole that clearing is not needed at all — it merely forces it
					 * to transfer the characters by a copy on every chunk
					 * @warning The retention grows the storage to the whole text being parsed: to a streaming
					 * parsing, parsing a text without an end, it is not suitable
					 * @param mode flag of the retention of the storage of the characters being set
					 *
					 * \~
					 */
					void keep(const bool mode) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи хранилища знаков наружу
					 *
					 * @details Хранилище переносится потребителю целиком, без копии. Применяется
					 * по окончании разбора тем, кто удержание затребовал: копия хранилища во весь
					 * разбираемый текст - самая дорогая из статей сборки дерева
					 *
					 * @warning Хранилище разбора после выдачи пусто, а выданные события ссылаются
					 * в него смещениями: звать его посреди разбора нельзя
					 *
					 * @param storage хранилище, куда переносятся знаки разбора
					 *
					 * \~english
					 * @brief Method of the issuance of the storage of the characters outwards
					 * @details The storage is transferred to the consumer as a whole, without a copy. It is applied
					 * upon the end of the parsing by the one who demanded the retention: a copy of the storage of the whole
					 * text being parsed is the most expensive of the items of the assembly of the tree
					 * @warning The storage of the parsing after the issuance is empty, while the issued events refer
					 * into it by the offsets: calling it in the middle of the parsing is not allowed
					 * @param storage storage into which the characters of the parsing are transferred
					 *
					 * \~
					 */
					void release(string & storage) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества байтов, выброшенных из хранилища знаков
					 *
					 * @return количество байтов, выброшенных из хранилища знаков
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the bytes discarded from the storage of the characters
					 * @return number of the bytes discarded from the storage of the characters
					 *
					 * \~
					 */
					AWH_JSON_INLINE uint64_t origin() const noexcept {
						// Выводим количество байтов, выброшенных из хранилища знаков
						return this->_origin;
					}
					/**
					 * \~russian
					 * @brief Метод извлечения положения текущего события в исходном тексте
					 *
					 * @return положение текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of the extraction of the position of the current event in the source text
					 * @return position of the current event in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
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
					 * @brief Метод извлечения кодировки исходного текста
					 *
					 * @return кодировка исходного текста
					 *
					 * \~english
					 * @brief Method of the extraction of the encoding of the source text
					 * @return encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
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
					 * @brief Метод извлечения настроек разбора текста
					 *
					 * @return настройки разбора текста
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the parsing of a text
					 * @return settings of the parsing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора текста
					 *
					 * @param settings устанавливаемые настройки разбора текста
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a text
					 * @param settings settings of the parsing of a text being set
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
					explicit Reader(const log_t * log) noexcept;
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
					~Reader() noexcept {}
			} reader_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_JSON_READER__
