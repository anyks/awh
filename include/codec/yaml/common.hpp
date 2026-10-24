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
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../value.hpp"
#include "../../sys/macro/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Принудительная подстановка горячих обращений кодека
 *
 * @details Обход дерева и снятие событий разбора - самые горячие пути кодека: работы
 *          в них одна арифметика, а зовутся они на всякий узел либо на всякое событие.
 *          Оставленные в переводимом наборе кодека, они обращаются в вызовы через
 *          границу единиц трансляции и стоят дороже самой работы.
 *
 * @note Приём этот - принятое в AWH исключение из правила о чистых заголовочных файлах:
 *       реализация живёт в `.cpp`, а для встраивания заводятся посредники с
 *       `always_inline`. Смотри `include/encoding/ascii.hpp`
 *
 * \~english
 * @brief Forced inlining of the hot accessors of the codec
 *
 * @details The traversal of the tree and the taking of the parsing events are the hottest paths of the codec: the work
 *          in them is mere arithmetic, while they are called on every node or on every event.
 *          Left in the translation unit of the codec, they turn into calls across
 *          the boundary of the translation units and cost more than the work itself.
 *
 * @note This device is an accepted exception in AWH from the rule about clean header files:
 *       the implementation lives in `.cpp`, while for the inlining mediators with
 *       `always_inline` are made. See `include/encoding/ascii.hpp`
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
		 *          соседних кодеков он отличается тем, что смысл текста несёт **отступ**, а не
		 *          знаки-скобки, и что одна и та же запись читается по-разному в зависимости от
		 *          окружения. Устройство задано четырьмя решениями:
		 *
		 * @li **Уровни вложенности держит стопа отступов.** Закрытие уровня наступает не
		 *     по знаку-закрывателю, а по приходе строки с меньшим отступом, оттого решение о
		 *     закрытии откладывается до этой строки
		 *
		 * @li **Разбор не заглядывает вперёд.** Знак, чьё значение зависит от следующего
		 *     за ним, переводит разбор в отдельное состояние, а не требует следующего знака
		 *     немедленно. Оттого выдача не зависит от того, как текст нарезан на куски при
		 *     подаче
		 *
		 * @li **Правила окончания простого значения сведены в одно тело.** Запись `a: b#c`
		 *     даёт значение `b#c`, а `a: b #c` - значение `b` с примечанием; в поточном
		 *     окружении правила иные, нежели в блочном. Два свода правил разошлись бы при
		 *     первой же правке одного из них
		 *
		 * @li **Дерево держит отрезок исходного текста при всяком узле.** Узел, правкой не
		 *     тронутый, записывается обратно **дословными исходными байтами**, а собирается
		 *     заново лишь тронутый. Оттого согласие перезаписи с исходным текстом побайтово
		 *     по построению, а не по старанию сборщика
		 *
		 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
		 *          возвращаться к этим вопросам при разборе кода не следует:
		 *
		 * @li **Договор извлечения значений взят у контейнера JSON дословно.** Вид
		 *     хранения извлечению не указ, отказ наступает лишь у не-числа, а дробное за
		 *     пределами затребованного вида выдаёт предел этого вида. Два кодека, читающие
		 *     одно и то же число по-разному, - это дефект, даже если каждый по себе прав
		 *
		 * @li **Число, в затребованный вид не вмещающееся, отказом НЕ отвечает.** Правило
		 *     это замерено 17.09.2026 на пяти кодеках разом и у всех пяти одинаково, а
		 *     записано было лишь наполовину - о дробном. Полностью оно таково: целое усекается
		 *     по младшим разрядам (`300` в `int8_t` даёт 44, `70000` в `int16_t` даёт 4464,
		 *     `-300` в `uint8_t` даёт 212), а дробное выдаёт предел вида (`300.7` в `int8_t`
		 *     даёт 127). Признак успеха возвращается В ОБОИХ случаях
		 *
		 * @li **Дробное, целым представимое, идёт путём ЦЕЛОГО.** Оттого `300.0` в
		 *     `int8_t` даёт 44, а `300.7` - 127: записи эти обе дробные, а правила у них
		 *     разные, и разность эта видна лишь замером. Потребителю, ширину вида берегущему,
		 *     надлежит спрашивать вид шире и сличать самому - признак успеха тут о вмещении
		 *     не говорит ничего
		 *
		 * @li **Нечисловая величина, извлекаемая целым видом, даёт нуль и признак успеха.**
		 *     Приведение `NaN` к целому есть неопределённое поведение при любом пределе, и
		 *     машины расходятся на деле: x86-64 выдаёт нижний предел, ARM64 - нуль. Правило
		 *     это взято у контейнера JSON и расхождению между ними не подлежит
		 *
		 * @li **Наречие 1.2 задано умолчанием, наречие 1.1 - настройкой.** Иначе `no` в
		 *     стране NO читается ложью, а `12:30` - числом 750
		 *
		 * @li **Поток ссылки не раскрывает.** Ссылка выдаётся событием, а раскрывает её
		 *     дерево: раскрытие потоком отняло бы у него постоянство памяти, ради которого он
		 *     и существует
		 *
		 * @li **Повторяющееся имя по умолчанию объявляется отказом.** Описание его
		 *     запрещает, а реализации расходятся; прочие правила доступны настройкой
		 *
		 * @li **Глубина вложенности и раскрытие ссылок ограничены.** Разбор ведётся без
		 *     рекурсии, но предел нужен и без неё: десять меток по десять ссылок раскрываются
		 *     в гигабайты, и беда эта известна под именем «миллиард смешков»
		 *
		 * \~english
		 * @brief YAML container namespace
		 *
		 * @details The container parses and builds a text according to the YAML 1.2.2 specification. From
		 *          the neighbouring codecs it differs in that the meaning of the text is carried by the **indentation** rather than by
		 *          the bracket characters, and in that one and the same record is read differently depending on
		 *          the surroundings. The structure is set by four decisions:
		 *
		 * @li **The levels of the nesting are held by a stack of the indentations.** The closing of a level comes not
		 *     by a closing character but by the arrival of a line with a smaller indentation, whereby the decision about
		 *     the closing is postponed until that line
		 *
		 * @li **The parsing does not look ahead.** A character whose meaning depends on the one following
		 *     it transfers the parsing into a separate state rather than demanding the next character
		 *     immediately. Whereby the issuance does not depend on how the text is cut into the chunks at the
		 *     feeding
		 *
		 * @li **The rules of the termination of a plain scalar are gathered into one body.** The record `a: b#c`
		 *     gives the value `b#c`, while `a: b #c` gives the value `b` with a comment; in the flow
		 *     surroundings the rules are other than in the block one. Two sets of the rules would diverge at
		 *     the very first editing of one of them
		 *
		 * @li **The tree holds a segment of the source text at every node.** A node not touched by an editing
		 *     is written back by the **literal source bytes**, while only a touched one is assembled
		 *     anew. Whereby the agreement of the rewriting with the source text is byte-for-byte
		 *     by the construction rather than by the diligence of the assembler
		 *
		 * @details **Deliberate decisions.** What is enumerated below is chosen consciously, and
		 *          one should not return to these questions at the reading of the code:
		 *
		 * @li **The contract of the extraction of the values is taken from the JSON container literally.** The kind
		 *     of the storage is not a directive to the extraction, a refusal comes only at a non-number, while a fractional number beyond
		 *     the limits of the demanded kind issues the limit of that kind. Two codecs reading
		 *     one and the same number differently is a defect even if each of them is right by itself
		 *
		 * @li **A number not fitting into the demanded kind does NOT answer with a refusal.** The rule
		 *     is measured on 2026-09-17 across five codecs at once and is alike in all the five, while
		 *     written down it was only by half — about the fractional one. In full it is this: an
		 *     integer is truncated by the lower digits (`300` into `int8_t` gives 44, `70000` into
		 *     `int16_t` gives 4464, `-300` into `uint8_t` gives 212), while a fractional one issues the
		 *     limit of the kind (`300.7` into `int8_t` gives 127). The sign of a success is returned IN
		 *     BOTH cases
		 *
		 * @li **A fractional number representable by an integer goes the INTEGER path.** Whereby
		 *     `300.0` into `int8_t` gives 44 while `300.7` gives 127: both records are fractional
		 *     notations, while their rules are different, and this difference is visible only by a
		 *     measurement. A consumer guarding the width of the kind ought to ask for a wider kind and
		 *     compare himself — the sign of a success here tells nothing of the fitting
		 *
		 * @li **A non-numeric value extracted by an integer kind gives a zero and the sign of a success.**
		 *     The conversion of a `NaN` to an integer is an undefined behaviour at any limit, and
		 *     the machines diverge in the deed: x86-64 issues the lower limit, ARM64 — a zero. This rule
		 *     is taken from the JSON container and is not subject to a divergence between them
		 *
		 * @li **The 1.2 dialect is set by default, the 1.1 dialect — by a setting.** Otherwise `no` in
		 *     the country NO is read as a falsehood, while `12:30` — as the number 750
		 *
		 * @li **The stream does not expand the aliases.** An alias is issued by an event, while the tree expands it:
		 *     an expansion by the stream would take away from it the constancy of the memory for the sake of which it
		 *     exists
		 *
		 * @li **A repeating name is declared a refusal by default.** The specification forbids
		 *     it, while the implementations diverge; the other rules are available by a setting
		 *
		 * @li **The depth of the nesting and the expansion of the aliases are limited.** The parsing is conducted without
		 *     a recursion, but a limit is needed even without it: ten anchors by ten aliases expand
		 *     into gigabytes, and this trouble is known under the name of the «billion laughs»
		 *
		 * \~
		 */
		namespace yaml {
			/**
			 * \~russian
			 * @brief Глубина вложенности по умолчанию при чтении и предельная при записи
			 *
			 * @details При чтении предел ограничивает глубину разбираемого текста и размер
			 *          стека вложенности. Настройка «maxDepth» заменяет его положительным
			 *          значением. При записи предел фиксирован: открыть больше вместилищ
			 *          одновременно нельзя, настройки для его изменения нет.
			 *
			 * @note Разбор текста и сборка текста по дереву документа используют явные
			 *       стеки. Снятие дерева во владеющее значение, копирование значения и
			 *       сборка текста по нему используют рекурсию. Предел записи останавливает
			 *       рекурсивную сборку значения при отказе открытия очередного вместилища
			 *
			 * @warning Увеличение предела чтения не увеличивает предел записи и не устраняет
			 *          рекурсию при снятии и копировании владеющего значения. Успешный разбор
			 *          более глубокого документа не гарантирует его записи: при сборке текста
			 *          заново «dump()» отказывает по глубине. Дословный перенос удержанного
			 *          исходного текста не требует такой сборки
			 *
			 * \~english
			 * @brief Default nesting depth for reading and maximum nesting depth for writing
			 *
			 * @details Reading limits the nesting of the input text and the size of its nesting
			 *          stack. A positive «maxDepth» setting replaces this limit. Writing uses
			 *          a fixed limit on simultaneously opened containers, with no setting
			 *          available to change it.
			 *
			 * @note Parsing and composing text from a document tree use explicit stacks.
			 *       Taking a tree into an owning value, copying the value and composing text
			 *       from it use recursion. The writing limit stops recursive composition
			 *       of a value when opening the next container fails
			 *
			 * @warning Raising the reading limit does not raise the writing limit or remove
			 *          recursion when taking and copying an owning value. Successfully parsing
			 *          a deeper document does not guarantee it can be written: «dump()» refuses
			 *          to rebuild text beyond the writing depth limit. Verbatim copying of
			 *          retained source text does not require rebuilding it
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 0x400;
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
			 * @brief Количество пар отображения, начиная с какого заводится указатель имён
			 *
			 * @details Ниже этого порога имя разыскивается перебором, и это дешевле всякого
			 *          указателя: сличение имён идёт по памяти подряд, а заведение указателя стоит
			 *          выделения памяти и подсчёта хеша на всякое имя. Выше порога перебор обращает
			 *          обход отображения по именам в квадратичный, и указатель окупается.
			 *
			 * @note Указатель заводится **по требованию** - при первом обращении по имени, а
			 *       не при разборе. Отображение, к которому по имени не обращались, не платит
			 *       ничего
			 *
			 * \~english
			 * @brief Number of the pairs of a mapping starting from which the index of the names is created
			 *
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 *          index: the comparison of the names goes through the memory consecutively, while the creation of an index
			 *          costs an allocation of the memory and a computation of a hash for every name. Above the threshold an enumeration
			 *          turns the traversal of a mapping by the names into a quadratic one, and the index pays off.
			 *
			 * @note The index is created **on demand** — at the first access by a name rather
			 *       than at the parsing. A mapping which has not been accessed by a name pays
			 *       nothing
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;
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
			 * @brief Наибольшая допустимая длина скалярного значения в байтах
			 *
			 * @details Предел считается по значению уже со снятой оградой и выполненной
			 *          свёрткой строк.
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
			 *
			 * @details The limit is counted over the value already with the quoting removed and the folding
			 *          of the lines performed.
			 *
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
			 * @brief Наибольшее допустимое количество узлов, порождаемых раскрытием ссылок
			 *
			 * @details Предел этот - ответ на беду, известную под именем «миллиард смешков»:
			 *          метка, ссылающаяся на метку, раскрывается произведением, и десяти уровней по
			 *          десяти ссылок довольно, чтобы занять всю память машины. Текст при этом
			 *          умещается в несколько строк.
			 *
			 * @note Цена предела этого замерена 17.09.2026: узел дерева стоит 119 байтов,
			 *       и предел в 16 777 216 узлов даёт 1.86 ГБ. Текст, до предела этого доходящий,
			 *       умещается в 341 байт - десять уровней ссылок по девяти упоминаний, - и
			 *       отказ приходит через 137 мс, израсходовав 1.36 ГБ
			 *
			 * @note Опора для суждения о величине: настоящий документ в 178 КБ
			 *       (`CMakeConfigureLog.yaml` сборки) разбирается в 3084 узла. Запас умолчания
			 *       над таким документом пятитысячекратен
			 *
			 * @warning Предел останавливает разбор ровно по счёту узлов, но память под них
			 *          УЖЕ выделена к тому мигу: замер с пределом 100 даёт 92 узла и 6.6 МБ, с
			 *          пределом 10 000 - 8304 узла. Потребителю, память берегущему, надлежит
			 *          ставить `maxExpansion` своей величиной, а не полагаться на умолчание
			 *
			 * \~english
			 * @brief Largest admissible number of the nodes generated by the expansion of the aliases
			 *
			 * @details This limit is the answer to the trouble known under the name of the «billion laughs»:
			 *          an anchor referring to an anchor expands by a product, and ten levels by
			 *          ten aliases are enough to occupy all the memory of a machine. The text thereby
			 *          fits into a few lines.
			 *
			 * @note The price of this limit was measured on 2026-09-17: a node of the tree costs 119 bytes,
			 *       and the limit of 16 777 216 nodes gives 1.86 GB. A text reaching this limit fits into
			 *       341 bytes — ten levels of aliases by nine mentions each — and the refusal comes through
			 *       137 ms having spent 1.36 GB
			 *
			 * @note A point of support for a judgement of the magnitude: a real document of 178 KB
			 *       (`CMakeConfigureLog.yaml` of a build) is parsed into 3084 nodes. The margin of the
			 *       default over such a document is five-thousandfold
			 *
			 * @warning The limit stops the parsing exactly by the count of the nodes, but the memory for
			 *          them is ALREADY allocated by that moment: a measurement with the limit of 100 gives
			 *          92 nodes and 6.6 MB, with the limit of 10 000 — 8304 nodes. A consumer guarding the
			 *          memory ought to set `maxExpansion` by its own size rather than rely on the default
			 *
			 * \~
			 */
			constexpr uint32_t MAX_EXPANSION = 0x1000000;
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
			 * @brief Смещение, записи в исходном тексте не имеющее
			 *
			 * @note Знаменует оно узел, за которым записи в тексте не стоит: текст не
			 *       удержан настройкою либо узел взят раскрытием ссылки
			 *
			 * \~english
			 * @brief Offset having no record in the source text
			 *
			 * @note It marks a node behind which no record in the text stands: the text is not held by the
			 *       setting, or the node is taken by the expansion of an alias
			 *
			 * \~
			 */
			constexpr uint32_t NO_ORIGIN = static_cast <uint32_t> (~0u);
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
			 * @warning ОБЪЯВЛЕННЫЕ, НО НИКОГДА НЕ ВЫДАВАЕМЫЕ отказы: замерено 08.09.2026 розыском
			 *          `error_t::<имя>` по исходникам кодека за вычетом `common.cpp`, где стоит
			 *          лишь таблица сообщений. Таковых ДЕСЯТЬ из пятидесяти четырёх:
			 *          UNEXPECTED_EOF, INVALID_NUMBER, NUMBER_OUT_OF_RANGE, EXPECTED_KEY,
			 *          EXPECTED_COLON, DUPLICATE_ANCHOR, UNEXPECTED_DOCUMENT, NUMBER_TOO_LONG,
			 *          OVERFLOW_LIMIT и CONFLICTING_SETTINGS
			 *
			 * @note Перемерено 17.09.2026 тем же розыском: с прошлого замера ожил
			 *       SETTINGS_ALREADY_APPLIED - выдаётся он ныне читающим при подаче настроек
			 *       вслед начатому разбору (`reader.cpp`), - и одиннадцать обратились десятью.
			 *       Запись, замера пережившая, здесь опаснее отсутствия: потребитель по ней
			 *       счёл бы отказ недостижимым и обхода ему не написал
			 *
			 * @note Число называю вместе с признаком счёта: считаны имена перечня, а выдачей
			 *       признаётся всякое упоминание вне таблицы сообщений. Пересчитывается тем же
			 *       розыском. Прежде их было тринадцать из пятидесяти; трое ожили в тот же день
			 *       - INVALID_BINARY, INVALID_STAMP и STORAGE_EXHAUSTED, - а перечень вырос
			 *       четырьмя кодами
			 *
			 * @note Причины УСТАНОВЛЕНЫ опытом у пяти, и разряды их РАЗНЫЕ:
			 *
			 * @note DUPLICATE_ANCHOR мёртв ПО ОПИСАНИЮ: повтор имени метки описанием дозволен -
			 *       `a: &x 1` да `b: &x 2` со ссылкою `*x` дают последнее значение, - и
			 *       отвергать тут нечего. Заведись отказ - кодек разошёлся бы с описанием
			 *
			 * @note UNEXPECTED_DOCUMENT мёртв ПО УСТРОЙСТВУ: многодокументная подача принимается
			 *       и сохраняется целиком, круг цел при обеих чертах и знаке конца - замерено
			 *       шестью подачами при удержании исходного текста
			 *
			 * @note NUMBER_TOO_LONG мёртв ЗА ОТСУТСТВИЕМ ПРЕДЕЛА: ни настройки, ни постоянной
			 *       `MAX_NUMBER` в кодеке не заведено вовсе, а длина записи числа огранена
			 *       общим пределом скалярного значения `MAX_SCALAR` и кодом SCALAR_TOO_LONG
			 *
			 * @note OVERFLOW_LIMIT мёртв КАК ЛИШНИЙ: всякий предел настроек имеет СВОЙ код -
			 *       DEPTH_EXCEEDED, SCALAR_TOO_LONG, TOO_MANY_NODES, ANCHOR_TOO_LONG, - и общий
			 *       поверх них не нужен. У кодеков XML, JSON, CEF и syslog он ЖИВ, ибо там
			 *       пределы своих кодов не имеют: расхождение это предписано устройством
			 *
			 * @note CONFLICTING_SETTINGS мёртв ЗА ОТСУТСТВИЕМ ПАРЫ, круг рвущей, и это ЗАМЕРЕНО,
			 *       а не выведено: расхождение схем разрешения между записью и чтением круга не
			 *       рвёт ни при одном сочетании - CORE против LEGACY, LEGACY против CORE, CORE
			 *       против JSON и против FAILSAFE, - запись хранит вид значения. У кодека INI
			 *       пара такая есть (ограда при читающем, её не снимающем), у TOML заведена
			 *       08.09.2026 (Юникод в голом имени при чтении, его не признающем)
			 *
			 * @note Прочие пять - UNEXPECTED_EOF, INVALID_NUMBER, NUMBER_OUT_OF_RANGE,
			 *       EXPECTED_KEY и EXPECTED_COLON - причин установленных не имеют: переехали
			 *       они в перечень вместе с образцом кодека JSON, где объявлены и живы, а к
			 *       местам выдачи привязаны не были
			 *
			 * @note Установлено 17.09.2026, что двое из пяти перекрыты кодом более общим:
			 *       на запись `a: b: c` да на `{a: 1: 2}` приходит INVALID_CHARACTER - «знак
			 *       не дозволен в этом месте», - а не EXPECTED_KEY либо EXPECTED_COLON. Отказ
			 *       при этом есть и причина названа верно, оттого правкой это не считаю:
			 *       место выдачи тут не пропущено, а занято соседом
			 *
			 * @note Потребителю они видимы наравне с живыми: он вправе ждать их и писать на них
			 *       разбор, какой не сработает никогда
			 *
			 * \~english
			 * @warning THE REFUSALS DECLARED BUT NEVER ISSUED: measured 2026-09-08 by a search of
			 *          `error_t::<name>` over the sources of the codec, `common.cpp` excepted, where only the
			 *          table of the messages stands. There are TEN of such among the fifty-four: UNEXPECTED_EOF,
			 *          INVALID_NUMBER, NUMBER_OUT_OF_RANGE, EXPECTED_KEY, EXPECTED_COLON, DUPLICATE_ANCHOR,
			 *          UNEXPECTED_DOCUMENT, NUMBER_TOO_LONG, OVERFLOW_LIMIT and CONFLICTING_SETTINGS
			 *
			 * @note Re-measured 2026-09-17 by the same search: since the last measurement
			 *       SETTINGS_ALREADY_APPLIED has come alive — it is issued now by the reading at a feeding of
			 *       the settings following a parsing already begun (`reader.cpp`) — and eleven have turned into
			 *       ten. A record surviving a measurement is more dangerous here than an absence: a consumer
			 *       would count a refusal unreachable by it and would write no workaround for it
			 *
			 * @note I name the number together with the sign of the counting: the names of the list have been
			 *       counted, while an issuing is acknowledged as any mention outside the table of the messages.
			 *       It is recounted by the same search. Formerly there were thirteen of the fifty; three came
			 *       alive on that same day — INVALID_BINARY, INVALID_STAMP and STORAGE_EXHAUSTED — and the list
			 *       grew by four codes
			 *
			 * @note The reasons are ESTABLISHED by an experience for five, and their ranks are DIFFERENT:
			 *
			 * @note DUPLICATE_ANCHOR is dead BY THE DESCRIPTION: a repetition of a label name is allowed by the
			 *       description — `a: &x 1` and `b: &x 2` with a reference `*x` give the last value — and there
			 *       is nothing to refuse here. Had a refusal been instituted, the codec would have diverged from
			 *       the description
			 *
			 * @note UNEXPECTED_DOCUMENT is dead BY THE CONSTRUCTION: a feeding of many documents is accepted and
			 *       is kept in full, the round holds with both dashes and with the sign of the end — measured by
			 *       six feedings at a retention of the source text
			 *
			 * @note NUMBER_TOO_LONG is dead FOR THE ABSENCE OF A LIMIT: neither a setting nor a constant
			 *       `MAX_NUMBER` is instituted in the codec at all, and the length of the writing of a number is
			 *       fenced by the common limit of a scalar value `MAX_SCALAR` and by the code SCALAR_TOO_LONG
			 *
			 * @note OVERFLOW_LIMIT is dead AS REDUNDANT: every limit of the settings has ITS OWN code —
			 *       DEPTH_EXCEEDED, SCALAR_TOO_LONG, TOO_MANY_NODES, ANCHOR_TOO_LONG — and a common one above
			 *       them is not needed. At the codecs XML, JSON, CEF and syslog it is ALIVE, for there the
			 *       limits have no codes of their own: this divergence is prescribed by the construction
			 *
			 * @note CONFLICTING_SETTINGS is dead FOR THE ABSENCE OF A PAIR tearing the round, and this IS
			 *       MEASURED, not drawn: a divergence of the schemes of the resolution between the writing and
			 *       the reading breaks the round at no combination — CORE against LEGACY, LEGACY against CORE,
			 *       CORE against JSON and against FAILSAFE — the writing keeps the kind of a value. The INI
			 *       codec has such a pair (a fence at a reading which does not lower it), in TOML it was
			 *       instituted 2026-09-08 (Unicode in a bare name at a reading which does not acknowledge it)
			 *
			 * @note The other five — UNEXPECTED_EOF, INVALID_NUMBER, NUMBER_OUT_OF_RANGE, EXPECTED_KEY and
			 *       EXPECTED_COLON — have no established reasons: they moved into the list together with the
			 *       pattern of the JSON codec, where they are declared and alive, and were not tied to the
			 *       places of an issuing
			 *
			 * @note It was established 2026-09-17 that two of the five are covered by a more general code: at a
			 *       writing `a: b: c` and at `{a: 1: 2}` an INVALID_CHARACTER arrives — «a sign is not allowed
			 *       in this place» — and not an EXPECTED_KEY or an EXPECTED_COLON. A refusal at that is present
			 *       and the reason is named rightly, so I do not count it an editing: the place of an issuing
			 *       here is not skipped but is taken by a neighbour
			 *
			 * @note They are visible to a consumer on a par with the living ones: he has the right to expect
			 *       them and to write by them a parsing that will never work
			 *
			 * \~
			 */

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
			 * @brief Правила преобразования чисел при разборе
			 *
			 * @details Правило задаёт лишь то, как поступать с числом, не вместимым ни в один
			 *          родной вид, - целым свыше `2^64` либо записью точнее `double`
			 *
			 * \~english
			 * @brief Rules of the conversion of the numbers at the parsing
			 *
			 * @details The rule sets only how to handle a number not containable in any
			 *          native kind — an integer above `2^64` or a record more precise than a `double`
			 *
			 * \~
			 */
			enum class number_t : uint8_t {
				NATIVE = 0x00, // Число, не вместимое ни в один родной вид, хранится записью
				CHECK  = 0x01  // Число, не вместимое ни в один родной вид, есть отказ разбора
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
			 * @note Настройка эта стояла прежде лишь у ДВУХ кодеков рамки из семи - YAML и JSON, -
			 *       тогда как довод выше гласит, что правило положено ОДИНАКОВЫМ для
			 *       всех. Хуже того, у TOML и INI отказ был БЕЗУСЛОВЕН и снабжён доводом,
			 *       ПРОТИВОПОЛОЖНЫМ здешнему: там писано, что отказ лучше подмены, ибо подмена
			 *       меняет данные молча. Два довода, друг другу перечащих, стояли в одной рамке
			 *
			 * @note Разведено 08.09.2026 в пользу общего довода: настройка заведена у TOML и
			 *       INI тем же составом и тем же умолчанием, а запись о расхождении снята ВМЕСТЕ
			 *       с заведением, а не после него. У CSV, XML и CEF её по-прежнему нет - кодеки
			 *       эти чужие, и расхождение вынесено к тем кодекам
			 *
			 * \~english
			 * @note This setting stood formerly only at TWO codecs of the frame of seven — YAML and JSON —
			 *       while the argument above says that the rule is bound to be COMMON for all of them. Worse
			 *       still, at TOML and INI the refusal was UNCONDITIONAL and furnished with an argument
			 *       OPPOSITE to this one: there it is written that a refusal is better than a substitution, for
			 *       a substitution changes the data silently. Two arguments contradicting each other stood in
			 *       one frame
			 *
			 * @note Divided 2026-09-08 in favour of the common argument: the setting was instituted at TOML and
			 *       INI by the same composition and the same default, and the record about the divergence was
			 *       removed TOGETHER with the institution, and not after it. At CSV, XML and CEF it is still
			 *       absent — those codecs are foreign, and the divergence is carried out to those codecs
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
			 * @brief Схемы разрешения видов скалярных значений
			 *
			 * @details Схема решает, чем окажется запись, оградою не обнесённая: `0777` есть
			 *          число 511 по наречию 1.1 и строка по наречию 1.2, а `12:30` - число 750 по
			 *          первому и строка по второму.
			 *
			 * @note Схема действует лишь при разборе: вид значения разрешается однажды, а
			 *       извлечение его только преобразует. Смена схемы после разбора ничего не
			 *       переразрешает - иначе одно и то же дерево отвечало бы на один вопрос
			 *       по-разному в зависимости от того, когда спросили
			 *
			 * \~english
			 * @brief Schemas of the resolution of the kinds of the scalar values
			 *
			 * @details A schema decides what a record not surrounded by a quoting turns out to be: `0777` is
			 *          the number 511 by the 1.1 dialect and a string by the 1.2 dialect, while `12:30` — the number 750 by
			 *          the first one and a string by the second one.
			 *
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
			 * @brief Правила обращения с повторяющимся именем пары отображения
			 *
			 * @details Описание повторяющиеся имена запрещает, однако реализации расходятся:
			 *          одни берут первое значение, другие последнее, третьи отвергают текст.
			 *
			 * @warning Перечень этот и порядок его членов совпадают с перечнем контейнера
			 *          JSON намеренно
			 *
			 * \~english
			 * @brief Rules of the handling of a repeating name of a pair of a mapping
			 *
			 * @details The specification forbids the repeating names, however the implementations diverge:
			 *          some take the first value, others the last one, the third ones reject the text.
			 *
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
			 * @brief Виды записи скалярного значения
			 *
			 * @details Вид записи хранится при узле не ради красоты, а ради перезаписи:
			 *          значение, записанное человеком дословной оградой, обязано такою и остаться,
			 *          иначе правка одного ключа переписала бы весь файл.
			 *
			 * \~english
			 * @brief Kinds of the notation of a scalar value
			 *
			 * @details The kind of the notation is held at a node not for the sake of the beauty but for the sake of the rewriting:
			 *          a value written by a person with a literal quoting must remain such,
			 *          otherwise an editing of one key would rewrite the whole file.
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
			 * @brief Виды узлов документа
			 *
			 * @details Виды эти отвечают описанию: узел есть либо скалярное значение, либо
			 *          перечень, либо отображение. Разделение скалярных значений по содержимому
			 *          принадлежит не узлу, а виду значения.
			 *
			 * @note Ссылка видом узла не является: дерево её раскрывает, и в дереве стоит
			 *       уже тот узел, на который ссылались. Событие ссылки есть лишь у потока
			 *
			 * \~english
			 * @brief Kinds of the nodes of a document
			 *
			 * @details These kinds correspond to the specification: a node is either a scalar value, or
			 *          a sequence, or a mapping. The division of the scalar values by their content
			 *          belongs not to the node but to the kind of the value.
			 *
			 * @note An alias is not a kind of a node: the tree expands it, and in the tree there stands
			 *       already that node which was referred to. The event of an alias exists only in the stream
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE     = 0x00, // Узел не определён либо ссылка недействительна
				NIL      = 0x01, // Пустое значение null
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
			 * @brief Виды событий чтения текста
			 *
			 * @details Названия событий взяты в точности из эталонного набора
			 *          `yaml-test-suite`: он печатает ожидаемый исход разбора рядом событий, и
			 *          совпадение имён обращает сличение с ним в сличение строка в строку, без
			 *          переводчика между моделями.
			 *
			 * @note Ссылка выдаётся событием, а не раскрывается: раскрытие есть дело дерева
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a text
			 *
			 * @details The names of the events are taken exactly from the reference set
			 *          `yaml-test-suite`: it prints the expected outcome of a parsing as a series of the events, and
			 *          the coincidence of the names turns the comparison with it into a comparison line by line, without
			 *          a translator between the models.
			 *
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
			 * @brief Виды значений документа
			 *
			 * @details Виды заданы разрядами, чтобы вопрос о принадлежности значения к
			 *          набору видов решался одним наложением разрядов: точный вопрос (`INT32`) и
			 *          сборный (`NUMBER`) стоят ровно одинаково.
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
			 *
			 * @details The kinds are set by the bits so that the question of the belonging of a value to
			 *          a set of the kinds is decided by one overlaying of the bits: an exact question (`INT32`) and
			 *          a composite one (`NUMBER`) cost exactly the same.
			 *
			 * @note The kind of the storage is not a directive to the extraction: an `INT8` is extracted both as a `double` and
			 *       as a `uint64_t`. The extraction compares with the limits of the demanded kind the value
			 *       itself rather than the kind of its storage
			 *
			 * @note A number not containable in any native kind — an integer above `2^64` or
			 *       a record more precise than a `double` — receives the kind `EXTENDED` and is stored as a record.
			 *       The precision of such a number is preserved completely, while the extraction parses the record
			 *
			 * @warning This list must coincide with the list of the JSON container in everything
			 *          that both of them have: a divergence would mean that one and the same number
			 *          is read by the two codecs differently
			 *
			 * \~
			 */
			enum class type_t : uint32_t {
				UNDEFINED = 0x00000000, // Значения нет вовсе: ссылка недействительна
				NIL       = 0x00000001, // Пустое значение null
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
				SIGNED = (INT8 | INT16 | INT32 | INT64),
				// Целое без знака любой ширины
				UNSIGNED = (UINT8 | UINT16 | UINT32 | UINT64),
				// Целое любой ширины и любой знаковости
				INT = (SIGNED | UNSIGNED),
				// Дробное любой точности
				REAL = (FLOAT | DOUBLE),
				// Число любого вида
				NUMBER = (INT | REAL | EXTENDED)
			};
			/**
			 * \~russian
			 * @brief Коды ошибок разбора текста
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 *          вместе с положением в исходном тексте, где отказ произошёл.
			 *
			 * \~english
			 * @brief Error codes of the parsing of a text
			 *
			 * @details The parsing does not throw exceptions: the error code together with the position
			 *          in the source text where the refusal has occurred serves as the sign of a refusal.
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                     = 0x00, // Ошибок не обнаружено
				INTERNAL                 = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF           = 0x02, // Текст оборвался посреди значения
				INVALID_CHARACTER        = 0x03, // Знак недопустим в этом месте текста
				INVALID_ENCODING         = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING     = 0x05, // Объявленная кодировка не поддерживается
				INVALID_INDENTATION      = 0x06, // Отступ не отвечает ни одному из открытых уровней
				TAB_IN_INDENTATION       = 0x07, // Отступ содержит знак горизонтальной подачи, описанием запрещённый
				UNTERMINATED_SCALAR      = 0x08, // Скалярное значение не закрыто оградой до конца текста
				INVALID_ESCAPE           = 0x09, // Отменяющая последовательность не опознана
				INVALID_UNICODE          = 0x0A, // Запись знака Юникода содержит недопустимые знаки
				UNPAIRED_SURROGATE       = 0x0B, // Суррогат не образует пары
				INVALID_BLOCK_HEADER     = 0x0C, // Заголовок блочного значения построен ошибочно
				INVALID_NUMBER           = 0x0D, // Запись числа не отвечает действующей схеме
				NUMBER_OUT_OF_RANGE      = 0x0E, // Число не представимо затребованным видом
				INVALID_BINARY           = 0x0F, // Содержимое метки !!binary не отвечает записи base64
				INVALID_STAMP            = 0x10, // Ошибочное построение отметки времени
				EXPECTED_VALUE           = 0x11, // Ожидалось значение
				EXPECTED_KEY             = 0x12, // Ожидалось имя пары отображения
				EXPECTED_COLON           = 0x13, // Ожидалось двоеточие после имени пары
				EXPECTED_COMMA           = 0x14, // Ожидалась запятая либо закрывающая скобка поточного построения
				UNCLOSED_FLOW            = 0x15, // Поточное построение не закрыто скобкой
				MIXED_COLLECTION         = 0x16, // Перечень и отображение смешаны на одном уровне
				DUPLICATE_KEY            = 0x17, // Имя пары отображения объявлено повторно
				COMPLEX_KEY              = 0x18, // Составное имя пары при запрещённых составных именах
				UNKNOWN_ALIAS            = 0x19, // Ссылка указывает на метку, ещё не объявленную
				DUPLICATE_ANCHOR         = 0x1A, // Метка с таким именем уже объявлена при строгом разборе
				RECURSIVE_ALIAS          = 0x1B, // Ссылка указывает сама на себя через цепочку меток
				EXPANSION_EXCEEDED       = 0x1C, // Раскрытие ссылок порождает больше узлов, чем дозволено
				INVALID_TAG              = 0x1D, // Метка типа построена ошибочно
				UNKNOWN_TAG_HANDLE       = 0x1E, // Сокращение метки типа не объявлено директивой %TAG
				TAG_MISMATCH             = 0x1F, // Содержимое не отвечает виду, заданному меткой типа
				INVALID_DIRECTIVE        = 0x20, // Директива построена ошибочно
				UNSUPPORTED_VERSION      = 0x21, // Наречие, объявленное директивой %YAML, не поддерживается
				UNEXPECTED_DOCUMENT      = 0x22, // Начало нового документа посреди значения
				TRAILING_CHARACTERS      = 0x23, // Знаки за окончанием документа
				DEPTH_EXCEEDED           = 0x24, // Глубина вложенности превышает допустимую
				SCALAR_TOO_LONG          = 0x25, // Длина скалярного значения превышает допустимую
				NUMBER_TOO_LONG          = 0x26, // Длина записи числа превышает допустимую
				ANCHOR_TOO_LONG          = 0x27, // Длина имени метки превышает допустимую
				TOO_MANY_NODES           = 0x28, // Количество узлов документа превышает допустимое
				EMPTY_TEXT               = 0x29, // Текст пуст, а документ затребован
				OVERFLOW_LIMIT           = 0x2A, // Превышен предел, ЗАДАННЫЙ НАСТРОЙКАМИ разбора: разрядность хранилища сюда не входит
				CONFLICTING_SETTINGS     = 0x2B, // Настройки записи противоречат толкованию читающего
				FILE_NOT_OPENED          = 0x2C, // Файл документа открыть не удалось
				UNKNOWN_NODE             = 0x2D, // Узла по такому пути документ не несёт
				INVALID_PATH             = 0x2E, // Путь к узлу построен ошибочно
				FILE_NOT_WRITTEN         = 0x31, // Текст документа записать в файл не удалось
				UNEXPECTED_CONTENT       = 0x2F, // Запись не отвечает построению, на этом месте открытому
				STORAGE_EXHAUSTED        = 0x30, // Разбираемый текст не помещается в разрядность хранилища
				FILE_NOT_READ            = 0x32, // Текст документа из файла прочитать не удалось
				TEXT_ALREADY_ENDED       = 0x33, // Подача текста продолжена после объявленного конца его
				ENCODING_ALREADY_CHOSEN  = 0x34, // Кодировка сменена посреди подачи текста
				SETTINGS_ALREADY_APPLIED = 0x35  // Настройки сменены посреди подачи текста
			};

			/**
			 * \~russian
			 * @note Перечень этот общ у кодеков INI, TOML и YAML ИМЕНАМИ, а не числами, и
			 *       равным держатся именно имена: понятие, у трёх наречий общее, зовётся у всех
			 *       трёх одинаково, и потребитель учит одно имя, а не три
			 *
			 * @warning ЧИСЛА при общих именах РАЗЛИЧНЫ, и прежняя запись - «потребитель,
			 *          разбирающий отказы по КОДУ, иначе учил бы для каждого кодека свой
			 *          перечень» - обещала обратное. Замерено 09.09.2026 сличением перечней
			 *          почленно: имён у всех трёх - 19, из них с РАЗНЫМИ числами - 13
			 *          (среди прочих TEXT_ALREADY_ENDED: ini 37, toml 38, yaml 51).
			 *          Числа и не могут совпасть: перечни разной длины - 40, 41 и 54 члена, -
			 *          а коды у каждого наречия свои сверх общих
			 *
			 * @note Разбирать отказы надлежит ПО ИМЕНИ своего кодека, а не по числу: типы
			 *       `ini::error_t`, `toml::error_t` и `yaml::error_t` разные, и сличение
			 *       чисел между ними смысла не имеет вовсе
			 *
			 * @note Оттого часть кодов кодеком YAML не выдаётся:
			 *
			 * @note - `STORAGE_EXHAUSTED` выдаётся ДВУМЯ местами, и пробел, здесь прежде
			 *       описанный, ЗАКРЫТ. Дерево YAML держит смещения в хранилище знаков четырьмя
			 *       октетами наравне с INI, TOML, XML, CSV и JSON - сверено 09.09.2026 по члену
			 *       `span_t` всех шести, у каждого `uint32_t offset` и `uint32_t length`, - и
			 *       ширина эта ныне поверяется:
			 *       при переносе записи (`deposit`) - дабы уберечь само вычисление смещения, - и
			 *       заставою при входе разбора, где отказ выходит наружу
			 *
			 * @warning Запись гласила «мест выдачи не имеет вовсе… вынесено за пределы кодека
			 *          отдельным докладом» и стояла так уже ПОСЛЕ того, как поверка была
			 *          заведена. Замерено 09.09.2026 счётом мест выдачи: у YAML их два,
			 *          тогда как у `UNEXPECTED_EOF` и `OVERFLOW_LIMIT` кодека INI и у
			 *          `OVERFLOW_LIMIT` кодека TOML - и вправду ноль
			 *
			 * @warning Отсюда тот же вывод, что и при подаче после конца текста: запись,
			 *          объявляющая пробел, обязана сноситься правкою, какая пробел
			 *          закрывает. Иначе вынесенное уходит второй раз по работе,
			 *          уже сделанной
			 *
			 * @warning Молчание кода означает НЕПРОВЕРЕННОСТЬ пути, а не исправность его:
			 *          снасть молчащих кодов перебирает места выдачи по трём прогонам, и код, ни
			 *          разу не выданный, есть либо застава с доводом, либо путь, проверкою не взятый
			 *
			 * \~english
			 * @note The INI, TOML, and YAML codecs share this set of identifiers—using names
			 *       rather than numbers. These names remain consistent across all three formats:
			 *       a given concept, common to all three, is referred to by the same name in each,
			 *       meaning the user only needs to learn one name instead of three
			 *
			 * @warning The numerical values ​​associated with shared names DIFFER, even though
			 *          the previous note—"a consumer that processes failures by CODE,
			 *          rather than having to learn a separate list for each format"—implied
			 *          the opposite. A comparison of the lists, item by item, was conducted
			 *          on September 9, 2026: there are 19 names common to all three,
			 *          13 of which have DIFFERENT numerical values ​​(including, for instance,
			 *          `TEXT_ALREADY_ENDED`: 37 for `ini`, 38 for `toml`, and 51 for `yaml`).
			 *          In fact, these values ​​could not possibly match, as the lists are
			 *          of different lengths—containing 40, 41, and 54 entries, respectively—and
			 *          each format includes its own unique codes in addition to the shared ones
			 *
			 * @note Failures should be handled based on the name of the codec, not by a numerical code:
			 *       the types `ini::error_t`, `toml::error_t`, and `yaml::error_t` are distinct,
			 *       and comparing their numerical values ​​makes no sense whatsoever
			 *
			 * @note That is why some codes are not produced by the YAML codec:
			 *
			 * @note - `STORAGE_EXHAUSTED` is triggered from two locations, and the gap previously
			 *         mentioned here has been closed. The YAML tree stores offsets into the character
			 *         storage using four-byte values—consistent with INI, TOML, XML, CSV, and JSON
			 *         (verified on 2026-09-09 via the `span_t` structure used in all six formats,
			 *         each containing a `uint32_t offset` and a `uint32_t length`). This width
			 *         is now strictly enforced: both during the relocation of an entry (`deposit`)—to
			 *         safeguard the offset calculation itself—and as a gatekeeper at the start
			 *         of the parsing process, where any overflow results in an error
			 *
			 * @warning The note read, "has no output locations at all… moved outside the codec into
			 *          a separate report," and it was already there *after* the audit had been initiated.
			 *          On September 9, 2026, the output locations were counted: YAML has two, whereas the
			 *          `UNEXPECTED_EOF` and `OVERFLOW_LIMIT` errors for the INI codec, as well as the
			 *          `OVERFLOW_LIMIT` error for the TOML codec, have—in fact—zero
			 *
			 * @warning This leads to the same conclusion as in the case of an entry made after the end of the text:
			 *          an entry that declares a gap must be removed by the correction that closes that gap.
			 *          Otherwise, the information already processed would be handled a second time
			 *
			 * @warning The silence of code signifies that a path is UNVERIFIED, not that it is sound:
			 *          the mechanism for tracking silent code examines all points of execution across three separate runs,
			 *          and any code that has never been triggered is either a conditional guard clause or a path that has
			 *          not been covered by testing
			 *
			 * \~
			 */

			/**
			 * \~russian
			 * @brief Структура отрезка в хранилище знаков
			 *
			 * @details Отрезок хранит смещение и длину, а не указатель: хранилище растёт
			 *          дописыванием и вправе переехать в памяти, и указатель после переезда стал бы недействителен.
			 *
			 * \~english
			 * @brief Structure of a segment in a character store
			 *
			 * @details A segment holds an offset and a length rather than a pointer: the storage grows
			 *          by an appending and may move in the memory, and a pointer after the move would become invalid.
			 *
			 * \~
			 */
			using span_t = codec::span_t;
			/**
			 * \~russian
			 * @brief Кодировки исходного текста
			 *
			 * @details Описание предписывает **UTF-8**, **UTF-16** и **UTF-32** и дозволяет
			 *          метку порядка байтов в начале потока. Кодировка опознаётся по метке, а при её
			 *          отсутствии - по расположению нулевых байтов в первых четырёх октетах.
			 *
			 * @note Кодировки однобайтовые - ISO-8859-1, US-ASCII и Windows-1252 - описанием
			 *       не предписаны и заведены сверх него: файл, полученный от чужой оснастки, бывает
			 *       записан иначе, и отвергать его целиком значило бы оставлять потребителя без
			 *       всякого способа его прочесть. Метки порядка байтов они не имеют вовсе и
			 *       опознанию не поддаются: принимаются только навязанными извне
			 *
			 * \~english
			 * @brief Encodings of the source text
			 *
			 * @details The specification prescribes **UTF-8**, **UTF-16** and **UTF-32** and permits
			 *          a byte order mark at the beginning of a stream. The encoding is recognised by the mark, while at its
			 *          absence — by the arrangement of the zero bytes in the first four octets.
			 *
			 * @note The single-byte encodings — ISO-8859-1, US-ASCII and Windows-1252 — are not prescribed
			 *       by the specification and are made beyond it: a file received from a foreign tooling is
			 *       sometimes written otherwise, and to refuse it wholly would mean to leave the consumer
			 *       without any means to read it. They have no byte order marks at all and are not amenable
			 *       to recognition: they are accepted only as imposed from outside
			 *
			 * \~
			 */
			using encoding_t = codec::encoding_t;

			/**
			 * \~russian
			 * @brief Структура числа, разобранное из записи своей
			 *
			 * @details Разобранное кладётся во все три поля разом, а какое из них взято -
			 *          сказывает вид числа: целое со знаком лежит в первом, целое без знака во втором,
			 *          дробное в третьем.
			 *
			 * \~english
			 * @brief The structure of a number, broken down from its representation
			 *
			 * @details What is parsed is placed into all the three fields at once, and which of them is taken
			 *          is told by the kind of the number. An integer with a sign lies in the first,
			 *          an integer without a sign in the second, a fraction in the third.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Numeric {
				// Дробное число двойной точности
				double real;
				// Целое число со знаком
				int64_t integer;
				// Целое число без знака
				uint64_t natural;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Numeric() noexcept;
			} numeric_t;
			/**
			 * \~russian
			 * @brief Структура положения в исходном тексте
			 *
			 * @details Служит для указания места отказа и для привязки значений к исходному тексту.
			 *
			 * @note Номер строки и положение в строке считаются в знаках Юникода, а смещение -
			 *       в байтах текста, к UTF-8 приведённого и метки порядка байтов лишённого.
			 *       Для текста UTF-8 без метки смещение это есть смещение в поданных байтах;
			 *       текст с меткою мельче поданного на длину её, а текст UTF-16 либо UTF-32 -
			 *       на всю разницу двух кодировок
			 *
			 * \~english
			 * @brief Structure of the provision in the source text
			 *
			 * @details Serves for indicating the place of a refusal and for binding the values to the source text.
			 *
			 * @note The line number and the position in the line are counted in Unicode characters, while the offset —
			 *       in the bytes of the text brought to UTF-8 and deprived of the byte order mark. For a UTF-8 text
			 *       without a mark this offset is an offset in the bytes fed; a text with a mark is smaller than the
			 *       one fed by the length of that mark, while a UTF-16 or UTF-32 text — by the whole difference of
			 *       the two encodings
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location : codec::location_t {
				// Глубина вложенности, считая с нуля
				uint32_t depth;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Location() noexcept;
			} location_t;
			/**
			 * \~russian
			 * @brief Структура отметки времени
			 *
			 * @details Признаётся лишь схемою наречия 1.1: описание 1.2 отметок времени не
			 *          знает вовсе, и `2001-12-14` есть там обыкновенная строка.
			 *
			 * @note Построение отметки повторяет отметку контейнера TOML намеренно: разбирают
			 *       они одну и ту же запись по ISO 8601, и расходиться им незачем
			 *
			 * \~english
			 * @brief Timestamp structure
			 *
			 * @details It is recognised only by the schema of the 1.1 dialect: the 1.2 specification does not know the timestamps
			 *          at all, and `2001-12-14` is an ordinary string there.
			 *
			 * @note The construction of the timestamp repeats the timestamp of the TOML container deliberately: they parse
			 *       one and the same record by ISO 8601, and there is no reason for them to diverge
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stamp {
				// Признак того, что часовой пояс объявлен буквою Z
				bool zulu;
				// Признак того, что дата отделена от времени пробелом, а не буквою T
				bool spaced;
				// День отметки времени, считая с единицы
				uint8_t day;
				// Час отметки времени
				uint8_t hour;
				// Месяц отметки времени, считая с единицы
				uint8_t month;
				// Минута отметки времени
				uint8_t minute;
				// Секунда отметки времени
				uint8_t second;
				// Смещение часового пояса в минутах
				int16_t offset;
				// Год отметки времени
				int32_t year;
				// Доля секунды в наносекундах
				uint32_t nanoseconds;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Stamp() noexcept;
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
			 *
			 * @param error error code of the parsing
			 * @return      textual description of the error code
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
			 *
			 * @param kind kind of a node of a document
			 * @return     name of the kind of the node
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
			 *
			 * @note The name is issued for an exact kind rather than for a composite one: a composite kind cannot be
			 *       named, for it is a set of the kinds rather than a kind
			 *
			 * @param type kind of a value of a document
			 * @return     name of the kind of the value
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия события чтения
			 *
			 * @details Названия совпадают с названиями эталонного набора `yaml-test-suite`, и
			 *          сличение с ним ведётся по ним же.
			 *
			 * @param event вид события чтения
			 * @return      название события чтения
			 *
			 * \~english
			 * @brief Function of the obtaining of the name of an event of the reading
			 *
			 * @details The names coincide with the names of the reference set `yaml-test-suite`, and
			 *          the comparison with it is conducted by them.
			 *
			 * @param event kind of an event of the reading
			 * @return      name of the event of the reading
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const event_t event) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения названия кодировки
			 *
			 * @param encoding кодировка исходного текста
			 * @return         общепринятое название кодировки
			 *
			 * \~english
			 * @brief Function of getting the name of an encoding
			 *
			 * @param encoding encoding of the source text
			 * @return         commonly accepted name of the encoding
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const encoding_t encoding) noexcept;
			/**
			 * \~russian
			 * @brief Функция определения кодировки по её названию
			 *
			 * @details Договор этот один у INI, TOML и YAML: кодек, кодировки принимающий,
			 *          обязан уметь и назвать её, и опознать по названию - иначе потребитель, взявший
			 *          название из настроек, обратить его в кодировку ничем не может.
			 *
			 * @param text название кодировки в любом регистре
			 * @return     определённая кодировка исходного текста
			 *
			 * \~english
			 * @brief Function of determining an encoding by its name
			 *
			 * @details This contract is one for INI, TOML and YAML: a codec accepting the encodings is
			 *          obliged to be able both to name them and to recognise them by a name — otherwise a
			 *          consumer having taken a name from the settings cannot turn it into an encoding by
			 *          anything.
			 *
			 * @param text name of the encoding in any case
			 * @return     determined encoding of the source text
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ encoding_t encoding(const string_view text) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения вида узла по виду значения
			 *
			 * @details Вид узла есть огрубление вида значения до словаря самого описания:
			 *          всякое число, каким бы видом оно ни хранилось, есть узел вида `NUMBER`
			 *
			 * @param type вид значения документа
			 * @return     вид узла документа
			 *
			 * \~english
			 * @brief Function of the obtaining of the kind of a node by the kind of a value
			 *
			 * @details The kind of a node is a coarsening of the kind of a value down to the vocabulary of the specification itself:
			 *          every number, by whichever kind it is stored, is a node of the kind `NUMBER`
			 *
			 * @param type kind of a value of a document
			 * @return     kind of a node of a document
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ kind_t kind(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Функция разрешения вида скалярного значения по его записи
			 *
			 * @details Разрешение ведётся действующей схемой: одна и та же запись есть число
			 *          по наречию 1.1 и строка по наречию 1.2. Значение, обнесённое оградой, схеме не
			 *          подлежит вовсе - оно строка всегда.
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
			 *
			 * @details The resolution is conducted by the acting schema: one and the same record is a number
			 *          by the 1.1 dialect and a string by the 1.2 dialect. A value surrounded by a quoting is not subject to a schema
			 *          at all — it is always a string.
			 *
			 * @note A number is issued by the **composite** kind `NUMBER`: its exact width is decided by
			 *       the conversion of the record rather than by its kind, and is determined at the parsing
			 *       — by the very same order as in the JSON container, by the narrowest of the fitting
			 *       kinds
			 *
			 * @param text   record of a value being resolved
			 * @param schema acting schema of the resolution
			 * @return       kind of the value corresponding to the record
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ type_t resolve(const string_view text, const schema_t schema) noexcept;
			/**
			 * \~russian
			 * @brief Функция разбора записи числа к самому узкому вмещающему виду
			 *
			 * @details Разбор ведётся действующей схемой: `0777` есть 511 по наречию 1.1 и
			 *          строка по наречию 1.2, а `12:30` есть 750 там же. Знак подчёркивания между
			 *          разрядами наречие 1.1 дозволяет, и он снимается прежде разбора.
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
			 *
			 * @details The parsing is conducted by the acting schema: `0777` is 511 by the 1.1 dialect and
			 *          a string by the 1.2 dialect, while `12:30` is 750 there as well. The underscore
			 *          sign between the digits the 1.1 dialect allows, and it is removed before the parsing.
			 *
			 * @note The streaming reading does not parse a number at all and issues the composite kind `NUMBER`:
			 *       to parse every number not demanded by anyone would mean to pay for what the consumer did not ask for.
			 *       It is the tree which parses it at the building of its own — there the number is already kept,
			 *       and it needs to be parsed only once
			 *
			 * @param text   record of a number being parsed
			 * @param schema acting schema of the resolution
			 * @param result parsed number
			 * @return       kind of the parsed number, `UNDEFINED` — the record is not a number
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ type_t narrow(const string_view text, const schema_t schema, numeric_t & result) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки записи имени метки либо ссылки
			 *
			 * @details Имя метки не вправе нести пробельных знаков и знаков, открывающих
			 *          поточное построение: `[`, `]`, `{`, `}` и запятой.
			 *
			 * @param text проверяемое имя метки
			 * @return     признак допустимости имени метки
			 *
			 * \~english
			 * @brief Function of the checking of the notation of the name of an anchor or of an alias
			 *
			 * @details The name of an anchor may not carry the whitespace characters and the characters opening
			 *          a flow construction: `[`, `]`, `{`, `}` and the comma.
			 *
			 * @param text name of an anchor being checked
			 * @return     sign of the admissibility of the name of the anchor
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool anchored(const string_view text) noexcept;
			/**
			 * \~russian
			 * @brief Функция выбора вида записи скалярного значения
			 *
			 * @details Выбирается наименее навязчивый вид, каким значение записать возможно:
			 *          значение, читаемое обратно тем же самым без ограды, ограды и не получает.
			 *
			 * @warning Проверка эта - оборотная сторона разрешения видов, и вести её обязано
			 *          одно с ним правило: запись, которую чтение разрешит числом, обнести
			 *          оградою **необходимо**, иначе строка `12` вернётся числом
			 *
			 * @note Вида укладки на входе нет намеренно: приговор ведётся СТРОЖЕ обоих,
			 *       и знаки, значащие лишь в потоковой укладке - запятая, скобки круглые
			 *       и фигурные, - обносятся оградою и в блочной. Замер по шести таким
			 *       значениям в обеих укладках: приговор посредника с приговором записи
			 *       совпал везде. Строгость эта намеренна - иначе потребитель, укладку
			 *       сменивший, получил бы запись, читаемую иначе
			 *
			 * @param text   записываемое значение
			 * @param schema действующая схема разрешения
			 * @param key    признак того, что значение записывается именем пары
			 * @return       наименее навязчивый допустимый вид записи
			 *
			 * \~english
			 * @brief Function of the choice of the kind of the notation of a scalar value
			 *
			 * @details The least obtrusive kind by which the value can be written is chosen:
			 *          a value read back as the very same one without a quoting does not receive a quoting either.
			 *
			 * @warning This check is the reverse side of the resolution of the kinds, and one and the same rule
			 *          must conduct them both: a record which the reading will resolve as a number must **necessarily** be
			 *          surrounded by a quoting, otherwise the string `12` will come back as a number
			 *
			 * @note The kind of the layout at the input is absent deliberately: the verdict is conducted
			 *       STRICTER than both, and the characters meaning only in the flow layout — the comma,
			 *       the round and the curly brackets — are surrounded by a quoting even in the block one. A
			 *       measurement over six such values in both layouts: the verdict of the intermediary
			 *       coincided with the verdict of the writing everywhere. This strictness is deliberate —
			 *       otherwise a consumer having changed the layout would receive a record read otherwise
			 *
			 * @param text   value being written
			 * @param schema acting schema of the resolution
			 * @param key    sign of the fact that the value is written as the name of a pair
			 * @return       least obtrusive admissible kind of the notation
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ style_t quoting(const string_view text, const schema_t schema, const bool key) noexcept;
			/**
			 * \~russian
			 * @brief Функция выбора вида записи скалярного значения с указанным удержанием строкою
			 *
			 * @details Вид тот же, что и у собрата о трёх доводах, однако правило удержания
			 *          строкою здесь отключаемо: запись, разрешаемую числом, собрат обносит оградою
			 *          **необходимо**, дабы строка `12` числом не вернулась.
			 *
			 * @details Отключается правило то там, где значение строкою и не является:
			 *          запись числа, исходным текстом удержанная, обязана вернуться числом же, и
			 *          ограда ей не нужна вовсе. Потребна ей лишь та часть приговора, что говорит
			 *          о записи возможной: знак неотменимый, пробел ведущий либо завершающий,
			 *          знак укладки потоковой. Оттого правило и разделено доводом, а не переписано.
			 *
			 * @warning Довод `stringly` ложью ставить дозволено ЛИШЬ значению, строкою не
			 *          являющемуся: строка, приговор сей ложью получившая, вернулась бы
			 *          числом либо логическим значением
			 *
			 * @param text     записываемое значение
			 * @param schema   действующая схема разрешения
			 * @param key      признак того, что значение записывается именем пары
			 * @param stringly признак того, что значение удержать строкою необходимо
			 * @return         наименее навязчивый допустимый вид записи
			 *
			 * \~english
			 * @brief Function of the choice of the kind of the notation of a scalar value with the specified retention as a string
			 *
			 * @details The kind is the same as that of the counterpart of three arguments, however the rule
			 *          of the retention as a string is switchable here: a record resolved as a number is **necessarily**
			 *          surrounded by a quoting by the counterpart, so that the string `12` would not come back as a number.
			 *
			 * @details That rule is switched off there where the value is not a string at all: a record of a number
			 *          retained by the source text must come back as a number, and a quoting is not needed by it at
			 *          all. Only that part of the verdict which speaks of a possible record is needed by it: an
			 *          inescapable sign, a leading or a trailing space, a sign of a streaming layout. For that
			 *          reason the rule is divided by an argument, and is not rewritten.
			 *
			 * @warning The argument `stringly` is permitted to be set false ONLY for a value which is not a
			 *          string: a string which has received this argument as false would come back as a number
			 *          or as a logical value
			 *
			 * @param text     value being written
			 * @param schema   acting schema of the resolution
			 * @param key      sign of the fact that the value is written as the name of a pair
			 * @param stringly sign of the fact that it is necessary to retain the value as a string
			 * @return         least obtrusive admissible kind of the notation
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ style_t quoting(const string_view text, const schema_t schema, const bool key, const bool stringly) noexcept;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"
