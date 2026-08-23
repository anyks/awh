/**
 * @file common.hpp
 * @date 2026-08-17
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
 * @brief Заголовочный файл общих объявлений контейнера YAML — пределы разбора, коды
 *        отказов, виды узлов и значений, события чтения, оформление записи и положение
 *        в исходном тексте
 *
 * \~english
 * @brief Header file of the common declarations of the YAML container — the limits of the parsing, the codes
 *        of the refusals, the kinds of the nodes and of the values, the events of the reading, the formatting of the writing
 *        and the position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_COMMON__
#define __AWH_CODEC_YAML_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

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
 * @details Обход дерева и снятие событий разбора - самые горячие пути кодека: работы
 *          в них одна арифметика, а зовутся они на всякий узел либо на всякое событие.
 *          Оставленные в переводимом наборе кодека, они обращаются в вызовы через
 *          границу единиц трансляции и стоят дороже самой работы
 *
 * @note Приём этот - принятое в AWH исключение из правила о чистых заголовочных файлах:
 *       реализация живёт в `.cpp`, а для встраивания заводятся посредники с
 *       `always_inline`. Смотри `include/encoding/ascii.hpp`
 *
 * \~english
 * @brief Forced inlining of the hot accessors of the codec
 * @details The traversal of the tree and the taking of the parsing events are the hottest paths of the codec: the work
 * in them is mere arithmetic, while they are called on every node or on every event.
 * Left in the translation unit of the codec, they turn into calls across
 * the boundary of the translation units and cost more than the work itself
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
	#define AWH_YAML_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_YAML_INLINE inline __attribute__((always_inline))
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
	 * \~russian
	 * @brief Прототип класса работы с логами
	 *
	 * @details Заголовок журнала целиком заголовками кодека не подключается: он
	 * тянет за собою заголовки системные, и всякий потребитель кодека получал бы их
	 * следом. Полное описание нужно лишь исходникам, они его и подключают
	 *
	 * \~english
	 * @brief Prototype of the class for working with logs
	 * @details The header of the log is not included in whole by the headers of the codec: it
	 * drags the system headers along, and every consumer of the codec would receive them next.
	 * The complete description is needed only by the sources, and they include it
	 *
	 * \~
	 */
	class Logging;

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
		 * @brief Пространство имён контейнера YAML
		 *
		 * @details Контейнер разбирает и собирает текст по описанию YAML 1.2.2. От
		 * соседних кодеков он отличается тем, что смысл текста несёт **отступ**, а не
		 * знаки-скобки, и что одна и та же запись читается по-разному в зависимости от
		 * окружения. Устройство задано четырьмя решениями:
		 *
		 * @li **Уровни вложенности держит стопа отступов.** Закрытие уровня наступает не
		 * по знаку-закрывателю, а по приходе строки с меньшим отступом, оттого решение о
		 * закрытии откладывается до этой строки
		 * @li **Разбор не заглядывает вперёд.** Знак, чьё значение зависит от следующего
		 * за ним, переводит разбор в отдельное состояние, а не требует следующего знака
		 * немедленно. Оттого выдача не зависит от того, как текст нарезан на куски при
		 * подаче
		 * @li **Правила окончания простого значения сведены в одно тело.** Запись `a: b#c`
		 * даёт значение `b#c`, а `a: b #c` - значение `b` с примечанием; в поточном
		 * окружении правила иные, нежели в блочном. Два свода правил разошлись бы при
		 * первой же правке одного из них
		 * @li **Дерево держит отрезок исходного текста при всяком узле.** Узел, правкой не
		 * тронутый, записывается обратно **дословными исходными байтами**, а собирается
		 * заново лишь тронутый. Оттого согласие перезаписи с исходным текстом побайтово
		 * по построению, а не по старанию сборщика
		 *
		 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
		 * возвращаться к этим вопросам при разборе кода не следует:
		 *
		 * @li **Договор извлечения значений взят у контейнера JSON дословно.** Вид
		 * хранения извлечению не указ, отказ наступает лишь у не-числа, а дробное за
		 * пределами затребованного вида выдаёт предел этого вида. Два кодека, читающие
		 * одно и то же число по-разному, - это дефект, даже если каждый по себе прав
		 * @li **Нечисловая величина, извлекаемая целым видом, даёт нуль и признак успеха.**
		 * Приведение `NaN` к целому есть неопределённое поведение при любом пределе, и
		 * машины расходятся на деле: x86-64 выдаёт нижний предел, ARM64 - нуль. Правило
		 * это взято у контейнера JSON и расхождению между ними не подлежит
		 * @li **Наречие 1.2 задано умолчанием, наречие 1.1 - настройкой.** Иначе `no` в
		 * стране NO читается ложью, а `12:30` - числом 750
		 * @li **Поток ссылки не раскрывает.** Ссылка выдаётся событием, а раскрывает её
		 * дерево: раскрытие потоком отняло бы у него постоянство памяти, ради которого он
		 * и существует
		 * @li **Повторяющееся имя по умолчанию объявляется отказом.** Описание его
		 * запрещает, а реализации расходятся; прочие правила доступны настройкой
		 * @li **Глубина вложенности и раскрытие ссылок ограничены.** Разбор ведётся без
		 * рекурсии, но предел нужен и без неё: десять меток по десять ссылок раскрываются
		 * в гигабайты, и беда эта известна под именем «миллиард смешков»
		 *
		 * \~english
		 * @brief YAML container namespace
		 * @details The container parses and builds a text according to the YAML 1.2.2 specification. From
		 * the neighbouring codecs it differs in that the meaning of the text is carried by the **indentation** rather than by
		 * the bracket characters, and in that one and the same record is read differently depending on
		 * the surroundings. The structure is set by four decisions:
		 * @li **The levels of the nesting are held by a stack of the indentations.** The closing of a level comes not
		 * by a closing character but by the arrival of a line with a smaller indentation, whereby the decision about
		 * the closing is postponed until that line
		 * @li **The parsing does not look ahead.** A character whose meaning depends on the one following
		 * it transfers the parsing into a separate state rather than demanding the next character
		 * immediately. Whereby the issuance does not depend on how the text is cut into the chunks at the
		 * feeding
		 * @li **The rules of the termination of a plain scalar are gathered into one body.** The record `a: b#c`
		 * gives the value `b#c`, while `a: b #c` gives the value `b` with a comment; in the flow
		 * surroundings the rules are other than in the block one. Two sets of the rules would diverge at
		 * the very first editing of one of them
		 * @li **The tree holds a segment of the source text at every node.** A node not touched by an editing
		 * is written back by the **literal source bytes**, while only a touched one is assembled
		 * anew. Whereby the agreement of the rewriting with the source text is byte-for-byte
		 * by the construction rather than by the diligence of the assembler
		 * @details **Deliberate decisions.** What is enumerated below is chosen consciously, and
		 * one should not return to these questions at the reading of the code:
		 * @li **The contract of the extraction of the values is taken from the JSON container literally.** The kind
		 * of the storage is not a directive to the extraction, a refusal comes only at a non-number, while a fractional number beyond
		 * the limits of the demanded kind issues the limit of that kind. Two codecs reading
		 * one and the same number differently is a defect even if each of them is right by itself
		 * @li **A non-numeric value extracted by an integer kind gives a zero and the sign of a success.**
		 * The conversion of a `NaN` to an integer is an undefined behaviour at any limit, and
		 * the machines diverge in the deed: x86-64 issues the lower limit, ARM64 — a zero. This rule
		 * is taken from the JSON container and is not subject to a divergence between them
		 * @li **The 1.2 dialect is set by default, the 1.1 dialect — by a setting.** Otherwise `no` in
		 * the country NO is read as a falsehood, while `12:30` — as the number 750
		 * @li **The stream does not expand the aliases.** An alias is issued by an event, while the tree expands it:
		 * an expansion by the stream would take away from it the constancy of the memory for the sake of which it
		 * exists
		 * @li **A repeating name is declared a refusal by default.** The specification forbids
		 * it, while the implementations diverge; the other rules are available by a setting
		 * @li **The depth of the nesting and the expansion of the aliases are limited.** The parsing is conducted without
		 * a recursion, but a limit is needed even without it: ten anchors by ten aliases expand
		 * into gigabytes, and this trouble is known under the name of the «billion laughs»
		 *
		 * \~
		 */
		namespace yaml {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина скалярного значения в байтах
			 *
			 * @details Предел считается по значению уже со снятой оградой и выполненной
			 * свёрткой строк
			 *
			 * @note Записи чисел предел этот стережёт наравне с прочими: своего предела длине
			 *       числа здесь нет намеренно. Описание YAML длину записи числа не ограничивает
			 *       вовсе, и запись в тысячу разрядов законна - удерживается она дословно видом
			 *       EXTENDED. Предел, отдельно числу отведённый, отверг бы законный текст, а
			 *       отвергнув его лишь при записи, а не при чтении, ещё и потерял бы значение
			 *       при дословной перезаписи. Кодек JSON такой предел держит, и держит верно:
			 *       там длина записи числа ограничена самим описанием
			 *
			 * \~english
			 * @brief Largest admissible length of a scalar value in bytes
			 * @details The limit is counted over the value already with the quoting removed and the folding
			 * of the lines performed
			 * @note This limit guards the records of the numbers on a par with the others: a limit of its own for
			 *       the length of a number is absent here deliberately. The description of the YAML does not limit
			 *       the length of the record of a number at all, and a record of a thousand digits is lawful — it is
			 *       held verbatim by the EXTENDED kind. A limit allotted to a number separately would refuse a lawful
			 *       text, and having refused it only at the writing rather than at the reading, it would also lose
			 *       the value at the verbatim rewriting. The JSON codec holds such a limit, and holds it rightly:
			 *       there the length of the record of a number is limited by the description itself
			 *
			 * \~
			 */
			constexpr uint32_t MAX_SCALAR = 0x1000000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени метки либо ссылки в байтах
			 *
			 * \~english
			 * @brief Largest admissible length of the name of an anchor or of an alias in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_ANCHOR = 0x100;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности
			 *
			 * @details Предел этот стережёт две вещи разом. Первая - память под стопу отступов:
			 * разбор ведётся без рекурсии, но текст из одних открывающих скобок либо из одних
			 * нарастающих отступов занял бы её всю. Вторая - стек вызовов тех работ, что идут
			 * по готовому дереву возвратно: сборка текста по дереву, снятие дерева в значение
			 * владеющее и его размножение
			 *
			 * @note Замер 18.08.2026 показал, где стек кончается: скупее прочих сборка текста
			 *       по дереву документа - у NetBSD amd64 при стеке 4 МБ она валится около
			 *       11400 уровней, у macOS arm64 при 8 МБ - около 37300; снятие в значение
			 *       владеющее и размножение его вдвое-впятеро просторнее. Отсюда и предел по
			 *       умолчанию: он оставляет запаса вдесятеро на самом скупом из стендов
			 *
			 * @warning Предел этот настройкою поднимается, и поднявший выше десяти тысяч
			 *          лишается той ограды: дерево такой глубины разберётся, а сборка текста
			 *          по нему сорвёт стек. Поднимая предел, поднимать и стек потока
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting
			 * @details This limit guards two things at once. The first one is the memory for the stack of
			 * the indentations: the parsing is conducted without a recursion, but a text of opening brackets alone
			 * or of growing indentations alone would occupy all of it. The second one is the stack of the calls
			 * of those works which are conducted over the ready tree recursively: the composing of a text by the tree,
			 * the taking of the tree into an owning value and the duplication of it
			 * @note The measurement of the 18.08.2026 has shown where the stack ends: leaner than the others is
			 *       the composing of a text by the tree of a document — on the NetBSD amd64 with the stack of 4 MB
			 *       it falls at about 11400 levels, on the macOS arm64 with 8 MB — at about 37300; the taking into
			 *       an owning value and the duplication of it are two to five times more spacious. Hence the limit
			 *       by default: it leaves the tenfold reserve on the leanest of the stands
			 * @warning This limit is raised by a setting, and the one who raises it above the ten thousands deprives
			 *          themselves of that guard: a tree of such a depth will be parsed, but the composing of a text
			 *          by it will break the stack. While raising the limit, raise the stack of the thread as well
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
			 * @brief Наибольшее допустимое количество узлов, порождаемых раскрытием ссылок
			 *
			 * @details Предел этот - ответ на беду, известную под именем «миллиард смешков»:
			 * метка, ссылающаяся на метку, раскрывается произведением, и десяти уровней по
			 * десяти ссылок довольно, чтобы занять всю память машины. Текст при этом
			 * умещается в несколько строк
			 *
			 * \~english
			 * @brief Largest admissible number of the nodes generated by the expansion of the aliases
			 * @details This limit is the answer to the trouble known under the name of the «billion laughs»:
			 * an anchor referring to an anchor expands by a product, and ten levels by
			 * ten aliases are enough to occupy all the memory of a machine. The text thereby
			 * fits into a few lines
			 *
			 * \~
			 */
			constexpr uint32_t MAX_EXPANSION = 0x1000000;

			/**
			 * \~russian
			 * @brief Количество пар отображения, начиная с какого заводится указатель имён
			 *
			 * @details Ниже этого порога имя разыскивается перебором, и это дешевле всякого
			 * указателя: сличение имён идёт по памяти подряд, а заведение указателя стоит
			 * выделения памяти и подсчёта хеша на всякое имя. Выше порога перебор обращает
			 * обход отображения по именам в квадратичный, и указатель окупается
			 *
			 * @note Указатель заводится **по требованию** - при первом обращении по имени, а
			 * не при разборе. Отображение, к которому по имени не обращались, не платит
			 * ничего
			 *
			 * \~english
			 * @brief Number of the pairs of a mapping starting from which the index of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * index: the comparison of the names goes through the memory consecutively, while the creation of an index
			 * costs an allocation of the memory and a computation of a hash for every name. Above the threshold an enumeration
			 * turns the traversal of a mapping by the names into a quadratic one, and the index pays off
			 * @note The index is created **on demand** — at the first access by a name rather
			 * than at the parsing. A mapping which has not been accessed by a name pays
			 * nothing
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
			 * @brief Смещение, записи в исходном тексте не имеющее
			 *
			 * @note Знаменует оно узел, за которым записи в тексте не стоит: текст не
			 *       удержан настройкою либо узел взят раскрытием ссылки
			 *
			 * \~english
			 * @brief Offset having no record in the source text
			 *
			 * \~
			 */
			constexpr uint32_t NO_ORIGIN = static_cast <uint32_t> (~0u);

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
			 * @brief Обозначение отсутствующего указателя отступа блочного значения
			 *
			 * \~english
			 * @brief Designation of an absent indentation indicator of a block scalar
			 *
			 * \~
			 */
			constexpr uint8_t NO_INDENT = static_cast <uint8_t> (0);

			/**
			 * \~russian
			 * @brief Виды узлов документа
			 *
			 * @details Виды эти отвечают описанию: узел есть либо скалярное значение, либо
			 * перечень, либо отображение. Разделение скалярных значений по содержимому
			 * принадлежит не узлу, а виду значения
			 *
			 * @note Ссылка видом узла не является: дерево её раскрывает, и в дереве стоит
			 *       уже тот узел, на который ссылались. Событие ссылки есть лишь у потока
			 *
			 * \~english
			 * @brief Kinds of the nodes of a document
			 * @details These kinds correspond to the specification: a node is either a scalar value, or
			 * a sequence, or a mapping. The division of the scalar values by their content
			 * belongs not to the node but to the kind of the value
			 * @note An alias is not a kind of a node: the tree expands it, and in the tree there stands
			 *       already that node which was referred to. The event of an alias exists only in the stream
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE     = 0x00, // Узел не определён либо ссылка недействительна
				NUL      = 0x01, // Пустое значение null
				BOOL     = 0x02, // Логическое значение
				NUMBER   = 0x03, // Число любого вида
				STRING   = 0x04, // Строка
				BINARY   = 0x05, // Двоичное содержимое, записанное меткой !!binary
				STAMP    = 0x06, // Отметка времени, признаваемая лишь наречием 1.1
				SEQUENCE = 0x07, // Перечень значений
				MAPPING  = 0x08  // Отображение пар
			};

			/**
			 * \~russian
			 * @brief Виды значений документа
			 *
			 * @details Виды заданы разрядами, чтобы вопрос о принадлежности значения к
			 * набору видов решался одним наложением разрядов: точный вопрос (`INT32`) и
			 * сборный (`NUMBER`) стоят ровно одинаково
			 *
			 * @note Вид хранения извлечению не указ: `INT8` извлекается и как `double`, и
			 *       как `uint64_t`. Извлечение сличает с пределами затребованного вида само
			 *       значение, а не вид его хранения
			 *
			 * @note Число, не вместимое ни в один родной вид, - целое свыше `2^64` либо
			 *       запись точнее `double` - получает вид `EXTENDED` и хранится записью.
			 *       Точность такого числа сохраняется целиком, а извлечение разбирает запись
			 *
			 * @warning Перечень этот обязан совпадать с перечнем контейнера JSON во всём,
			 *          что есть у обоих: расхождение означало бы, что одно и то же число
			 *          читается двумя кодеками по-разному
			 *
			 * \~english
			 * @brief Kinds of the values of a document
			 * @details The kinds are set by the bits so that the question of the belonging of a value to
			 * a set of the kinds is decided by one overlaying of the bits: an exact question (`INT32`) and
			 * a composite one (`NUMBER`) cost exactly the same
			 * @note The kind of the storage is not a directive to the extraction: an `INT8` is extracted both as a `double` and
			 *       as a `uint64_t`. The extraction compares with the limits of the demanded kind the value
			 *       itself rather than the kind of its storage
			 * @note A number not containable in any native kind — an integer above `2^64` or
			 *       a record more precise than a `double` — receives the kind `EXTENDED` and is stored as a record.
			 *       The precision of such a number is preserved completely, while the extraction parses the record
			 * @warning This list must coincide with the list of the JSON container in everything
			 *          that both of them have: a divergence would mean that one and the same number
			 *          is read by the two codecs differently
			 *
			 * \~
			 */
			enum class type_t : uint32_t {
				UNDEFINED = 0x00000000, // Значения нет вовсе: ссылка недействительна
				NUL       = 0x00000001, // Пустое значение null
				BOOL      = 0x00000002, // Логическое значение
				STRING    = 0x00000004, // Строка
				SEQUENCE  = 0x00000008, // Перечень значений
				MAPPING   = 0x00000010, // Отображение пар
				INT8      = 0x00000020, // Целое со знаком шириною в один байт
				INT16     = 0x00000040, // Целое со знаком шириною в два байта
				INT32     = 0x00000080, // Целое со знаком шириною в четыре байта
				INT64     = 0x00000100, // Целое со знаком шириною в восемь байтов
				UINT8     = 0x00000200, // Целое без знака шириною в один байт
				UINT16    = 0x00000400, // Целое без знака шириною в два байта
				UINT32    = 0x00000800, // Целое без знака шириною в четыре байта
				UINT64    = 0x00001000, // Целое без знака шириною в восемь байтов
				FLOAT     = 0x00002000, // Дробное, точно представимое одинарной точностью
				DOUBLE    = 0x00004000, // Дробное двойной точности
				EXTENDED  = 0x00008000, // Число, не вместимое ни в один родной вид
				BINARY    = 0x00010000, // Двоичное содержимое, записанное меткой !!binary
				STAMP     = 0x00020000, // Отметка времени, признаваемая лишь наречием 1.1
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
			 * @brief Схемы разрешения видов скалярных значений
			 *
			 * @details Схема решает, чем окажется запись, оградою не обнесённая: `0777` есть
			 * число 511 по наречию 1.1 и строка по наречию 1.2, а `12:30` - число 750 по
			 * первому и строка по второму
			 *
			 * @note Схема действует лишь при разборе: вид значения разрешается однажды, а
			 *       извлечение его только преобразует. Смена схемы после разбора ничего не
			 *       переразрешает - иначе одно и то же дерево отвечало бы на один вопрос
			 *       по-разному в зависимости от того, когда спросили
			 *
			 * \~english
			 * @brief Schemas of the resolution of the kinds of the scalar values
			 * @details A schema decides what a record not surrounded by a quoting turns out to be: `0777` is
			 * the number 511 by the 1.1 dialect and a string by the 1.2 dialect, while `12:30` — the number 750 by
			 * the first one and a string by the second one
			 * @note A schema acts only at the parsing: the kind of a value is resolved once, while
			 *       its extraction only converts it. A change of the schema after the parsing re-resolves
			 *       nothing — otherwise one and the same tree would answer one question
			 *       differently depending on when it was asked
			 *
			 * \~
			 */
			enum class schema_t : uint8_t {
				FAILSAFE = 0x00, // Всякое скалярное значение есть строка
				JSON     = 0x01, // Разрешение строго по правилам JSON
				CORE     = 0x02, // Ядровая схема наречия 1.2, правило по умолчанию
				LEGACY   = 0x03  // Схема наречия 1.1: yes и no, восьмеричные с ведущим нулём, шестидесятиричные, отметки времени
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
				INTERNAL              = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF        = 0x02, // Текст оборвался посреди значения
				INVALID_CHARACTER     = 0x03, // Знак недопустим в этом месте текста
				INVALID_ENCODING      = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING  = 0x05, // Объявленная кодировка не поддерживается
				INVALID_INDENTATION   = 0x06, // Отступ не отвечает ни одному из открытых уровней
				TAB_IN_INDENTATION    = 0x07, // Отступ содержит знак горизонтальной подачи, описанием запрещённый
				UNTERMINATED_SCALAR   = 0x08, // Скалярное значение не закрыто оградой до конца текста
				INVALID_ESCAPE        = 0x09, // Отменяющая последовательность не опознана
				INVALID_UNICODE       = 0x0A, // Запись знака Юникода содержит недопустимые знаки
				UNPAIRED_SURROGATE    = 0x0B, // Суррогат не образует пары
				INVALID_BLOCK_HEADER  = 0x0C, // Заголовок блочного значения построен ошибочно
				INVALID_NUMBER        = 0x0D, // Запись числа не отвечает действующей схеме
				NUMBER_OUT_OF_RANGE   = 0x0E, // Число не представимо затребованным видом
				INVALID_BINARY        = 0x0F, // Содержимое метки !!binary не отвечает записи base64
				INVALID_STAMP         = 0x10, // Ошибочное построение отметки времени
				EXPECTED_VALUE        = 0x11, // Ожидалось значение
				EXPECTED_KEY          = 0x12, // Ожидалось имя пары отображения
				EXPECTED_COLON        = 0x13, // Ожидалось двоеточие после имени пары
				EXPECTED_COMMA        = 0x14, // Ожидалась запятая либо закрывающая скобка поточного построения
				UNCLOSED_FLOW         = 0x15, // Поточное построение не закрыто скобкой
				MIXED_COLLECTION      = 0x16, // Перечень и отображение смешаны на одном уровне
				DUPLICATE_KEY         = 0x17, // Имя пары отображения объявлено повторно
				COMPLEX_KEY           = 0x18, // Составное имя пары при запрещённых составных именах
				UNKNOWN_ALIAS         = 0x19, // Ссылка указывает на метку, ещё не объявленную
				DUPLICATE_ANCHOR      = 0x1A, // Метка с таким именем уже объявлена при строгом разборе
				RECURSIVE_ALIAS       = 0x1B, // Ссылка указывает сама на себя через цепочку меток
				EXPANSION_EXCEEDED    = 0x1C, // Раскрытие ссылок порождает больше узлов, чем дозволено
				INVALID_TAG           = 0x1D, // Метка типа построена ошибочно
				UNKNOWN_TAG_HANDLE    = 0x1E, // Сокращение метки типа не объявлено директивой %TAG
				TAG_MISMATCH          = 0x1F, // Содержимое не отвечает виду, заданному меткой типа
				INVALID_DIRECTIVE     = 0x20, // Директива построена ошибочно
				UNSUPPORTED_VERSION   = 0x21, // Наречие, объявленное директивой %YAML, не поддерживается
				UNEXPECTED_DOCUMENT   = 0x22, // Начало нового документа посреди значения
				TRAILING_CHARACTERS   = 0x23, // Знаки за окончанием документа
				DEPTH_EXCEEDED        = 0x24, // Глубина вложенности превышает допустимую
				SCALAR_TOO_LONG       = 0x25, // Длина скалярного значения превышает допустимую
				NUMBER_TOO_LONG       = 0x26, // Длина записи числа превышает допустимую
				ANCHOR_TOO_LONG       = 0x27, // Длина имени метки превышает допустимую
				TOO_MANY_NODES        = 0x28, // Количество узлов документа превышает допустимое
				EMPTY_TEXT            = 0x29, // Текст пуст, а документ затребован
				OVERFLOW_LIMIT        = 0x2A, // Превышен предел, заданный настройками разбора
				CONFLICTING_SETTINGS  = 0x2B  // Настройки записи противоречат толкованию читающего
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста
			 *
			 * @details Названия событий взяты в точности из эталонного набора
			 * `yaml-test-suite`: он печатает ожидаемый исход разбора рядом событий, и
			 * совпадение имён обращает сличение с ним в сличение строка в строку, без
			 * переводчика между моделями
			 *
			 * @note Ссылка выдаётся событием, а не раскрывается: раскрытие есть дело дерева
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a text
			 * @details The names of the events are taken exactly from the reference set
			 * `yaml-test-suite`: it prints the expected outcome of a parsing as a series of the events, and
			 * the coincidence of the names turns the comparison with it into a comparison line by line, without
			 * a translator between the models
			 * @note An alias is issued by an event rather than expanded: the expansion is the business of the tree
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE            = 0x00, // Событие не определено
				STREAM_START    = 0x01, // Начало потока документов
				STREAM_END      = 0x02, // Конец потока документов
				DOCUMENT_START  = 0x03, // Начало очередного документа
				DOCUMENT_END    = 0x04, // Конец очередного документа
				MAPPING_START   = 0x05, // Начало отображения пар
				MAPPING_END     = 0x06, // Конец отображения пар
				SEQUENCE_START  = 0x07, // Начало перечня значений
				SEQUENCE_END    = 0x08, // Конец перечня значений
				SCALAR          = 0x09, // Скалярное значение
				ALIAS           = 0x0A, // Ссылка на объявленную метку
				COMMENT         = 0x0B, // Примечание, выдаётся лишь при затребованной выдаче
				BLANK           = 0x0C, // Пустая строка, выдаётся лишь при затребованной выдаче
				FINISH          = 0x0D  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста
			 *
			 * @details Описание предписывает **UTF-8**, **UTF-16** и **UTF-32** и дозволяет
			 * метку порядка байтов в начале потока. Кодировка опознаётся по метке, а при её
			 * отсутствии - по расположению нулевых байтов в первых четырёх октетах
			 *
			 * \~english
			 * @brief Encodings of the source text
			 * @details The specification prescribes **UTF-8**, **UTF-16** and **UTF-32** and permits
			 * a byte order mark at the beginning of a stream. The encoding is recognised by the mark, while at its
			 * absence — by the arrangement of the zero bytes in the first four octets
			 *
			 * \~
			 */
			enum class encoding_t : uint8_t {
				NONE    = 0x00, // Кодировка не определена, определяется по метке порядка байтов
				UTF8    = 0x01, // Кодировка UTF-8
				UTF16LE = 0x02, // Кодировка UTF-16 с обратным порядком байтов
				UTF16BE = 0x03, // Кодировка UTF-16 с прямым порядком байтов
				UTF32LE = 0x04, // Кодировка UTF-32 с обратным порядком байтов
				UTF32BE = 0x05  // Кодировка UTF-32 с прямым порядком байтов
			};

			/**
			 * \~russian
			 * @brief Правила обращения с повторяющимся именем пары отображения
			 *
			 * @details Описание повторяющиеся имена запрещает, однако реализации расходятся:
			 * одни берут первое значение, другие последнее, третьи отвергают текст
			 *
			 * @warning Перечень этот и порядок его членов совпадают с перечнем контейнера
			 *          JSON намеренно
			 *
			 * \~english
			 * @brief Rules of the handling of a repeating name of a pair of a mapping
			 * @details The specification forbids the repeating names, however the implementations diverge:
			 * some take the first value, others the last one, the third ones reject the text
			 * @warning This list and the order of its members coincide with the list of the JSON
			 *          container deliberately
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
			 * @details Правило задаёт лишь то, как поступать с числом, не вместимым ни в один
			 * родной вид, - целым свыше `2^64` либо записью точнее `double`
			 *
			 * \~english
			 * @brief Rules of the conversion of the numbers at the parsing
			 * @details The rule sets only how to handle a number not containable in any
			 * native kind — an integer above `2^64` or a record more precise than a `double`
			 *
			 * \~
			 */
			enum class number_t : uint8_t {
				NATIVE = 0x00, // Число, не вместимое ни в один родной вид, хранится записью
				CHECK  = 0x01  // Число, не вместимое ни в один родной вид, есть отказ разбора
			};

			/**
			 * \~russian
			 * @brief Виды записи скалярного значения
			 *
			 * @details Вид записи хранится при узле не ради красоты, а ради перезаписи:
			 * значение, записанное человеком дословной оградой, обязано такою и остаться,
			 * иначе правка одного ключа переписала бы весь файл
			 *
			 * \~english
			 * @brief Kinds of the notation of a scalar value
			 * @details The kind of the notation is held at a node not for the sake of the beauty but for the sake of the rewriting:
			 * a value written by a person with a literal quoting must remain such,
			 * otherwise an editing of one key would rewrite the whole file
			 *
			 * \~
			 */
			enum class style_t : uint8_t {
				PLAIN   = 0x00, // Значение записано без ограды
				SINGLE  = 0x01, // Значение обнесено одинарной оградой
				DOUBLE  = 0x02, // Значение обнесено двойной оградой, отменяющие последовательности признаются
				LITERAL = 0x03, // Блочное значение с сохранением переводов строк
				FOLDED  = 0x04  // Блочное значение со свёрткой строк пробелом
			};

			/**
			 * \~russian
			 * @brief Правила усечения переводов строк в конце блочного значения
			 *
			 * \~english
			 * @brief Rules of the chomping of the line breaks at the end of a block scalar
			 *
			 * \~
			 */
			enum class chomp_t : uint8_t {
				CLIP  = 0x00, // Оставляется один перевод строки, правило по умолчанию
				STRIP = 0x01, // Переводы строк снимаются все
				KEEP  = 0x02  // Переводы строк сохраняются все
			};

			/**
			 * \~russian
			 * @brief Виды построения перечня либо отображения
			 *
			 * \~english
			 * @brief Kinds of the construction of a sequence or of a mapping
			 *
			 * \~
			 */
			enum class layout_t : uint8_t {
				BLOCK = 0x00, // Построение отступом
				FLOW  = 0x01  // Построение скобками, как в JSON
			};

			/**
			 * \~russian
			 * @brief Правила обращения с негодной последовательностью UTF-8 при записи
			 *
			 * @details Чтение байты, кодировке не отвечающие, отвергает - того требует
			 * описание. Запись же их пропускала, и кодек выдавал текст, какой сам же
			 * прочитать не мог. Случай этот не выдуманный: значения, взятые из журналов и
			 * сетевых сообщений, битые байты несут обычным делом
			 *
			 * @details Правилом по умолчанию взята замена, а не отказ. Отказ записи
			 * обрывает **весь** текст, а не одно значение, и один битый байт в одном
			 * сообщении обнулил бы целый документ. Знак замены же предписан самим
			 * Юникодом и в выданном тексте виден глазом, а не проглатывается молча
			 *
			 * @note Правило это решено владельцем одинаковым у всех кодеков
			 *
			 * \~english
			 * @brief Rules of the treatment of a malformed UTF-8 sequence at the writing
			 * @details The reading rejects the bytes not corresponding to the encoding — the specification
			 * requires that. The writing, however, passed them through, and the codec gave away a text which it itself
			 * could not read. This case is not invented: the values taken from the logs and
			 * the network messages carry the broken bytes as a usual matter
			 * @details The replacement rather than the refusal is taken as the default rule. A refusal of the writing
			 * cuts off the **whole** text rather than a single value, and one broken byte in one
			 * message would nullify an entire document. The replacement character, on the other hand, is prescribed by Unicode itself
			 * and is visible to the eye in the given text rather than being swallowed silently
			 * @note This rule is decided by the owner to be identical for all the codecs
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
			 * \~english
			 * @brief Segment in the storage of the characters
			 * @details A segment holds an offset and a length rather than a pointer: the storage grows
			 * by an appending and may move in the memory, and a pointer after the move would become
			 * invalid
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
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а смещение -
			 *       в байтах текста, к UTF-8 приведённого и метки порядка байтов лишённого.
			 *       Для текста UTF-8 без метки смещение это есть смещение в поданных байтах;
			 *       текст с меткою мельче поданного на длину её, а текст UTF-16 либо UTF-32 -
			 *       на всю разницу двух кодировок
			 *
			 * \~english
			 * @brief Position in the source text
			 * @details Serves for indicating the place of a refusal and for binding the values to the source
			 * text
			 * @note The line number and the position in the line are counted in Unicode characters, while the offset —
			 *       in the bytes of the text brought to UTF-8 and deprived of the byte order mark
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				// Смещение от начала текста в байтах
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
			 * @brief Отметка времени
			 *
			 * @details Признаётся лишь схемою наречия 1.1: описание 1.2 отметок времени не
			 * знает вовсе, и `2001-12-14` есть там обыкновенная строка
			 *
			 * @note Построение отметки повторяет отметку контейнера TOML намеренно: разбирают
			 *       они одну и ту же запись по ISO 8601, и расходиться им незачем
			 *
			 * \~english
			 * @brief Timestamp
			 * @details It is recognised only by the schema of the 1.1 dialect: the 1.2 specification does not know the timestamps
			 * at all, and `2001-12-14` is an ordinary string there
			 * @note The construction of the timestamp repeats the timestamp of the TOML container deliberately: they parse
			 *       one and the same record by ISO 8601, and there is no reason for them to diverge
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stamp {
				// Год отметки времени
				int32_t year;
				// Месяц отметки времени, считая с единицы
				uint8_t month;
				// День отметки времени, считая с единицы
				uint8_t day;
				// Час отметки времени
				uint8_t hour;
				// Минута отметки времени
				uint8_t minute;
				// Секунда отметки времени
				uint8_t second;
				// Доля секунды в наносекундах
				uint32_t nanoseconds;
				// Смещение часового пояса в минутах
				int16_t offset;
				// Признак того, что часовой пояс объявлен буквою Z
				bool zulu;
				// Признак того, что дата отделена от времени пробелом, а не буквою T
				bool spaced;
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
				Stamp() noexcept :
				 year(0), month(0), day(0), hour(0), minute(0), second(0),
				 nanoseconds(0), offset(0), zulu(false), spaced(false) {}
			} stamp_t;

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
			 * @brief Функция получения названия события чтения
			 *
			 * @details Названия совпадают с названиями эталонного набора `yaml-test-suite`, и
			 * сличение с ним ведётся по ним же
			 *
			 * @param event вид события чтения
			 * @return      название события чтения
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of an event of the reading
			 * @details The names coincide with the names of the reference set `yaml-test-suite`, and
			 * the comparison with it is conducted by them
			 * @param event kind of an event of the reading
			 * @return name of the event of the reading
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const event_t event) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения вида узла по виду значения
			 *
			 * @details Вид узла есть огрубление вида значения до словаря самого описания:
			 * всякое число, каким бы видом оно ни хранилось, есть узел вида `NUMBER`
			 *
			 * @param type вид значения документа
			 * @return     вид узла документа
			 *
			 * \~english
			 * @brief Function of the obtaining of the kind of a node by the kind of a value
			 * @details The kind of a node is a coarsening of the kind of a value down to the vocabulary of the specification itself:
			 * every number, by whichever kind it is stored, is a node of the kind `NUMBER`
			 * @param type kind of a value of a document
			 * @return kind of a node of a document
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ kind_t kind(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция разрешения вида скалярного значения по его записи
			 *
			 * @details Разрешение ведётся действующей схемой: одна и та же запись есть число
			 * по наречию 1.1 и строка по наречию 1.2. Значение, обнесённое оградой, схеме не
			 * подлежит вовсе - оно строка всегда
			 *
			 * @note Число выдаётся **сборным** видом `NUMBER`: точная ширина его решается
			 *       преобразованием записи, а не видом её, и определяется при разборе - тем
			 *       же порядком, что и у контейнера JSON, самым узким из вмещающих видов
			 *
			 * @param text   разрешаемая запись значения
			 * @param schema действующая схема разрешения
			 * @return       вид значения, отвечающий записи
			 *
			 * \~english
			 * @brief Function of the resolution of the kind of a scalar value by its notation
			 * @details The resolution is conducted by the acting schema: one and the same record is a number
			 * by the 1.1 dialect and a string by the 1.2 dialect. A value surrounded by a quoting is not subject to a schema
			 * at all — it is always a string
			 * @param text record of a value being resolved
			 * @param schema acting schema of the resolution
			 * @return kind of the value corresponding to the record
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ type_t resolve(const string_view text, const schema_t schema) noexcept;
			/**
			 * \~russian
			 * @brief Число, разобранное из записи своей
			 *
			 * @details Разобранное кладётся во все три поля разом, а какое из них взято -
			 * сказывает вид числа: целое со знаком лежит в первом, целое без знака во втором,
			 * дробное в третьем
			 *
			 * \~english
			 * @brief Number parsed from its record
			 * @details What is parsed is placed into all the three fields at once, and which of them is taken
			 * is told by the kind of the number
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Numeric {
				// Целое число со знаком
				int64_t integer;
				// Целое число без знака
				uint64_t natural;
				// Дробное число двойной точности
				double real;
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
				Numeric() noexcept : integer(0), natural(0), real(0.) {}
			} numeric_t;
			/**
			 * \~russian
			 * @brief Функция разбора записи числа к самому узкому вмещающему виду
			 *
			 * @details Разбор ведётся действующей схемой: `0777` есть 511 по наречию 1.1 и
			 * строка по наречию 1.2, а `12:30` есть 750 там же. Знак подчёркивания между
			 * разрядами наречие 1.1 дозволяет, и он снимается прежде разбора
			 *
			 * @note Потоковое чтение число не разбирает вовсе и выдаёт сборный вид `NUMBER`:
			 *       разбирать всякое число, никем не затребованное, значило бы платить за то,
			 *       чего потребитель не просил. Разбирает его дерево при постройке своей -
			 *       там число уже удерживается, и разобрать его нужно однажды
			 *
			 * @param text   разбираемая запись числа
			 * @param schema действующая схема разрешения
			 * @param result разобранное число
			 * @return       вид разобранного числа, `UNDEFINED` - запись числом не является
			 *
			 * \~english
			 * @brief Function of the parsing of a record of a number to the narrowest containing kind
			 * @details The parsing is conducted by the acting schema: `0777` is 511 by the 1.1 dialect and
			 * a string by the 1.2 dialect, while `12:30` is 750 there as well
			 * @note The streaming reading does not parse a number at all and issues the composite kind `NUMBER`:
			 *       to parse every number not demanded by anyone would mean to pay for what the consumer did not ask for
			 * @param text record of a number being parsed
			 * @param schema acting schema of the resolution
			 * @param result parsed number
			 * @return kind of the parsed number, `UNDEFINED` — the record is not a number
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ type_t narrow(const string_view text, const schema_t schema, numeric_t & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки записи имени метки либо ссылки
			 *
			 * @details Имя метки не вправе нести пробельных знаков и знаков, открывающих
			 * поточное построение: `[`, `]`, `{`, `}` и запятой
			 *
			 * @param text проверяемое имя метки
			 * @return     признак допустимости имени метки
			 *
			 * \~english
			 * @brief Function of the checking of the notation of the name of an anchor or of an alias
			 * @details The name of an anchor may not carry the whitespace characters and the characters opening
			 * a flow construction: `[`, `]`, `{`, `}` and the comma
			 * @param text name of an anchor being checked
			 * @return sign of the admissibility of the name of the anchor
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool anchored(const string_view text) noexcept;
			/**
			 * \~russian
			 * @brief Функция выбора вида записи скалярного значения
			 *
			 * @details Выбирается наименее навязчивый вид, каким значение записать возможно:
			 * значение, читаемое обратно тем же самым без ограды, ограды и не получает
			 *
			 * @warning Проверка эта - оборотная сторона разрешения видов, и вести её обязано
			 *          одно с ним правило: запись, которую чтение разрешит числом, обнести
			 *          оградою **необходимо**, иначе строка `12` вернётся числом
			 *
			 * @param text   записываемое значение
			 * @param schema действующая схема разрешения
			 * @param key    признак того, что значение записывается именем пары
			 * @return       наименее навязчивый допустимый вид записи
			 *
			 * \~english
			 * @brief Function of the choice of the kind of the notation of a scalar value
			 * @details The least obtrusive kind by which the value can be written is chosen:
			 * a value read back as the very same one without a quoting does not receive a quoting either
			 * @warning This check is the reverse side of the resolution of the kinds, and one and the same rule
			 *          must conduct them both: a record which the reading will resolve as a number must **necessarily** be
			 *          surrounded by a quoting, otherwise the string `12` will come back as a number
			 * @param text value being written
			 * @param schema acting schema of the resolution
			 * @param key sign of the fact that the value is written as the name of a pair
			 * @return least obtrusive admissible kind of the notation
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ style_t quoting(const string_view text, const schema_t schema, const bool key) noexcept;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_COMMON__
