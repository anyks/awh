/**
 * @file common.hpp
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
 * @brief Заголовочный файл общих объявлений контейнера JSON — пределы разбора, коды
 *        отказов, виды узлов, события чтения и положение в исходном тексте
 *
 * \~english
 * @brief Header file of the common declarations of the JSON container — the limits of the parsing, the codes
 *        of the refusals, the kinds of the nodes, the events of the reading and the position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_JSON_COMMON__
#define __AWH_CODEC_JSON_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Принудительная подстановка горячих обращений кодека
 *
 * @details Обход дерева и снятие событий разбора - самые горячие пути кодека:
 *          `valid()`, `kind()`, `next()`, `begin()`, `origin()` и `storage()` зовутся
 *          на всякий узел либо на всякое событие, а работы в них - одна арифметика.
 *          Оставленные в переводимом наборе кодека, они обращаются в вызовы через
 *          границу единиц трансляции и стоят дороже самой работы
 *
 * @note Приём этот - принятое в AWH исключение из правила о чистых заголовочных файлах:
 *       реализация живёт в `.cpp`, а для встраивания заводятся посредники с
 *       `always_inline`. Смотри `include/encoding/ascii.hpp`
 *
 * \~english
 * @brief Forced inlining of the hot accessors of the codec
 * @details The traversal of the tree and the taking of the parsing events are the hottest paths of the codec:
 * `valid()`, `kind()`, `next()`, `begin()`, `origin()` and `storage()` are called on every node
 * or on every event, while the work in them is mere arithmetic. Left in the translation unit
 * of the codec, they turn into calls across the boundary of the translation units and cost
 * more than the work itself
 * @note This device is an accepted exception in AWH from the rule about clean header files:
 * the implementation lives in `.cpp`, while for the inlining mediators with
 * `always_inline` are made. See `include/encoding/ascii.hpp`
 *
 * \~
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_JSON_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_JSON_INLINE inline __attribute__((always_inline))
#endif

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
		 * @details Контейнер разбирает и собирает текст по RFC 8259. Устройство его
		 * задано тремя решениями, и каждое из них проверено замером, а не выбрано по
		 * привычке:
		 *
		 * @li **Узел занимает шестнадцать байтов, а дети лежат сразу за родителем.**
		 * Указаний на детей нет вовсе: переход к следующему соседу стоит одного
		 * сложения, а узел-вместилище хранит размер всего своего поддерева, отчего
		 * пропуск вложенного объекта целиком тоже стоит сложения. Обход идёт вперёд по
		 * памяти, а не прыжками по указателям
		 * @li **Знаки всех строк и имён лежат в одном хранилище.** Имя не выделяет
		 * памяти вовсе, и весь собранный документ обходится двумя выделениями вместо
		 * выделения на всякий узел
		 * @li **Разбор не заглядывает вперёд.** Знак, чьё значение зависит от
		 * следующего за ним, переводит разбор в отдельное состояние, а не требует
		 * следующего знака немедленно. Оттого выдача не зависит от того, как текст
		 * нарезан на куски при подаче
		 *
		 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
		 * возвращаться к этим вопросам при разборе кода не следует:
		 *
		 * @li **Число не преобразуется, пока его не спросят.** Разбор запоминает запись
		 * числа, а преобразование выполняется по требованию и тем видом, какой
		 * затребован. Иначе `9007199254740993` терялось бы уже при разборе, а
		 * потребитель об этом не узнал бы вовсе
		 * @li **Повторяющийся ключ по умолчанию объявляется отказом.** Стандарт его
		 * допускает, а поведение не задаёт; молчаливый выбор одного из значений
		 * означал бы потерю данных. Прочие правила доступны настройкой
		 * @li **Глубина вложенности ограничена.** Разбор ведётся без рекурсии, но предел
		 * нужен и без неё: `[[[[…` на миллион скобок иначе съедает память под стек
		 * состояний
		 * @li **Отступления от стандарта отключены по умолчанию.** Примечания, запятая
		 * в конце, `NaN` и бесконечность, одинарные кавычки — всё это в обиходе есть, и
		 * всё это включается настройкой, а не молчаливо
		 *
		 * \~english
		 * @brief JSON container namespace
		 * @details The container parses and builds a text according to RFC 8259. Its structure
		 * is set by three decisions, and each of them is verified by a measurement rather than chosen by
		 * habit:
		 * @li **A node occupies sixteen bytes, and the children lie right after the parent.**
		 * There are no pointers to the children at all: the transition to the next sibling costs one
		 * addition, while a container node holds the size of its whole subtree, whereby
		 * the skipping of a nested object as a whole also costs an addition. The traversal goes forward through
		 * the memory rather than by jumps over the pointers
		 * @li **The characters of all the strings and of the names lie in one storage.** A name does not allocate
		 * memory at all, and the whole assembled document costs two allocations instead of
		 * an allocation per every node
		 * @li **The parsing does not look ahead.** A character whose meaning depends on the
		 * one following it transfers the parsing into a separate state rather than demanding
		 * the next character immediately. Whereby the issuance does not depend on how the text
		 * is cut into the chunks at the feeding
		 * @details **Deliberate decisions.** What is enumerated below is chosen consciously, and
		 * one should not return to these questions at the reading of the code:
		 * @li **A number is not converted until it is asked for.** The parsing remembers the record
		 * of the number, and the conversion is performed on demand and by that kind which
		 * is demanded. Otherwise `9007199254740993` would be lost already at the parsing, and the
		 * consumer would not learn of it at all
		 * @li **A repeating key is declared a refusal by default.** The standard permits
		 * it but does not set the behaviour; a silent choice of one of the values
		 * would mean a loss of the data. The other rules are available by a setting
		 * @li **The depth of the nesting is limited.** The parsing is conducted without a recursion, but a limit
		 * is needed even without it: `[[[[…` on a million brackets otherwise eats up the memory for the stack
		 * of the states
		 * @li **The departures from the standard are switched off by default.** The comments, a trailing comma,
		 * `NaN` and the infinity, the single quotes — all this exists in the practice, and
		 * all this is switched on by a setting rather than silently
		 *
		 * \~
		 */
		namespace json {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина строкового значения в байтах
			 *
			 * @details Предел считается по значению уже со снятым экранированием
			 *
			 * \~english
			 * @brief Largest admissible length of a string value in bytes
			 * @details The limit is counted over the value already with the escaping removed
			 *
			 * \~
			 */
			constexpr uint32_t MAX_STRING = 0x1000000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина записи числа в байтах
			 *
			 * @details Запись числа удерживается разбором целиком, оттого и ограничивается.
			 * Числа этого довольно и для целого о тысяче разрядов, и для дробного с любым
			 * разумным порядком
			 *
			 * \~english
			 * @brief Largest admissible length of the record of a number in bytes
			 * @details The record of a number is held by the parsing in full, whereby it is limited as well.
			 * This number is enough both for an integer of a thousand digits and for a fractional one with any
			 * reasonable exponent
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NUMBER = 0x400;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности
			 *
			 * @details Предел этот стережёт не стек вызовов - разбор ведётся без рекурсии, -
			 * а память под стек состояний: текст из одних открывающих скобок иначе занял бы
			 * её всю
			 *
			 * @details Разбором предел этот стережёт не стек вызовов, а память под стек
			 * состояний. Возвратны, однако, работы ПО ГОТОВОМУ ДЕРЕВУ - размножение
			 * владеющего значения, снятие его и сличение, - и тем же пределом держатся
			 * они: заведение по пути глубже предела отвергается, а запись глубже него
			 * отвергается писателем
			 *
			 * @warning Предел этот, поднятый настройкою разбора выше десяти тысяч, стек
			 *          возвратных работ срывает: замер дал срыв размножения около 65 000
			 *          уровней при стеке 8 МБ и около 30 000 при 4 МБ. Поднимающему предел
			 *          надлежит знать, зачем он это делает
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting
			 * @details This limit guards not the stack of the calls — the parsing is conducted without a recursion —
			 * but the memory for the stack of the states: a text of opening brackets alone would otherwise occupy
			 * all of it
			 * @details Recursive, however, are the works UPON A READY TREE — the duplication of an owning value,
			 * its taking and its comparison — and they are held by the very same limit: a creation by a path deeper
			 * than the limit is refused, while a writing deeper than it is refused by the writer
			 * @warning This limit, raised by the setting of the parsing above ten thousand, overflows the stack of
			 *          the recursive works: a measurement gave an overflow of the duplication at about 65 000 levels
			 *          with a stack of 8 MB and at about 30 000 with 4 MB. Whoever raises the limit ought to know why
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 0x400;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество узлов в документе
			 *
			 * \~english
			 * @brief Largest admissible number of the nodes in a document
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NODES = 0x4000000;

			/**
			 * \~russian
			 * @brief Количество полей объекта, начиная с какого заводится отображение имён
			 *
			 * @details Ниже этого порога имя разыскивается перебором, и это дешевле всякого
			 * отображения: сличение имён идёт по памяти подряд, а заведение отображения
			 * стоит выделения памяти и подсчёта хеша на всякое имя. Выше порога перебор
			 * обращает обход объекта по именам в квадратичный, и отображение окупается
			 *
			 * @note Отображение заводится **по требованию** - при первом обращении по имени,
			 * а не при разборе. Объект, к которому по имени не обращались, не платит ничего
			 *
			 * \~english
			 * @brief Number of the fields of an object starting from which the mapping of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * mapping: the comparison of the names goes through the memory consecutively, while the creation of a mapping
			 * costs an allocation of the memory and a computation of a hash for every name. Above the threshold an enumeration
			 * turns the traversal of an object by the names into a quadratic one, and the mapping pays off
			 * @note The mapping is created **on demand** — at the first access by a name
			 * rather than at the parsing. An object which has not been accessed by a name pays nothing
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего положения в исходном тексте
			 *
			 * \~english
			 * @brief Designation of an absent position in the source text
			 *
			 * \~
			 */
			constexpr uint64_t NO_OFFSET = static_cast <uint64_t> (~0ull);

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего номера узла
			 *
			 * \~english
			 * @brief Designation of an absent number of a node
			 *
			 * \~
			 */
			constexpr uint32_t NO_INDEX = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Виды узлов документа
			 *
			 * @details Виды эти отвечают стандарту дословно: у JSON их ровно шесть, и
			 * седьмого быть не может
			 *
			 * @note Целое и дробное числа видом не разделяются намеренно: разделение это
			 * принадлежит не документу, а тому, каким видом значение затребовано. Число
			 * `1.0` целым числом выдаётся, а `1e400` не выдаётся ни тем, ни другим
			 *
			 * \~english
			 * @brief Kinds of the nodes of a document
			 * @details These kinds correspond to the standard literally: JSON has exactly six of them, and
			 * there cannot be a seventh one
			 * @note An integer and a fractional number are not divided by a kind deliberately: this division
			 * belongs not to the document but to that by which kind the value is demanded. The number
			 * `1.0` is issued as an integer, while `1e400` is issued neither as the one nor as the other
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE   = 0x00, // Узел не определён либо ссылка недействительна
				NUL    = 0x01, // Пустое значение null
				BOOL   = 0x02, // Логическое значение true либо false
				NUMBER = 0x03, // Число
				STRING = 0x04, // Строка
				ARRAY  = 0x05, // Массив
				OBJECT = 0x06  // Объект
			};

			/**
			 * \~russian
			 * @brief Виды значений документа
			 *
			 * @details Вид узла отвечает на вопрос стандарта - шесть видов и не более того,
			 * тогда как вид значения отвечает на вопрос потребителя: каким именно числом
			 * значение хранится. Разбор определяет самый узкий вмещающий вид сразу, и
			 * повторного разбора записи не требуется вовсе
			 *
			 * @details Виды заданы разрядами, а сборные виды - объединением разрядов. Оттого
			 * точный вопрос (`INT32`) и сборный (`SIGNED`, `NUMBER`) стоят ровно одинаково -
			 * одно наложение разрядов
			 *
			 * @note Вид хранения извлечению не указ: `INT8` извлекается и как `double`, и как
			 * `uint64_t`. Извлечение сличает само значение с пределами затребованного вида,
			 * а не вид хранения с видом затребованным
			 *
			 * @note Число, не вместимое ни в один родной вид - целое свыше `2^64` либо запись
			 * с точностью выше `double` - получает вид `EXTENDED` и хранится записью. Точность
			 * такого числа сохраняется полностью, а извлечение его разбирает запись
			 *
			 * \~english
			 * @brief Kinds of the values of a document
			 * @details The kind of a node answers the question of the standard — six kinds and no more,
			 * whereas the kind of a value answers the question of the consumer: by which number exactly
			 * the value is stored. The parsing determines the narrowest containing kind at once, and
			 * a repeated parsing of the record is not required at all
			 * @details The kinds are set by the bits, and the composite kinds by a union of the bits. Whereby
			 * an exact question (`INT32`) and a composite one (`SIGNED`, `NUMBER`) cost exactly the same —
			 * one overlaying of the bits
			 * @note The kind of the storage is not a directive to the extraction: `INT8` is extracted both as a `double` and as
			 * a `uint64_t`. The extraction compares the value itself with the limits of the demanded kind
			 * rather than the kind of the storage with the demanded kind
			 * @note A number not containable in any native kind — an integer above `2^64` or a record
			 * with a precision higher than `double` — receives the kind `EXTENDED` and is stored as a record. The precision
			 * of such a number is preserved completely, and its extraction parses the record
			 *
			 * \~
			 */
			enum class type_t : uint16_t {
				UNDEFINED = 0x0000, // Значения нет вовсе: ссылка недействительна
				NUL       = 0x0001, // Пустое значение null
				BOOL      = 0x0002, // Логическое значение true либо false
				STRING    = 0x0004, // Строка
				ARRAY     = 0x0008, // Массив
				OBJECT    = 0x0010, // Объект
				INT8      = 0x0020, // Целое со знаком шириною в один байт
				INT16     = 0x0040, // Целое со знаком шириною в два байта
				INT32     = 0x0080, // Целое со знаком шириною в четыре байта
				INT64     = 0x0100, // Целое со знаком шириною в восемь байтов
				UINT8     = 0x0200, // Целое без знака шириною в один байт
				UINT16    = 0x0400, // Целое без знака шириною в два байта
				UINT32    = 0x0800, // Целое без знака шириною в четыре байта
				UINT64    = 0x1000, // Целое без знака шириною в восемь байтов
				FLOAT     = 0x2000, // Дробное, точно представимое одинарной точностью
				DOUBLE    = 0x4000, // Дробное двойной точности
				EXTENDED  = 0x8000, // Число, не вместимое ни в один родной вид
				// Целое со знаком любой ширины
				SIGNED    = (INT8 | INT16 | INT32 | INT64),
				// Целое без знака любой ширины
				UNSIGNED  = (UINT8 | UINT16 | UINT32 | UINT64),
				// Целое любой ширины и любой знаковости
				INT       = (SIGNED | UNSIGNED),
				// Дробное любой точности
				REAL      = (FLOAT | DOUBLE),
				// Число любого вида
				NUMBER    = (INT | REAL | EXTENDED)
			};

			/**
			 * \~russian
			 * @brief Коды ошибок разбора текста
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе с положением в исходном тексте, где отказ произошёл
			 *
			 * \~english
			 * @brief Error codes of the parsing of a text
			 * @details The parsing does not throw exceptions: the error code together with the position
			 * in the source text where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                  = 0x00, // Ошибок не обнаружено
				/**
				 * Внутренняя ошибка разбора
				 *
				 * @note Код этот ЗАПАСНОЙ и в исправной работе не рождается вовсе: ставится
				 *       он там, где нижний слой отказал, НЕ НАЗВАВ кода. Замерено картой
				 *       рождённых кодов по всему набору - молчит он один из немногих, и
				 *       молчание это ожидаемо: случай требует изъяна в самом кодеке, оттого
				 *       проверкою он и не закреплён
				 */
				INTERNAL              = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF        = 0x02, // Текст оборвался посреди значения
				INVALID_CHARACTER     = 0x03, // Знак недопустим в этом месте текста
				INVALID_ENCODING      = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING  = 0x05, // Объявленная кодировка не поддерживается
				UNTERMINATED_STRING   = 0x06, // Строка не закрыта до конца текста
				INVALID_ESCAPE        = 0x07, // Отменяющая последовательность не опознана
				INVALID_UNICODE       = 0x08, // Запись \\uXXXX содержит недопустимые знаки
				UNPAIRED_SURROGATE    = 0x09, // Суррогат не образует пары
				CONTROL_IN_STRING     = 0x0A, // Управляющий знак внутри строки без экранирования
				INVALID_NUMBER        = 0x0B, // Запись числа не отвечает стандарту
				NUMBER_OUT_OF_RANGE   = 0x0C, // Число не представимо затребованным видом
				INVALID_LITERAL       = 0x0D, // Вместо true, false либо null стоит иное
				TRAILING_CHARACTERS   = 0x0E, // Знаки за окончанием документа
				EXPECTED_VALUE        = 0x0F, // Ожидалось значение
				EXPECTED_KEY          = 0x10, // Ожидалось имя поля объекта
				EXPECTED_COLON        = 0x11, // Ожидалось двоеточие после имени поля
				EXPECTED_COMMA        = 0x12, // Ожидалась запятая либо закрывающая скобка
				TRAILING_COMMA        = 0x13, // Запятая перед закрывающей скобкой при строгом разборе
				DUPLICATE_KEY         = 0x14, // Имя поля объекта объявлено повторно
				DEPTH_EXCEEDED        = 0x15, // Глубина вложенности превышает допустимую
				STRING_TOO_LONG       = 0x16, // Длина строкового значения превышает допустимую
				NUMBER_TOO_LONG       = 0x17, // Длина записи числа превышает допустимую
				TOO_MANY_NODES        = 0x18, // Количество узлов документа превышает допустимое
				COMMENT_NOT_ALLOWED   = 0x19, // Примечание встречено при строгом разборе
				UNTERMINATED_COMMENT  = 0x1A, // Примечание не закрыто до конца текста
				EMPTY_TEXT            = 0x1B, // Текст пуст, а документ затребован
				/**
				 * Превышен предел, заданный настройками разбора
				 *
				 * @note Код этот ныне не выдаётся НИ ОДНИМ местом кодека и оставлен под
				 *       пределы из настроек, какие заведутся впредь. Прежде им отвечало
				 *       переполнение разрядности хранилища - условие, настройками не
				 *       управляемое вовсе; оно переведено на `STORAGE_EXHAUSTED`
				 *
				 * @note Снести его нельзя: перечень открыт, и числа его входят в договор
				 */
				OVERFLOW_LIMIT        = 0x1C, // Превышен предел, заданный настройками разбора
				FILE_NOT_OPENED       = 0x1D, // Файл документа открыть не удалось
				TEXT_ALREADY_ENDED    = 0x1E, // Подача продолжена после объявленного конца текста
				FILE_NOT_WRITTEN      = 0x1F, // Текст документа записать в файл не удалось
				MULTIPLE_ROOTS        = 0x20, // Корень документа уже несёт значение
				NO_CONTAINER_OPEN     = 0x21, // Ни одно вместилище не открыто
				KEY_OUTSIDE_OBJECT    = 0x22, // Имя поля допустимо лишь внутри объекта и лишь однажды
				FILE_NOT_READ         = 0x23, // Файл документа прочитать не удалось
				/**
				 * Разбираемый текст не помещается в разрядность хранилища
				 *
				 * @note Условие это ОДНО на все кодеки, и код у него ныне один: смещение
				 *       содержимого узла отведено четырьмя октетами, и текст свыше 4 ГБ в
				 *       него не ложится. Прежде JSON отвечал на него `OVERFLOW_LIMIT` -
				 *       кодом, описанным как «превышен предел, заданный НАСТРОЙКАМИ
				 *       разбора», - и слал потребителя править настройки, которых предел
				 *       этот не слушает вовсе: он задан разрядностью, а не выбором
				 *
				 * @note Расхождение вскрыто картой рождённых кодов и сличением с соседями:
				 *       XML и CSV звали то же условие `STORAGE_EXHAUSTED` изначально, а
				 *       Николай нашёл ту же беду у TOML против INI. Свойство это общее -
				 *       одно условие обязано зваться у кодеков одинаково
				 */
				STORAGE_EXHAUSTED     = 0x24  // Разбираемый текст не помещается в разрядность хранилища
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * @note Начало и конец вместилища выдаются отдельными событиями, а не одним
			 * событием с готовым вместилищем: массив на миллион значений иначе удерживался
			 * бы в памяти целиком прежде первой выдачи, и потоковое чтение потеряло бы
			 * смысл
			 *
			 * @note Имя поля объекта выдаётся своим событием, предшествующим событию
			 * значения. Значением при этом вправе оказаться вместилище, и тогда за именем
			 * идёт начало вместилища
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a text
			 * @details The reading issues the events as the text is parsed without holding it in full
			 * @note The beginning and the end of a container are issued by separate events rather than by one
			 * event with a ready container: an array of a million values would otherwise be held
			 * in the memory in full before the first issuance, and the streaming reading would lose its
			 * sense
			 * @note The name of a field of an object is issued by its own event preceding the event
			 * of the value. The value may thereby turn out to be a container, and then the name
			 * is followed by the beginning of a container
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE          = 0x00, // Событие не определено
				KEY           = 0x01, // Имя поля объекта
				NUL           = 0x02, // Значение null
				BOOL          = 0x03, // Значение true либо false
				NUMBER        = 0x04, // Число, выдаётся записью без преобразования
				STRING        = 0x05, // Строка со снятым экранированием
				ARRAY_BEGIN   = 0x06, // Начало массива
				ARRAY_END     = 0x07, // Конец массива
				OBJECT_BEGIN  = 0x08, // Начало объекта
				OBJECT_END    = 0x09, // Конец объекта
				COMMENT       = 0x0A, // Примечание, выдаётся лишь при разрешённых примечаниях
				DOCUMENT      = 0x0B, // Документ разобран до конца, следом вправе идти новый при потоке NDJSON
				FINISH        = 0x0C  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста
			 *
			 * @details Стандарт предписывает **UTF-8** для обмена между разными системами и
			 * метку порядка байтов запрещает. Прочие кодировки распознаются потому, что в
			 * обиходе они встречаются, и текст в них лучше принять с пометкой, чем
			 * отвергнуть молчанием
			 *
			 * @note Кодировок UTF-32 среди распознаваемых нет: стандарт их для обмена не
			 * велит, а в обиходе они не встречаются вовсе
			 *
			 * \~english
			 * @brief Encodings of the source text
			 * @details The standard prescribes UTF-8 and forbids the byte order mark. The other
			 * encodings are recognised because they are encountered in the practice, and a text in them
			 * is better accepted with a note than rejected by a silence
			 *
			 * \~
			 */
			enum class encoding_t : uint8_t {
				NONE    = 0x00, // Кодировка не определена, определяется по метке порядка байтов
				UTF8    = 0x01, // Кодировка UTF-8, предписанная стандартом
				UTF16LE = 0x02, // Кодировка UTF-16 с обратным порядком байтов
				UTF16BE = 0x03, // Кодировка UTF-16 с прямым порядком байтов
				LATIN1  = 0x04, // Кодировка ISO-8859-1
				ASCII   = 0x05, // Кодировка US-ASCII
				CP1252  = 0x06  // Кодировка Windows-1252
			};

			/**
			 * \~russian
			 * @brief Правила обращения с повторяющимся именем поля объекта
			 *
			 * @details Стандарт повторяющиеся имена допускает, а поведение при них не
			 * задаёт вовсе, и реализации расходятся: одни берут первое значение, другие
			 * последнее, третьи собирают все
			 *
			 * @warning Дерево, разобранное правилом `KEEP`, записывается повторами и
			 *          обратно читается ЛИШЬ правилом `FIRST`, `LAST` либо `KEEP`:
			 *          умолчательное правило `ERROR` такой текст отвергает отказом
			 *          `DUPLICATE_KEY`. Круговой ход через умолчания на нём рвётся, и
			 *          сохраняющему повторы надлежит знать, чем их потом читать
			 *
			 * \~english
			 * @brief Rules of the handling of a repeating name of a field of an object
			 * @details The standard permits the repeating names but does not set the behaviour
			 * for them at all, and the implementations diverge: some take the first value, others
			 * the last one, the third ones collect all of them
			 * @warning A tree parsed by the rule `KEEP` is written with the repetitions and is read
			 *          back ONLY by the rule `FIRST`, `LAST` or `KEEP`: the default rule `ERROR`
			 *          rejects such a text with a `DUPLICATE_KEY` refusal. The round trip through
			 *          the defaults breaks upon it, and whoever preserves the repetitions ought to
			 *          know what will read them afterwards
			 *
			 * \~
			 */
			enum class duplicate_t : uint8_t {
				ERROR = 0x00, // Отказ разбора, правило по умолчанию
				FIRST = 0x01, // Берётся первое встреченное значение, прочие отбрасываются
				LAST  = 0x02, // Берётся последнее встреченное значение
				KEEP  = 0x03  // Удерживаются все, доступ по имени отдаёт первое
			};

			/**
			 * \~russian
			 * @brief Правила преобразования чисел при разборе
			 *
			 * @details Разбор определяет вид числа и преобразует его всегда: повторного
			 * разбора записи не бывает вовсе. Правило это задаёт лишь то, как поступать с
			 * числом, ВЕЛИЧИНА которого не вмещается ни в один родной вид: целым за пределами
			 * `int64_t` и `uint64_t` либо дробным, вышедшим за пределы `double` - как вверх,
			 * обращаясь в бесконечность, так и вниз, обращаясь в нуль
			 *
			 * @note Излишняя ТОЧНОСТЬ записи отказом не считается ни при каком правиле:
			 *       запись `0.1234567890123456789012345` величиною в `double` вмещается и
			 *       округляется до `0.12345678901234568`, как округлил бы её сам язык.
			 *       Отвергать всякую запись, точнее `double`, значило бы отвергать и `0.1`,
			 *       записанное двадцатью знаками, - то есть обычную запись обычного числа
			 *
			 * \~english
			 * @brief Rules of the conversion of the numbers at the parsing
			 * @details The parsing determines the kind of a number and converts it always: a repeated
			 * parsing of the record does not happen at all. This rule sets only how to handle a
			 * number whose MAGNITUDE is not containable in any native kind: an integer beyond the limits
			 * of `int64_t` and `uint64_t`, or a real that has left the range of a `double` — either upwards,
			 * turning into an infinity, or downwards, turning into a zero
			 * @note An excessive PRECISION of a record is not counted as a refusal under any rule:
			 *       the record `0.1234567890123456789012345` is containable in a `double` by its magnitude and
			 *       is rounded to `0.12345678901234568`, as the language itself would round it.
			 *       To reject every record more precise than a `double` would be to reject `0.1` too,
			 *       written with twenty digits — that is, an ordinary record of an ordinary number
			 *
			 * \~
			 */
			enum class number_t : uint8_t {
				NATIVE = 0x00, // Число, не вместимое ни в один родной вид, хранится записью
				CHECK  = 0x01  // Число, не вместимое ни в один родной вид, есть отказ разбора
			};

			/**
			 * \~russian
			 * @brief Виды оформления собираемого текста
			 *
			 * \~english
			 * @brief Kinds of the formatting of the text being assembled
			 *
			 * \~
			 */
			enum class format_t : uint8_t {
				COMPACT = 0x00, // Без пробелов и переводов строк
				PRETTY  = 0x01  // С отступами и переводами строк
			};

			/**
			 * \~russian
			 * @brief Правила экранирования при записи текста
			 *
			 * \~english
			 * @brief Rules of the escaping at the writing of a text
			 *
			 * \~
			 */
			enum class escape_t : uint8_t {
				MINIMAL = 0x00, // Экранируется лишь предписанное стандартом
				SOLIDUS = 0x01, // Сверх того экранируется косая черта
				ASCII   = 0x02  // Сверх того всякий знак вне US-ASCII записывается как \\uXXXX
			};

			/**
			 * \~russian
			 * @brief Правила обращения с последовательностью, кодировке не отвечающей
			 *
			 * @details Стандарт предписывает тексту кодировку UTF-8, и разбор наш байты,
			 * ей не отвечающие, отвергает. Запись их насквозь выдала бы текст, какой
			 * кодек прочитать не сможет сам, оттого правило обращения с ними и заведено
			 *
			 * @note Негодными считаются: ведущий байт, ведущим не являющийся; недостача
			 *       продолжающих байтов; продолжающий байт вне отведённого ведущему
			 *       предела; запись кодовой точки длиннее необходимого; суррогат;
			 *       кодовая точка свыше U+10FFFF
			 *
			 * \~english
			 * @brief Rules of the handling of a sequence not conforming to the encoding
			 * @details The standard prescribes the UTF-8 encoding for a text, and our parsing rejects
			 * the bytes not conforming to it. The writing of them through would give away a text which
			 * the codec itself will not be able to read, whereby the rule of the handling of them is made
			 * @note The following are considered malformed: a leading byte which is not a leading one; a shortage
			 *       of the continuing bytes; a continuing byte outside the limit allotted to the leading one;
			 *       a record of a code point longer than necessary; a surrogate;
			 *       a code point above U+10FFFF
			 *
			 * \~
			 */
			enum class malformed_t : uint8_t {
				REPLACE = 0x00, // Негодная последовательность заменяется знаком U+FFFD
				REFUSE  = 0x01, // Запись отвергается, ничего не записав
				PASS    = 0x02  // Байты пропускаются как есть
			};

			/**
			 * \~russian
			 * @brief Отрезок в хранилище знаков
			 *
			 * @details Отрезок хранит смещение и длину, а не указатель: хранилище растёт
			 * дописыванием и вправе переехать в памяти, и указатель после переезда стал бы
			 * недействителен
			 *
			 * @warning Смещение отсчитывается по ХРАНИЛИЩУ ЗНАКОВ, а не по разбираемому
			 * тексту, и месту в тексте оно НЕ РАВНО: знаки ложатся в хранилище уже
			 * раскодированными и уплотнёнными - без кавычек, обвязки и разделителей, с
			 * развёрнутыми последовательностями ухода. Ключ `имя` из записи `{"имя":1}`
			 * лежит в хранилище нулевым, а в тексте стоит вторым. Для привязки к исходному
			 * тексту служит `location_t`, объявленный ниже, - величина иная
			 * @note При СНЯТОМ удержании хранилища оно очищается по исчерпании выданных
			 * событий, и смещение отсчитывается от текущего его состояния: целое место в
			 * потоке знаков сводится через `Reader::origin()` - число выброшенных байтов.
			 * При удержании ПОСТАВЛЕННОМ хранилище растёт во весь разбираемый текст, а
			 * `origin` остаётся нулём
			 *
			 * \~english
			 * @brief Segment in the storage of the characters
			 * @details A segment holds an offset and a length rather than a pointer: the storage grows
			 * by an appending and may move in the memory, and a pointer after the move would become
			 * invalid
			 * @warning The offset is counted along the STORAGE OF THE CHARACTERS rather than along the text
			 * being parsed, and it is NOT EQUAL to the place in the text: the characters lie in the storage
			 * already decoded and compacted — without the quotes, the framing and the separators, with the
			 * escape sequences expanded. The key `имя` of the record `{"имя":1}` lies in the storage as the
			 * zeroth one, while in the text it stands as the second. For the binding to the source text there
			 * serves `location_t`, declared below — a different quantity
			 * @note With the retention of the storage REMOVED it is cleared upon the exhaustion of the issued
			 * events, and the offset is counted from its current state: the whole place in the stream of the
			 * characters is derived through `Reader::origin()` — the number of the discarded bytes. With the
			 * retention SET the storage grows to the whole text being parsed, while `origin` remains a zero
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Span {
				// Смещение начала отрезка в хранилище знаков
				uint32_t offset;
				// Длина отрезка в байтах
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
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param offset смещение начала отрезка в хранилище знаков
				 * @param length длина отрезка в байтах
				 *
				 * \~english
				 * @brief Constructor
				 * @param offset offset of the beginning of the segment in the storage of the characters
				 * @param length length of the segment in bytes
				 *
				 * \~
				 */
				Span(const uint32_t offset, const uint32_t length) noexcept : offset(offset), length(length) {}
			} span_t;

			/**
			 * \~russian
			 * @brief Положение в исходном тексте
			 *
			 * @details Служит для указания места отказа и для привязки значений к исходному
			 * тексту
			 *
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а
			 * смещение - в байтах исходного текста до перекодирования
			 *
			 * \~english
			 * @brief Position in the source text
			 * @details Serves for indicating the place of a refusal and for binding the values to the source
			 * text
			 * @note The line number and the position in the line are counted in Unicode characters, while
			 * the offset — in the bytes of the source text before the transcoding
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				/**
				 * \~russian
				 * @brief Смещение от начала текста в байтах
				 *
				 * @warning Смещение считается по тексту, ПРИВЕДЁННОМУ к UTF-8, а не по
				 *          исходным байтам. Метка порядка байтов снята, а текст в иной
				 *          кодировке уже переведён, и длины расходятся: документ Latin-1
				 *          в 52 байта даёт смещение конца 54, а метка порядка байтов
				 *          сдвигает все смещения на свою длину. Для указания места в
				 *          ИСХОДНОМ файле смещение это негодно, если текст подан не в
				 *          UTF-8 без метки
				 *
				 * \~english
				 * @brief The offset from the beginning of the text in bytes
				 *
				 * @warning The offset is counted over the text CONVERTED to UTF-8 rather than over
				 *          the source bytes. The byte order mark is stripped and a text in another
				 *          encoding is already converted, so the lengths diverge: a Latin-1 document
				 *          of 52 bytes gives the ending offset 54, and a byte order mark shifts every
				 *          offset by its length. For pointing at a place in the SOURCE file this
				 *          offset is unfit unless the text is supplied as UTF-8 without a mark
				 *
				 * \~
				 */
				uint64_t offset;
				// Номер строки, считая с единицы
				uint32_t line;
				// Положение в строке, считая с единицы
				uint32_t column;
				// Глубина вложенности, считая с нуля
				uint32_t depth;
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
				Location() noexcept : offset(NO_OFFSET), line(0), column(0), depth(0) {}
			} location_t;

			/**
			 * \~russian
			 * @brief Функция получения текстового описания кода отказа
			 *
			 * @param error код отказа разбора
			 * @return      текстовое описание кода отказа
			 *
			 * \~english
			 * @brief Function of the obtaining of the textual description of a refusal code
			 * @param error error code of the parsing
			 * @return textual description of the error code
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия вида узла
			 *
			 * @param kind вид узла документа
			 * @return     название вида узла
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of a kind of a node
			 * @param kind kind of a node of a document
			 * @return name of the kind of the node
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const kind_t kind) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия вида значения
			 *
			 * @note Название выдаётся точному виду, а не сборному: сборный вид назван быть не
			 *       может, ибо он есть множество видов, а не вид
			 *
			 * @param type вид значения документа
			 * @return     название вида значения
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of a kind of a value
			 * @note The name is issued for an exact kind rather than for a composite one: a composite kind cannot be
			 *       named, for it is a set of the kinds rather than a kind
			 * @param type kind of a value of a document
			 * @return name of the kind of the value
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия кодировки исходного текста
			 *
			 * @note Заведена наравне с кодеками разметки и таблицы: у них выдача эта была, а у
			 *       документа её не было вовсе, и опознанную кодировку звучащему нечем было
			 *       назвать - ни в журнал, ни в сообщение об отказе
			 *
			 * @param encoding кодировка исходного текста
			 * @return         название кодировки
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of an encoding of a source text
			 * @note It has been introduced on a par with the codecs of a markup and of a table: they had
			 * the present issuance, while the document did not have it at all, and the caller had nothing
			 * to name the recognized encoding with — neither into a log nor into a message about a refusal
			 * @param encoding encoding of a source text
			 * @return         name of the encoding
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const encoding_t encoding) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения вида узла по виду значения
			 *
			 * @details Вид узла есть огрубление вида значения до словаря самого стандарта:
			 * всякое число, каким бы видом оно ни хранилось, есть узел вида `NUMBER`
			 *
			 * @param type вид значения документа
			 * @return     вид узла документа
			 *
			 * \~english
			 * @brief Function of the obtaining of the kind of a node by the kind of a value
			 * @details The kind of a node is a coarsening of the kind of a value down to the vocabulary of the standard itself:
			 * every number, by whichever kind it is stored, is a node of the kind `NUMBER`
			 * @param type kind of a value of a document
			 * @return kind of a node of a document
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ kind_t kind(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки записи числа на соответствие стандарту
			 *
			 * @details Проверяется лишь запись, преобразование не выполняется. Стандарт
			 * запрещает ведущий нуль, ведущий плюс, точку без цифры после неё и порядок без
			 * цифр
			 *
			 * @param text проверяемая запись числа
			 * @return     признак соответствия записи стандарту
			 *
			 * \~english
			 * @brief Function of the checking of the record of a number for the conformity to the standard
			 * @details Only the record is checked, the conversion is not performed. The standard
			 * forbids a leading zero, a leading plus, a point without a digit after it and an exponent without
			 * the digits
			 * @param text record of a number being checked
			 * @return sign of the conformity of the record to the standard
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool numeric(const string & text) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки необходимости экранирования строки
			 *
			 * @details Строка, не требующая экранирования, записывается переносом отрезком
			 * целиком, минуя разбор по знакам
			 *
			 * @param text   проверяемое значение
			 * @param escape правило экранирования при записи
			 * @return       признак необходимости экранирования
			 *
			 * \~english
			 * @brief Function of the checking of the necessity of the escaping of a string
			 * @details A string not requiring an escaping is written by a transfer of the segment
			 * as a whole bypassing the parsing by the characters
			 * @param text value being checked
			 * @param escape rule of the escaping at the writing
			 * @return sign of the necessity of the escaping
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool escapable(const string & text, const escape_t escape) noexcept;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_JSON_COMMON__
