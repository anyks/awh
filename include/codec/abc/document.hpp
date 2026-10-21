/**
 * @file document.hpp
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
 * @brief Заголовочный файл дерева документа бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the tree of a document of the ABC binary container
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
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "encoding.hpp"

/**
 * Подключаем заголовочный файл проекта
 */
#include "../../sys/macro/global.hpp"

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
			 * @brief Класс дерева документа
			 *
			 * @details Дерево лежит одним вместилищем узлов подряд: дети стоят сразу за
			 *          родителем, указаний на них нет вовсе. Переход к следующему соседу стоит
			 *          сложения, и пропуск вложенного построения целиком - тоже, ибо узел-вместимое
			 *          несёт размах своего поддерева.
			 *
			 * @details Содержимое всех строк, двоичных значений и имён полей лежит в одном
			 *          хранилище: имя не выделяет памяти вовсе, и весь документ обходится двумя
			 *          выделениями вместо выделения на всякий узел.
			 *
			 * @details **Намеренные решения.** Перечисленное ниже выбрано осознанно, и
			 *          возвращаться к этим вопросам при разборе кода не следует:
			 *
			 * @li **Дерево документа правки не имеет.** Разбор укладывает дерево, сборка
			 *     выводит из него запись, а правится содержимое на стороне владеющего значения.
			 *     Правка дерева завела бы второй путь укладки записи - мимо строгого вида, порога
			 *     укладки содержимого ссылкой, порога объявления размаха вместимого и подбора
			 *     метода сжатия: две дороги укладывать одно и то же, и честна перед настройками
			 *     сборки лишь одна. Тем ABC и расходится с текстовыми кодеками, где дерево есть
			 *     то, что правят, а текст - то, что выводят: у бинарного контейнера первична
			 *     запись, а дерево из неё выведено
			 *
			 * @note Перенос владеющего значения в дерево этим не отменён: `Value::graft` его
			 *       ведёт, но стоит он полного круга через октеты и правкою на месте не является.
			 *       Прямая же дорога такова: собрать значение вызовами `builder_t`, уложить его в
			 *       запись через `compose` либо `dump`, внести запись в контейнер через
			 *       `editor_t::append` с закреплением `commit`, а разобранное поднять обратно в
			 *       значение через `Value(const Document::value_t &)`
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 *          лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 *          равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 *          тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 *          четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 *          ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 *          свободно
			 *
			 * @note **О заслонах документа, ни разу не сработавших.** Сплошной щуп нужности
			 *       09.09.2026 обесточил порознь всякий заслон `document`, сохраняя условие
			 *       вычисляемым (`if((X) && false)`). Под сторожем 61 место, молчали 37, и молчание
			 *       сведено по существу зовомого:
			 *       - **шесть** в отсеве повторов имён стерегут ЦЕНУ, а не правильность: выбывшая
			 *       пара, сличённая вторично, выбывает вторично же, а перенос при пустом отсеве
			 *       отдаёт то же дерево. Работа растёт квадратом числа пар, и мера им - замер;
			 *       - **пять** суть запас хранилища октетов (`::room`): смещение узла несёт
			 *       тридцать два разряда, отказ требует настоящих четырёх гигабайт;
			 *       - **девять** НЕДОСТИЖИМЫ по устройству, и все девять одним доводом: дерево
			 *       заводится ЕДИНСТВЕННО разбором - работ, правящих его, у документа нет
			 *       вовсе, - а всякое значение, разбор прошедшее, сборщиком принимается.
			 *       Сюда же и оборванная пара отображения, и незакрытое вместимое: разбор
			 *       уравновешивает вместимые сам. Довод этот ПРОВЕРЕН опытом, а не выведен:
			 *       проверка, писанная под дерево с негодной кодировкой, покраснела на своей же
			 *       посылке - разбор поверяет кодировку всегда и такую запись отвергает сам;
			 *       - **прочие** затенены соседом либо берегут работу, и у каждого назван свой
			 *       довод при коде
			 *
			 * @note **Щуп НУЖНОСТИ (заслон снимается), 08.09.2026.** Обесточены порознь все
			 *       100 заслонов части, под сторожем 47. Молчали ПЯТЬДЕСЯТ ТРИ - доля высшая по
			 *       кодеку, и причина её в способе проверять: дерево поверяют КРУГОВЫМ обходом
			 *       (собрали, разобрали, сличили), а обход этот ходит дорогами удачными. Заслоны
			 *       же стоят на дорогах отказа, и круг их не касается, оставаясь зелёным.
			 *       Сведение молчавших:
			 *       - **одиннадцать** - `!valid()` у каждой работы ссылки порознь
			 *       (`TheInvalidCursorAnswersEmptinessToEverything`). Все одиннадцать закреплены
			 *       ПАДЕНИЕМ, привязанным отдельным щупом: ссылка в никуда получается путём
			 *       обычным, а спрашивать у непригодной прочее не пробовал никто;
			 *       - **три** - несоответствие вида (`TheDemandAgainstTheKindIsAnsweredWithEmptiness`):
			 *       имя поля у массива, порядок у строки, свой вид записи у неё же;
			 *       - **пара** обращения по номеру закреплена СОВМЕСТНО: `!container()` и
			 *       `index >= size()` затеняют друг друга, ибо у невместимого число значений
			 *       нулевое. Приговор о паре слабее одиночного;
			 *       - **шесть** отсева повторяющихся имён стерегут ЦЕНУ, а не правильность, и
			 *       проверкою не закрываются по существу - разбор при самой работе;
			 *       - **прочие** суть последние руки носителя и хранилища да места, объявленные
			 *       недостижимыми ранее
			 *
			 * @warning Воронка `Document::fail` чёрным ящиком НЕДОСТИЖИМА, и писанная было
			 *          под неё проверка оказалась ЗЕЛЕНА И СЛЕПА: донесение, какое видит подписчик
			 *          журнала, приходит от `Reader::fail`. Ныне она честно объявляет, что стережёт
			 *          разбирателя. Опора на чужое ДОНЕСЕНИЕ есть та же слепота, что и опора на
			 *          чужой заслон, но заметить её труднее: сообщение в журнале есть, и причина в
			 *          нём верная
			 *
			 * @warning **Затенение поверкой действительности бывает СЛУЧАЙНЫМ.** Три заслона
			 *          доступа - корень пустого дерева, обход по недействительной ссылке, обход по
			 *          пути - затенены `valid()` вправду: узел за концом дерева ею и отсеивается. Три
			 *          же ДРУГИХ - спрос имени за пределом отображения, обход ПУСТОГО вместимого и
			 *          спрос имени поля у ПЕРЕЧНЯ - не затенены ничем: за концом вместимого стоит узел
			 *          НАСТОЯЩИЙ, соседнее значение, и обход предела отдал бы его под чужим видом, а
			 *          `valid()` того не заметила бы - узел ведь в дереве есть. Набор молчал по одной
			 *          случайности во всех трёх: вместимое стояло в проверках ПОСЛЕДНИМ значением, и
			 *          за концом его лежал конец дерева. Закрыто
			 *          `CodecAbcDocument.TheDemandBeyondTheContainerDoesNotYieldTheNeighbour`, где
			 *          вместимое взято не последним нарочно
			 *
			 * @warning **Имена полей разного ВИДА повтором не являются.** Пустая строка и
			 *          пустые двоичные данные держат длину нулевую обе, содержимое обеих лежит в
			 *          хранилище октетов, и сличение содержимого их проходит по НУЛЮ октетов - то есть
			 *          совпадает всегда. Различает их единственно поверка вида, и без неё правило
			 *          `FIRST` отсеяло бы пару, повтором не являющуюся, а документ молча потерял бы
			 *          поле. Закреплено `CodecAbcDocument.KeysOfDifferentKindsAreNotDuplicates`
			 *
			 * \~english
			 * @brief Class of the tree of a document
			 *
			 * @details The tree lies as one container of the nodes in a row: the children stand right after
			 *          the parent, there are no pointers to them at all. The transition to the next neighbour costs
			 *          an addition, and the skipping of a nested construction as a whole costs it too, for a node-container
			 *          carries the extent of its own subtree.
			 *
			 * @details The content of all the strings, binary values and names of the fields lies in one
			 *          storage: a name does not allocate any memory at all, and the whole document makes do with two
			 *          allocations instead of an allocation for every node.
			 *
			 * @details **Deliberate decisions.** What is listed below was chosen knowingly, and one should
			 *          not return to these questions when reading the code:
			 *
			 * @li **The document tree has no editing.** The parser lays the tree out, the composer issues a
			 *     record from it, and the content is edited on the side of the owning value. An edit of the tree
			 *     would open a second road to lay a record out - bypassing the strict kind, the threshold of
			 *     laying content out by reference, the threshold of declaring the extent of a container, and the
			 *     choice of the compression method: two roads to lay out the same thing, and only one is honest
			 *     before the build settings. Therein ABC diverges from the text codecs, where the tree is what
			 *     is edited and the text is what is issued: for a binary container the record is primary, and
			 *     the tree is derived from it
			 *
			 * @note The transfer of an owning value into the tree is not cancelled by this: `Value::graft`
			 *       performs it, but it costs a full round trip through the octets and is not an in-place edit.
			 *       The direct road is this: build a value by the calls of `builder_t`, lay it into a record
			 *       through `compose` or `dump`, insert the record into the container through `editor_t::append`
			 *       with a `commit` fix, and lift the parsed one back into a value through
			 *       `Value(const Document::value_t &)`
			 *
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 *          holds a lock - it needs one for the timed fix by its own thread - and one must not judge
			 *          the others by it. A measurement of 2026-08-25, one `Fetcher` over four threads: thirteen
			 *          reports of TSan and nineteen records of four hundred read wrongly, silently. An own
			 *          object per thread over a SHARED source of the reading: zero reports, zero divergences -
			 *          the source is read, not edited, and is shared freely
			 *
			 * @note **Of the guards of the document, never once triggered.** The full audit of necessity of
			 *       2026-09-09 de-energized every `document` guard apart, keeping the condition computable
			 *       (`if((X) && false)`). Under the watch 61 sites stood silent in 37, and the silence is
			 *       reduced by the substance of what is called:
			 *       - **six** in the filter of name repetitions guard the COST, not the correctness: a pair that
			 *       has dropped out, compared a second time, drops out a second time as well, and a transfer with
			 *       an empty filter yields the same tree. The work grows as the square of the number of pairs,
			 *       and their measure is a measurement;
			 *       - **five** are the reserve of the octet storage (`::room`): the offset of a node carries
			 *       thirty-two bits, a failure demands a real four gigabytes;
			 *       - **nine** are UNREACHABLE by design, and all nine by one argument: the tree is created ONLY
			 *       by the parser - the document has no work that edits it at all - and any value that has passed
			 *       the parser is accepted by the composer. Here belong also the broken pair of a mapping and the
			 *       unclosed container: the parser balances containers itself. This argument is PROVEN by
			 *       experiment, not derived: a check written for a tree with a bad encoding went red on its own
			 *       premise - the parser verifies the encoding always and rejects such a record itself;
			 *       - **the rest** are shadowed by a neighbour or guard the work, and each has its own argument
			 *       named at the code
			 *
			 * @note **The NEEDNESS audit (a guard is removed), 2026-09-08.** All 100 guards of the
			 *       part were de-energized apart, under the watch 47. FIFTY-THREE stood silent - the
			 *       highest share across the codec, and its cause lies in the way of checking: the
			 *       tree is verified by a ROUND-TRIP walk (assembled, parsed, compared), and this walk
			 *       travels the successful roads. The guards stand on the roads of failure, and the
			 *       round trip does not touch them, staying green. The reduction of the silent ones:
			 *       - **eleven** - `!valid()` at each reference work apart
			 *       (`TheInvalidCursorAnswersEmptinessToEverything`). All eleven are fixed by a CRASH
			 *       caught by a separate probe: a link into nowhere is obtained by the usual means, and
			 *       no one had tried to ask an invalid reference for anything else;
			 *       - **three** - a kind mismatch (`TheDemandAgainstTheKindIsAnsweredWithEmptiness`):
			 *       a field name of an array, an ordering of a string, its own kind of a record of that
			 *       same string;
			 *       - **a pair** of accesses by number is fixed JOINTLY: `!container()` and
			 *       `index >= size()` shadow one another, for an incontinent value has a zero count.
			 *       The verdict on a pair is weaker than a solo one;
			 *       - **six** of the filter of repeating names guard the COST, not the correctness, and
			 *       are not closed by a check by the substance - parsing runs at the work itself;
			 *       - **the rest** are the last hands of the carrier and the storage and the places
			 *       declared unreachable earlier
			 *
			 * @warning The funnel `Document::fail` is UNREACHABLE as a black box, and a check once
			 *          written for it turned out GREEN AND BLIND: the report a log subscriber sees
			 *          comes from `Reader::fail`. Now it honestly declares that it guards the parser.
			 *          Relying on another's REPORT is the same blindness as relying on another's
			 *          guard, but it is harder to notice: there is a message in the log, and the cause
			 *          in it is correct
			 *
			 * @warning **Shadowing by the validity check can be ACCIDENTAL.** Three access guards - the root of
			 *          an empty tree, a walk by an invalid reference, a walk by a path - are truly shadowed by
			 *          `valid()`: a node past the end of the tree is filtered by it. But three OTHERS - a name
			 *          demand beyond a mapping, a walk of an EMPTY container, and a field-name demand of an ARRAY
			 *          - are shadowed by nothing: past the end of a container stands a REAL node, a neighbouring
			 *          value, and a walk past the bound would have issued it under a foreign kind, and `valid()`
			 *          would not have noticed - the node is in the tree after all. The set stood silent by one
			 *          coincidence in all three: the container stood in the checks as the LAST value, and past
			 *          its end lay the end of the tree. Closed by
			 *          `CodecAbcDocument.TheDemandBeyondTheContainerDoesNotYieldTheNeighbour`, where the container
			 *          is taken not the last on purpose
			 *
			 * @warning **Field names of different KINDS are not repetitions.** An empty string and empty binary
			 *          data both hold a zero length, the content of both lies in the octet storage, and the
			 *          comparison of their content passes over ZERO octets - that is, it always matches. They are
			 *          distinguished only by the kind check, and without it the `FIRST` rule would have filtered
			 *          a pair that is not a repetition, and the document would have silently lost a field. Fixed
			 *          by `CodecAbcDocument.KeysOfDifferentKindsAreNotDuplicates`
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Узел дерева документа
					 *
					 * @details Узел занимает двадцать октетов. Число кладётся в поле
					 *          содержимого переносом октетов, а не отдельным полем шириною в восемь
					 *          октетов: поле такое потребовало бы выравнивания, и узел вырос бы ещё.
					 *
					 * @details **Имя поля отображения есть отдельный узел, стоящий перед
					 *          значением.** У текстовых кодеков имя лежит полем узла, ибо именем стоит
					 *          лишь строка; здесь же именем вправе стоять значение любого вида, кроме
					 *          вместимого, и полем узла его было бы не уложить. Оттого отображение из
					 *          `N` пар несёт `2N` детей, а признак `keyed` отличает имя от значения.
					 *
					 * \~english
					 * @brief Node of the tree of a document
					 *
					 * @details A node occupies twenty octets. A number is placed into the field
					 *          of the content by a transfer of the octets rather than by a separate field eight
					 *          octets wide: such a field would demand an alignment, and the node would grow further.
					 *
					 * @details **The name of a field of a mapping is a separate node standing before the value.**
					 *          For the text codecs a name lies as a field of the node, because a name there is only ever
					 *          a string; here a value of any kind but a container has the right to stand as a name, and it
					 *          could not be laid as a field of the node. Therefore a mapping of `N` pairs carries `2N`
					 *          children, and the `keyed` flag distinguishes a name from a value.
					 *
					 * \~
					 */
					typedef struct Node {
						// Вид значения узла документа
						type_t type;
						/**
						 * \~russian
						 * Признак того, что узел является именем поля отображения
						 *
						 * @note Признак этот необходим: имя стоит тем же узлом, что и значение,
						 *       и без него первое значение отображения было бы неотличимо от имени
						 *
						 * \~english
						 * Flag that the node is the name of a field of a mapping
						 *
						 * @note This flag is necessary: a name stands as the same node as a value,
						 *       and without it the first value of a mapping would be indistinguishable from a name
						 *
						 * \~
						 */
						bool keyed;
						// Признак того, что величина числа меньше нуля
						bool negative;
						/**
						 * \~russian
						 * Признак того, что поддерево узла ещё не развёрнуто
						 *
						 * @note Задел под отложенный разбор: узел-вместимое вправе хранить
						 *       лишь отрезок записи своего поддерева, а дети его заводятся при
						 *       первом обращении. Обход дерева обязан спрашивать признак этот
						 *       прежде обращения к детям
						 *
						 * \~english
						 * Flag that the subtree of the node is not yet expanded
						 *
						 * @note Groundwork for the deferred parsing: a node-container has the right to hold
						 *       only the segment of the record of its subtree, while its children are created at the
						 *       first access. The traversal of the tree must ask this flag before an access to the children
						 *
						 * \~
						 */
						bool pending;
						// Смещение содержимого значения в хранилище октетов
						uint32_t offset;
						/**
						 * \~russian
						 * Содержимое узла шириною в восемь октетов
						 *
						 * @details Восемь этих октетов служат узлу по-разному, смотря по виду
						 *          значения его. У вместимого это пара чисел: количество детей и размах
						 *          поддерева. У строки и у двоичных данных - длина содержимого. У числа
						 *          родного вида - само число, готовое к выдаче. У числа неограниченной
						 *          ширины - длина октетов величины его.
						 *
						 * \~english
						 * Content of the node eight octets wide
						 *
						 * @details These eight octets serve the node differently, depending on the kind
						 *          of its value. For a container it is a pair of numbers: the number of the children and the extent
						 *          of the subtree. For a string and for binary data it is the length of the content. For a number
						 *          of a native kind it is the number itself ready for the issuance. For a number of an unlimited
						 *          width it is the length of the octets of its magnitude.
						 *
						 * \~
						 */
						uint32_t content[2];
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместимого либо длины содержимого
						 *
						 * @return количество детей вместимого либо длина содержимого в октетах
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the content
						 *
						 * @return number of the children of a container or the length of the content in octets
						 *
						 * \~
						 */
						AWH_ABC_INLINE bool container() const noexcept {
							// Выводим признак того, что узел является вместимым
							return ((static_cast <uint32_t> (this->type) & static_cast <uint32_t> (type_t::CONTAINER)) != 0);
						}
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместимого либо длины содержимого
						 *
						 * @return количество детей вместимого либо длина содержимого в октетах
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the content
						 *
						 * @return number of the children of a container or the length of the content in octets
						 *
						 * \~
						 */
						AWH_ABC_INLINE uint32_t length() const noexcept {
							// Выводим количество детей вместимого либо длину содержимого
							return this->content[0];
						}
						/**
						 * \~russian
						 * @brief Метод получения размаха поддерева узла
						 *
						 * @return размах поддерева узла в узлах, считая сам узел
						 *
						 * \~english
						 * @brief Method of the obtaining of the extent of the subtree of a node
						 *
						 * @return extent of the subtree of the node in the nodes counting the node itself
						 *
						 * \~
						 */
						AWH_ABC_INLINE uint32_t extent() const noexcept {
							/**
							 * Если узел вместимым не является, размах его равен одному узлу.
							 *
							 * Хранить размах у одиночного узла нельзя: поле содержимого занято
							 * у него самим числом, и запись размаха затёрла бы старшую половину
							 * его разрядов. Оттого размах одиночного узла подразумевается, а
							 * не хранится
							 */
							if(!this->container())
								// Выводим размах поддерева одиночного узла
								return 1;
							// Выводим размах поддерева вместимого
							return this->content[1];
						}
						/**
						 * \~russian
						 * @brief Метод установки количества детей вместимого либо длины содержимого
						 *
						 * @param value устанавливаемое значение
						 *
						 * \~english
						 * @brief Method of the setting of the number of the children of a container or of the length of the content
						 *
						 * @param value value being set
						 *
						 * \~
						 */
						AWH_ABC_INLINE void length(const uint32_t value) noexcept {
							// Выполняем установку количества детей вместимого либо длины содержимого
							this->content[0] = value;
						}
						/**
						 * \~russian
						 * @brief Метод установки размаха поддерева узла
						 *
						 * @param value устанавливаемое значение
						 *
						 * \~english
						 * @brief Method of the setting of the extent of the subtree of a node
						 *
						 * @param value value being set
						 *
						 * \~
						 */
						AWH_ABC_INLINE void extent(const uint32_t value) noexcept {
							// Выполняем установку размаха поддерева узла
							this->content[1] = value;
						}
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Node() noexcept;
					} node_t;
					/**
					 * \~russian
					 * @brief Сторож ширины узла дерева документа
					 *
					 * @details Ширина эта объявлена договором класса числом («узел занимает
					 *          двадцать октетов»), а поверить её проверкой НЕЛЬЗЯ: тип узла закрыт, и
					 *          снаружи он не виден. Число же при доводе о плотном дереве держится ничем -
					 *          довод («дети лежат сразу за родителем, переход к соседу стоит сложения»)
					 *          от точной ширины не зависит вовсе, и поле, добавленное по надобности,
					 *          растило бы узел МОЛЧА, оставляя договор устаревшим.
					 *
					 * @note Сторож стоит внутри работы класса нарочно: там закрытый тип виден.
					 *       Поле, прибавленное к узлу, не даст собрать кодек - рост становится
					 *       решением осознанным, а не случившимся по дороге. Ширина эта переносима:
					 *       поля узла суть перечень, три признака и два числа по четыре октета, а
					 *       указателей и восьмиоктетных полей у него нет вовсе
					 *
					 * \~english
					 * @brief Guard of the width of a node of the tree of a document
					 *
					 * @details This width is declared by the contract of the class as a number, and it cannot
					 *          be checked by a test: the type of a node is closed and is not visible from the outside.
					 *          Yet the number rests on nothing beside the argument about a dense tree - the argument
					 *          ("the children lie right after the parent, a step to a neighbour costs an addition") does
					 *          not depend on the exact width at all, and a field added as needed would have grown the node
					 *          SILENTLY, leaving the contract stale.
					 *
					 * @note The guard stands inside the work of the class on purpose: there the closed type is visible.
					 *       A field added to the node would not let the codec build - the growth becomes a knowing
					 *       decision rather than one that happened on the way. This width is portable: the fields of a
					 *       node are an enum, three flags and two numbers four octets each, and it has no pointers and no
					 *       eight-octet fields at all
					 *
					 * \~
					 */
					static_assert(sizeof(Node) == 20, "ABC: ширина узла дерева документа переменилась, договор класса устарел");
				public:
					/**
					 * \~russian
					 * @brief Структура настроек двоичного контейнера
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать контейнер
					 *          с одними пределами и записать его иным видом - обыкновенное дело, и
					 *          связывать их незачем.
					 *
					 * @note Построение это заведено ради согласия кодеков рамки: настройки дерева
					 *       ставятся одним ходом `settings()`, а вид их свой у каждого кодека
					 *
					 * \~english
					 * @brief Binary container settings structure
					 *
					 * @details The settings of the parsing and of the writing are kept separately: to read
					 *          the container with one limits and to write it in another form is an ordinary
					 *          affair, and there is no sense in binding them.
					 *
					 * @note This construction is made for the sake of the consent of the codecs of the framework:
					 *       the settings of a tree are set by the single `settings()` step, while their
					 *       form is own to each codec
					 *
					 * \~
					 */
					typedef struct Settings {
						// Настройки записи контейнера
						writer_t::settings_t writer;
						// Настройки разбора контейнера
						reader_t::settings_t reader;
					} settings_t;
					/**
					 * \~russian
					 * @brief Класс ссылки на значение документа
					 *
					 * @details Ссылка не владеет ничем и живёт не дольше документа. Извлечение
					 *          сличает само значение с пределами затребованного вида, а не вид хранения
					 *          с видом затребованным.
					 *
					 * \~english
					 * @brief Class of a reference to a value of a document
					 *
					 * @details A reference owns nothing and lives no longer than the document. The extraction
					 *          compares the value itself with the limits of the demanded kind rather than the kind of the storage
					 *          with the demanded kind.
					 *
					 * \~
					 */
					typedef class __AWH_SHARED_EXPORT__ Value {
						private:
							/**
							 * \~russian
							 * Номер узла за последним узлом вместимого, какому значение принадлежит
							 *
							 * @details Указания на родителя узел не несёт, а переход к соседу обязан
							 *          останавливаться на границе своего вместимого: без границы обход массива
							 *          продолжился бы соседом родителя его.
							 *
							 * \~english
							 * Index of the node past the last node of the container to which the value belongs
							 *
							 * @details A node does not carry a pointer to its parent, while the transition to a sibling is obliged
							 *          to stop at the boundary of its own container: without the boundary the traversal of an array
							 *          would continue with the sibling of its parent.
							 *
							 * \~
							 */
							uint32_t _bound;
						private:
							// Номер узла значения в дереве документа
							uint32_t _index;
						private:
							// Документ, которому значение принадлежит
							const Document * _doc;
						public:
							/**
							 * \~russian
							 * @brief Метод проверки действительности ссылки
							 *
							 * @return признак действительности ссылки
							 *
							 * \~english
							 * @brief Method of the checking of the validity of a reference
							 *
							 * @return sign of the validity of the reference
							 *
							 * \~
							 */
							bool valid() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида значения
							 *
							 * @return вид значения документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of a value
							 *
							 * @return kind of the value of the document
							 *
							 * \~
							 */
							type_t type() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида узла
							 *
							 * @return вид узла документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of a node
							 *
							 * @return kind of the node of the document
							 *
							 * \~
							 */
							kind_t kind() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки принадлежности значения к виду
							 *
							 * @param type вид значения, сборный либо точный
							 * @return     признак принадлежности значения к виду
							 *
							 * \~english
							 * @brief Method of the checking of the belonging of a value to a kind
							 *
							 * @param type kind of the value, composite or exact
							 * @return     sign of the belonging of the value to the kind
							 *
							 * \~
							 */
							bool is(const type_t type) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения количества значений вместимого
							 *
							 * @return количество значений вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the values of a container
							 *
							 * @return number of the values of the container
							 *
							 * \~
							 */
							size_t size() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения значения вместимого по его номеру
							 *
							 * @param index номер значения вместимого
							 * @return      ссылка на значение вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of a value of a container by its number
							 *
							 * @param index number of the value of the container
							 * @return      reference to the value of the container
							 *
							 * \~
							 */
							Value at(const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени поля отображения по его номеру
							 *
							 * @details Имя поля выдаётся ссылкою на значение: именем вправе стоять
							 *          не только строка, но и число, и отметка времени, и опознаватель.
							 *
							 * @param index номер пары отображения
							 * @return      ссылка на имя поля отображения
							 *
							 * \~english
							 * @brief Method of the extraction of the name of a field of a mapping by its number
							 *
							 * @details The name of a field is issued by a reference to a value: not only a string has the right
							 *          to stand as a name, but also a number, and a time stamp, and an identifier.
							 *
							 * @param index number of the pair of the mapping
							 * @return      reference to the name of the field of the mapping
							 *
							 * \~
							 */
							Value key(const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения первого значения вместимого
							 *
							 * @details Обход вместимого ведётся первым значением вместе с переходом к
							 *          соседу: обращение по номеру пропускает узлы, стоящие до затребованного,
							 *          и обход им обошёлся бы дороже квадрата.
							 *
							 * @note У отображения первым значением стоит имя первого поля: имя есть
							 *       такой же узел, и признак `keyed` отличает его от значения
							 *
							 * @return ссылка на первое значение вместимого
							 *
							 * \~english
							 * @brief Method of the extraction of the first value of a container
							 *
							 * @details The traversal of a container is conducted by the first value together with the transition to
							 *          a sibling: an access by a number skips the nodes standing before the demanded one,
							 *          and a traversal by it would cost more than a square.
							 *
							 * @note For a mapping the first value is the name of the first field: a name is
							 *       the same kind of node, and the flag `keyed` distinguishes it from a value
							 *
							 * @return reference to the first value of the container
							 *
							 * \~
							 */
							Value begin() const noexcept;
							/**
							 * \~russian
							 * @brief Метод перехода к следующему значению вместимого
							 *
							 * @details Переход стоит одного сложения: вложенное вместимое пропускается
							 *          целиком по размаху своего поддерева.
							 *
							 * @return ссылка на следующее значение вместимого
							 *
							 * \~english
							 * @brief Method of the transition to the next value of a container
							 *
							 * @details The transition costs one addition: a nested container is skipped
							 *          as a whole by the extent of its subtree.
							 *
							 * @return reference to the next value of the container
							 *
							 * \~
							 */
							Value next() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки того, что значение является именем поля отображения
							 *
							 * @return признак того, что значение является именем поля отображения
							 *
							 * \~english
							 * @brief Method of the checking that a value is the name of a field of a mapping
							 *
							 * @return sign that the value is the name of a field of a mapping
							 *
							 * \~
							 */
							bool keyed() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки того, что две ссылки ведут к ОДНОМУ узлу дерева
							 *
							 * @details Ход этот отвечает на вопрос ТОЖДЕСТВА, а не равенства: истину он
							 *          даёт лишь тогда, когда обе ссылки указывают на один и тот же узел одного и
							 *          того же дерева. Два разных узла с одинаковым содержимым тождественными НЕ
							 *          являются, и ход этот их различает.
							 *
							 * @note Назван он словом, а не записан оператором `==`, намеренно. У
							 *       владеющего значения `Value` оператор этот означает РАВЕНСТВО
							 *       содержимого, и заведи мы здесь оператор с иным смыслом - два одинаковых
							 *       написания в одном кодеке означали бы разное, а различить их читающий
							 *       мог бы лишь по виду левого довода. Ловушка эта молчалива, и заводить
							 *       её ради краткости не стоит
							 *
							 * @note Договор замкнутости обхода требует именно тождества: перечисление и
							 *       розыск обязаны вести к одному узлу, а сличение содержимого того не
							 *       докажет - два разных узла вправе нести одно и то же
							 *
							 * @param value сличаемая ссылка на значение
							 * @return      признак того, что обе ссылки ведут к одному узлу
							 *
							 * \~english
							 * @brief Method of the checking that two references lead to the SAME node of a tree
							 *
							 * @details This answers the question of the IDENTITY rather than of the equality: it yields truth
							 *          only when both references point to the very same node of the very same tree. Two different
							 *          nodes with the same content are NOT identical, and this method distinguishes them.
							 *
							 * @note It is named by a word rather than written as the `==` operator, on purpose. For the owning
							 *       value `Value` this operator means the EQUALITY of the content, and had we introduced an
							 *       operator with a different meaning here, two identical spellings in one codec would mean
							 *       different things, and a reader could tell them apart only by the look of the left argument.
							 *       This trap is silent, and it is not worth introducing for the sake of brevity
							 *
							 * @note The contract of the closure of a walk requires exactly identity: enumeration and lookup must
							 *       lead to one node, and a comparison of the content will not prove it - two different nodes
							 *       have the right to carry the same thing
							 *
							 * @param value reference to a value being compared
							 * @return      flag that both references lead to the same node
							 *
							 * \~
							 */
							bool same(const Value & value) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения значения поля отображения по имени
							 *
							 * @param name имя поля отображения
							 * @return     ссылка на значение поля отображения
							 *
							 * \~english
							 * @brief Method of the extraction of a value of a field of a mapping by a name
							 *
							 * @param name name of the field of the mapping
							 * @return     reference to the value of the field of the mapping
							 *
							 * \~
							 */
							Value get(const string_view name) const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки наличия поля отображения по имени
							 *
							 * @details Ход зовётся `contains`, а не `has`, по укладу рамки: у ДОКУМЕНТА
							 *          наличие по ПУТИ спрашивается словом `has`, а наличие вложенного по ИМЕНИ -
							 *          словом `contains`, у какого бы взгляда на дерево о том ни спрашивали.
							 *          Разделение не прихоть: у разметки слово `has` занято под наличие СВОЙСТВА
							 *          узла, величины иной, и сведи оба хода к одному имени - потребитель попадал
							 *          бы в ход об ином предмете.
							 *
							 * @note Курсор этот звался `has` до 04.09.2026, и расхождение вскрылось
							 *       сличением состава ходов двух взглядов, а не проверкою: договор об
							 *       ИМЕНИ проверкою поведения не стережётся вовсе - ход, названный
							 *       иначе, работает верно и свои проверки проходит. Закреплено вызовом
							 *       в CodecContract.PresenceOfANestedValueIsAskedByOneWord
							 *
							 * @param name имя поля отображения
							 * @return     признак наличия поля отображения
							 *
							 * \~english
							 * @brief Method of the checking of the presence of a field of a mapping by a name
							 *
							 * @details The method is called `contains`, not `has`, by the layout of the frame: for the DOCUMENT,
							 *          presence BY A PATH is asked by the word `has`, while the presence of a nested value BY A
							 *          NAME is asked by the word `contains`, at whichever view of the tree one asks about it. The
							 *          separation is not a whim: for a markup the word `has` is taken under the presence of a
							 *          PROPERTY of a node, a different quantity, and had both methods been reduced to one name, a
							 *          consumer would have hit a method about a different subject.
							 *
							 * @note This cursor was called `has` until 2026-09-04, and the divergence was revealed by comparing
							 *       the composition of the methods of the two views, not by a check: a contract about a NAME is
							 *       not guarded by a behaviour check at all - a method named otherwise works correctly and passes
							 *       its checks. Fixed by a call in CodecContract.PresenceOfANestedValueIsAskedByOneWord
							 *
							 * @param name name of the field of the mapping
							 * @return     sign of the presence of the field of the mapping
							 *
							 * \~
							 */
							bool contains(const string_view name) const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения логического значения
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a logical value
							 *
							 * @param result value being extracted
							 * @return       sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(bool & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом целого без знака
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of an integer without a sign
							 *
							 * @param result value being extracted
							 * @return       sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом целого со знаком
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of an integer with a sign
							 *
							 * @param result value being extracted
							 * @return       sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом дробного
							 *
							 * @param result извлекаемое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind of a fractional one
							 *
							 * @param result value being extracted
							 * @return       sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(double & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения содержимого значения
							 *
							 * @details Выдаётся содержимое строки, двоичных данных, опознавателя
							 *          либо октеты величины числа неограниченной ширины.
							 *
							 * @return содержимое значения
							 *
							 * \~english
							 * @brief Method of the extraction of the content of a value
							 *
							 * @details The content of a string, of binary data, of an identifier or
							 *          the octets of the magnitude of a number of an unlimited width is issued.
							 *
							 * @return content of the value
							 *
							 * \~
							 */
							string_view data() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения десятичного порядка величины
							 *
							 * @return десятичный порядок величины
							 *
							 * \~english
							 * @brief Method of the extraction of the decimal exponent of a magnitude
							 *
							 * @return decimal exponent of the magnitude
							 *
							 * \~
							 */
							int64_t exponent() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения номера подвида открытого расширения
							 *
							 * @note Номер этот принадлежит потребителю целиком: модуль его не толкует
							 *
							 * @return номер подвида открытого расширения
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the subtype of an open extension
							 *
							 * @note This number belongs to the consumer entirely: the module does not interpret it
							 *
							 * @return number of the subtype of the open extension
							 *
							 * \~
							 */
							uint64_t subtype() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки того, что величина меньше нуля
							 *
							 * @return признак того, что величина меньше нуля
							 *
							 * \~english
							 * @brief Method of the checking that a magnitude is less than zero
							 *
							 * @return sign that the magnitude is less than zero
							 *
							 * \~
							 */
							bool negative() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Конструктор
							 *
							 * \~english
							 * @brief Constructor
							 *
							 * \~
							 */
							explicit Value() noexcept;
							/**
							 * \~russian
							 * @brief Конструктор
							 *
							 * @param doc   документ, которому значение принадлежит
							 * @param index номер узла значения в дереве документа
							 * @param bound номер узла за последним узлом вместимого, какому значение принадлежит
							 *
							 * \~english
							 * @brief Constructor
							 *
							 * @param doc   document the value belongs to
							 * @param index number of the node of the value in the tree of the document
							 * @param bound index of the node past the last node of the container to which the value belongs
							 *
							 * \~
							 */
							Value(const Document * doc, const uint32_t index, const uint32_t bound) noexcept;
					} value_t;
				private:
					/**
					 * Правило обращения с повторяющимся именем поля отображения
					 *
					 * Правило это живёт состоянием сборки, а не документом: применяется оно в миг
					 * закрытия отображения, а собранному документу знать о нём уже незачем
					 */
					duplicate_t _duplicates;
				private:
					// Объект потокового чтения записей
					mutable reader_t _reader;
					// Объект записи событий
					mutable writer_t _writer;
				private:
					// Код отказа разбора записи
					mutable error_t _error;
				private:
					// Хранилище содержимого значений и имён полей
					string _storage;
				private:
					// Вместилище узлов дерева документа
					vector <node_t> _nodes;
				private:
					/**
					 * \~russian
					 * Место, где отказ разбора произошёл
					 *
					 * @details Место снимается у разбирателя при отказе: разбиратель заводится
					 *          разбором и им же разрушается, и без переноса место его пропадало бы.
					 *
					 * \~english
					 * Location where the failure of the parsing has occurred
					 *
					 * @details The location is taken from the parser upon a failure: the parser is created by the
					 *          parsing and destroyed by it too, and without a transfer its location would be lost.
					 *
					 * \~
					 */
					location_t _location;
				private:
					// Указатели имён полей отображений, заводимые по требованию
					mutable unordered_map <uint32_t, unordered_map <string_view, uint32_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод заведения указателя имён полей отображения
					 *
					 * @param index номер узла отображения в дереве документа
					 * @return      заведённый указатель имён полей
					 *
					 * \~english
					 * @brief Method of the creation of an index of the names of the fields of a mapping
					 *
					 * @param index number of the node of the mapping in the tree of the document
					 * @return      created index of the names of the fields
					 *
					 * \~
					 */
					const unordered_map <string_view, uint32_t> & naming(const uint32_t index) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сличения двух поддеревьев дерева документа
					 *
					 * @details Сличение ведётся по значению узлов, а не по записи, их
					 *          породившей: дерево записи уже не помнит. Оттого два имени, писанные
					 *          разной шириною метки, но несущие одно число, сличаются РАВНЫМИ, тогда
					 *          как отказ `duplicate_t::REFUSE` у разбирателя сличает полную запись и
					 *          счёл бы их различными. Расхождение это намеренно: правила `FIRST` и
					 *          `LAST` выбирают между ЗНАЧЕНИЯМИ, и ширина метки к выбору отношения
					 *          не имеет. Расхождение это закреплено проверкой
					 *          `CodecAbcDocument.KeyIdentityDivergesByLayer`: она утверждает ОБЕ
					 *          стороны его разом - согласие разбирателя и потерю пары деревом на
					 *          одной и той же записи.
					 *
					 * @param left  номер узла первого сличаемого поддерева
					 * @param right номер узла второго сличаемого поддерева
					 * @return      признак совпадения поддеревьев
					 *
					 * \~english
					 * @brief Method of the comparison of two subtrees of the tree of a document
					 *
					 * @details The comparison is conducted by the value of the nodes rather than by the record
					 *          that produced them: the tree no longer remembers the record. Therefore two names written
					 *          with a label of a different width but carrying one number are compared as EQUAL, whereas
					 *          the refusal `duplicate_t::REFUSE` at the parser compares the full record and would have
					 *          counted them as different. This divergence is deliberate: the `FIRST` and `LAST` rules choose
					 *          between VALUES, and the width of a label bears no relation to the choice. This divergence
					 *          is fixed by a check `CodecAbcDocument.KeyIdentityDivergesByLayer`: it asserts BOTH sides of
					 *          it at once - the agreement of the parser and the loss of a pair by the tree on the very same
					 *          record.
					 *
					 * @param left  number of the node of the first subtree being compared
					 * @param right number of the node of the second subtree being compared
					 * @return      sign of the coincidence of the subtrees
					 *
					 * \~
					 */
					bool identical(const uint32_t left, const uint32_t right) const noexcept;
					/**
					 * \~russian
					 * @brief Метод применения правила повтора имени к закрытому отображению
					 *
					 * @details Правило применяется в миг закрытия отображения оттого, что
					 *          узлы его о ту пору суть ХВОСТ перечня: ничего построенного позади них
					 *          ещё нет, и изъятие пары не двигает ни одного стороннего узла. Размах
					 *          поддерева хранится длиною, а не указателем, оттого уцелевшие пары
					 *          переносятся на новое место как есть.
					 *
					 * @note Потоковый разбор правил `FIRST` и `LAST` осуществить не может:
					 *       выбор между первым и последним требует видеть отображение целиком, а
					 *       события выдаются по одному и назад разбор не ходит. Оттого правила эти
					 *       живут деревом, а разбиратель их пропускает наравне с `KEEP`
					 *
					 * @param index номер узла закрываемого отображения
					 * @param rule  правило обращения с повторяющимся именем поля
					 *
					 * \~english
					 * @brief Method of the application of the rule of a repeating name to a closed mapping
					 *
					 * @details The rule is applied at the moment of the closing of a mapping because its nodes
					 *          are the TAIL of the list by that time: nothing built behind them exists yet, and the removal
					 *          of a pair moves no foreign node. The extent of a subtree is stored as a length rather than as
					 *          a pointer, so the surviving pairs are transferred to the new place as they are.
					 *
					 * @note A streaming parse cannot carry out the `FIRST` and `LAST` rules: the choice between the first
					 *       and the last requires seeing the mapping in full, while the events are issued one by one and the
					 *       parser does not step back. Therefore these rules live in the tree, and the parser skips them on
					 *       a par with `KEEP`
					 *
					 * @param index number of the node of the mapping being closed
					 * @param rule  rule of the handling of a repeating name of a field
					 *
					 * \~
					 */
					void resolve(const uint32_t index, const duplicate_t rule) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа разбора документа
					 *
					 * @details Донесение идёт отсюда, из единственного места объявления отказа:
					 *          работа отвечает отказом множеством путей, и запись в каждом из них
					 *          разошлась бы с прочими. Отказ, ПРИНЯТЫЙ от нижнего слоя, сюда не идёт -
					 *          тот слой донёс о нём сам, и второе донесение лишь двоило бы записи.
					 *
					 * @param error объявляемый код отказа
					 * @return      признак успешности, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a failure
					 *
					 * @details The report comes from here, from the single place of the declaration of a failure: the
					 *          work answers with a failure by many roads, and a record at each of them would diverge from
					 *          the others. A failure ACCEPTED from a lower layer does not come here - that layer has
					 *          reported it itself, and a second report would only double the records.
					 *
					 * @param error code of the failure being declared
					 * @return      flag of the success, always false
					 *
					 * \~
					 */
					bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия собранного содержимого документа
					 *
					 * @details Снятие это отличается от очистки одним, и различие это существенно:
					 *          код отказа и место его здесь СОХРАНЯЮТСЯ. Зовётся оно с путей отказа, где
					 *          содержимое обязано уйти, а причина - остаться; очистка же, сбрасывающая и
					 *          причину, зовётся с путей, где документ снимается по заказу потребителя.
					 *
					 * @note Дороги эти близнецы, и разошлись бы они молча, зови отказ очистку:
					 *       потребитель, спросивший причину после отказа, получил бы «нет ошибки»
					 *
					 * @note Разобрано подменами 04.09.2026, и разбор уточняет довод: на самом
					 *       месте вызова замена `discard` очисткою БЕЗВРЕДНА, ибо причина пишется
					 *       ПОСЛЕ снятия. Опасность живёт не в выборе работы, а в ПОРЯДКЕ - и вот
					 *       она стережётся: очистка, поставленная после записи причины, краснит
					 *       пять проверок (`CodecAbcDocument.Failures`, `.ErrorLocationReachesTheConsumer`,
					 *       `.EmptyRecordParsingNamesItsCause`, `.DuplicateRules`, `.DuplicateRulesByKeyKind`)
					 *
					 * \~english
					 * @brief Method of the dropping of the assembled content of a document
					 *
					 * @details This dropping differs from the reset in one thing, and that difference is essential: the
					 *          code of the failure and its location are KEPT here. It is called from the roads of failure,
					 *          where the content must go away while the cause must remain; whereas the reset, which also
					 *          drops the cause, is called from the roads where the document is dropped at the consumer's
					 *          request.
					 *
					 * @note These roads are twins, and they would have diverged silently had the failure called the
					 *       reset: a consumer who asked the cause after a failure would have got "no error"
					 *
					 * @note Resolved by substitutions on 2026-09-04, and the resolution sharpens the argument: at the
					 *       actual call site replacing `discard` with the reset is HARMLESS, because the cause is written
					 *       AFTER the dropping. The danger lives not in the choice of the work but in the ORDER - and here
					 *       it is guarded: a reset placed after the cause is written turns five checks red
					 *       (`CodecAbcDocument.Failures`, `.ErrorLocationReachesTheConsumer`,
					 *       `.EmptyRecordParsingNamesItsCause`, `.DuplicateRules`, `.DuplicateRulesByKeyKind`)
					 *
					 * \~
					 */
					void discard() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод приёма события разбора, выданного прямо из чтения
					 *
					 * @param context указание на состояние сборки дерева документа
					 * @param reader  разбиратель бинарной записи
					 * @param event   вид принимаемого события разбора
					 *
					 * \~english
					 * @brief Method of the reception of an event of the parsing issued directly from the reading
					 *
					 * @param context pointer to the state of the assembling of the tree of the document
					 * @param reader  parser of a binary record
					 * @param event   kind of the received event of the parsing
					 *
					 * \~
					 */
					static void assemble(void * context, reader_t & reader, const event_t event) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки дерева документа по событию разбора
					 *
					 * @param context указание на состояние сборки дерева документа
					 * @param self    документ, чьё дерево собирается
					 * @param reader  разбиратель бинарной записи
					 * @param event   вид принимаемого события разбора
					 * @return        признак успешности сборки дерева
					 *
					 * \~english
					 * @brief Method of the assembling of the tree of a document by an event of the parsing
					 *
					 * @param context pointer to the state of the assembling of the tree of the document
					 * @param self    document whose tree is being assembled
					 * @param reader  parser of a binary record
					 * @param event   kind of the received event of the parsing
					 * @return        sign of the success of the assembling of the tree
					 *
					 * \~
					 */
					static bool digest(void * context, Document * self, const reader_t & reader, const event_t event) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния документа
					 *
					 *
					 * \~english
					 * @brief Method of the reset of the state of a document
					 *
					 * \~
					 */
					void clear() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи в дерево документа
					 *
					 * @details Разбор опустошает прежнее дерево сам, СОХРАНЯЯ запас его
					 *          вместилищ: разбирающему многие записи подряд надлежит держать ОДНО
					 *          дерево и звать разбор по кругу, а не заводить дерево на всякую запись.
					 *
					 * @note Замер 23.08.2026, малая запись, 200 000 кругов: своё дерево на
					 *       всякую запись идёт 0.27 мкс, одно дерево по кругу - 0.20 мкс, то есть
					 *       на треть быстрее. У сборки записи рычаг этот вчетверо крупнее -
					 *       см. `Writer::reset`
					 *
					 * @param buffer буфер разбираемой записи
					 * @param size   размер разбираемой записи в октетах
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a record into a tree of a document
					 *
					 * @details The parsing empties the former tree itself, KEEPING the reserve of its
					 *          containers: one parsing many records in a row ought to keep a SINGLE tree
					 *          and call the parsing in a circle, rather than to start a new tree at every
					 *          record.
					 *
					 * @note Measurement of 2026-08-23, a small record, 200 000 rounds: an own tree per
					 *       every record runs at 0.27 us, a single tree in a circle — at 0.20 us, that is
					 *       by a third faster. For the assembling of a record this lever is four times
					 *       larger - see `Writer::reset`
					 *
					 * @param buffer buffer of the record being parsed
					 * @param size   size of the record being parsed in octets
					 * @return       sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const void * buffer, const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из дерева документа
					 *
					 * @warning Собранная запись равна разобранной по СМЫСЛУ, но не по октетам:
					 *          дерево хранит состав вместимого, а не вид записи его, и вместимое
					 *          неопределённой длины укладывается обратно определённым. Так, массив
					 *          `9F 01 02 03 DF` (5 окт.) выходит записью `83 01 02 03` (4 окт.)
					 *
					 * @warning Подпись контейнера считается по ОКТЕТАМ: запись, прочитанная в
					 *          дерево и уложенная обратно, подписи своей более не отвечает. Кому нужна
					 *          дословность, тот держит исходные октеты сам, а не пересобирает их деревом
					 *
					 * @param writer сборщик бинарной записи
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembling of a record from a tree of a document
					 *
					 * @warning The assembled record equals the parsed one by the MEANING but not by the octets: the
					 *          tree stores the composition of a container, not the form of its record, and a container of
					 *          an indefinite length is laid back as a definite one. Thus an array `9F 01 02 03 DF` (5 oct.)
					 *          issues a record `83 01 02 03` (4 oct.)
					 *
					 * @warning The signature of a container is computed over the OCTETS: a record read into a tree and
					 *          laid back no longer agrees with its signature. Whoever needs the verbatim keeps the original
					 *          octets himself rather than reassembling them by the tree
					 *
					 * @param writer assembler of a binary record
					 * @return       sign of the success of the assembling
					 *
					 * \~
					 */
					bool build(writer_t & writer) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из дерева документа
					 *
					 * @details Работа эта заведена ради согласия договоров кодеков: у шести
					 *          текстовых кодеков рамки сборка зовётся тем же именем.
					 *
					 * @note Выдаётся вместилище ОКТЕТОВ, а не строка: запись ABC двоична, и
					 *       нулевой октет в ней рядовой. Имя общее, вид выдачи свой - различие идёт
					 *       от существа записи, а не от разнобоя имён
					 *
					 * @return собранная запись, пустая при отказе сборки
					 *
					 * \~english
					 * @brief Method of the assembling of a record out of the tree of a document
					 *
					 * @details This work is introduced for the sake of the agreement of the codec contracts: for six text
					 *          codecs of the frame the assembling is called by the same name.
					 *
					 * @note A container of OCTETS is issued rather than a string: the record of ABC is binary, and a zero
					 *       octet in it is ordinary. The name is common, the kind of issuance is its own - the difference
					 *       goes from the substance of the record, not from a mismatch of names
					 *
					 * @return assembled record, empty upon a failure of the assembling
					 *
					 * \~
					 */
					vector <uint8_t> dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записи
					 *
					 * @param settings устанавливаемые настройки разбора записи
					 * @return         признак принятия настроек документа
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a record
					 *
					 * @param settings settings of the parsing of a record being set
					 * @return         sign of the acceptance of the settings of a document
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения настроек записи, отвечающих состоявшемуся разбору
					 *
					 * @return настройки записи текста
					 *
					 * \~english
					 * @brief Method of obtaining the settings of the writing corresponding to the parsing that has taken place
					 *
					 * @return settings of the writing of a text
					 *
					 * \~
					 */
					writer_t::settings_t writing() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения корня дерева документа
					 *
					 * @return ссылка на корень дерева документа
					 *
					 * \~english
					 * @brief Method of the extraction of the root of the tree of a document
					 *
					 * @return reference to the root of the tree of the document
					 *
					 * \~
					 */
					value_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества значений корня дерева документа
					 *
					 * @details Мерою взяты ДЕТИ КОРНЯ, а не узлы арены, и решено это сообща
					 *          по всем семи кодекам 03.09.2026. Довод от устройства: «узлы арены»
					 *          одного смысла у семерых иметь не могут - арены ведут себя по-разному (у
					 *          `xml` не сжимается, у `yaml` сжимается), - а «дети корня» определимо у всех
					 *          семи одинаково.
					 *
					 * @note Счёт узлов арены НЕ пропал, а зовётся `nodes()`. Прежде обе работы
					 *       выдавали одно и то же, и `nodes()` была помечена устаревшей: пометка снята,
					 *       работы разведены по смыслу
					 *
					 * @warning Договор этот СМЕНИЛСЯ: до 03.09.2026 работа выдавала счёт узлов
					 *          арены. Зовущему, коему нужен прежний смысл, надлежит звать `nodes()`
					 *
					 * @return количество значений корня дерева документа
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the values of the root of a document
					 *
					 * @details The measure taken is THE CHILDREN OF THE ROOT, not the nodes of the arena, and this was
					 *          decided jointly across all seven codecs on 2026-09-03. The argument from the design: "the
					 *          nodes of an arena" cannot have one meaning across the seven - the arenas behave differently
					 *          (the `xml` one is not compressed, the `yaml` one is compressed) - while "the children of the
					 *          root" is definable in all seven the same way.
					 *
					 * @note The count of the nodes of the arena has NOT vanished, it is called `nodes()`. Before, both
					 *       works issued the same thing, and `nodes()` was marked deprecated: the mark is removed, the
					 *       works are separated by meaning
					 *
					 * @warning This contract has CHANGED: before 2026-09-03 the work issued the count of the nodes of the
					 *          arena. A caller who needs the former meaning must call `nodes()`
					 *
					 * @return number of the values of the root of the document
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения дерева документа по пути
					 *
					 * @details Путь равняется на RFC 6901 и разбирается ТЕМ ЖЕ посредником
					 *          `abc::segment`, каким разбирает его владеющее значение (`abc::value_t`):
					 *          путь пустой значит весь документ, ведущая косая черта есть приставка
					 *          указателя, а звено пустое внутри пути есть законное имя поля.
					 *
					 * @details О том, взять ли звено номером либо именем, судит вид ВМЕСТИЛИЩА,
					 *          как то и велит RFC 6901: у отображения звено есть имя поля всегда, хотя бы
					 *          и записанное цифрами; у перечня - номер, и номер с ведущим нулём негоден.
					 *
					 * @note Работа эта заведена 03.09.2026 при вводе ABC в общие проверки договора
					 *       кодеков: дерево разбора пути не понимало ВОВСЕ, тогда как владеющее значение
					 *       их понимало. Тот же почерк, каким вскрыты двенадцать находок кодека, -
					 *       дорога есть, а близнеца у неё нет
					 *
					 * @param path путь к извлекаемому значению
					 * @return     ссылка на значение, недействительная при отсутствии его
					 *
					 * \~english
					 * @brief Method of the extraction of a value of the tree of a document by a path
					 *
					 * @details The path follows RFC 6901 and is parsed by THE SAME intermediary `abc::segment` by which
					 *          the owning value (`abc::value_t`) parses it: an empty path means the whole document, a
					 *          leading slash is the prefix of a pointer, and an empty link inside the path is a legitimate
					 *          field name.
					 *
					 * @details Whether to take a link by a number or by a name is judged by the kind of the CONTAINER, as
					 *          RFC 6901 commands: for a mapping a link is always a field name, even one written in digits;
					 *          for an array it is a number, and a number with a leading zero is unfit.
					 *
					 * @note This work was introduced on 2026-09-03 when ABC was brought into the common checks of the
					 *       codec contract: the tree understood the parsing of a path NOT AT ALL, while the owning value
					 *       understood them. The same handwriting by which twelve of the codec's findings were revealed -
					 *       there is a road, but it has no twin
					 *
					 * @param path path to the value being extracted
					 * @return     reference to the value, invalid if it is absent
					 *
					 * \~
					 */
					value_t at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия значения дерева документа по пути
					 *
					 * @details Опрос ведётся ТЕМ ЖЕ обходом, что и извлечение, а не своим:
					 *          разойдись они хоть на одном звене - разошлись бы молча.
					 *
					 * @param path путь к опрашиваемому значению
					 * @return     признак наличия значения по пути
					 *
					 * \~english
					 * @brief Method of the checking of the presence of a value of the tree of a document by a path
					 *
					 * @details The query is made by THE SAME walk as the extraction, not by its own: were they to diverge
					 *          even at one link, they would have diverged silently.
					 *
					 * @param path path to the value being asked about
					 * @return     sign of the presence of the value by the path
					 *
					 * \~
					 */
					bool has(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева документа на пустоту
					 *
					 * @return признак отсутствия узлов в дереве документа
					 *
					 * \~english
					 * @brief Method of the checking of the tree of a document for emptiness
					 *
					 * @return sign of the absence of the nodes in the tree of the document
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества узлов дерева документа
					 *
					 * @details Узлы арены - величина, зависящая от УСТРОЙСТВА кодека, и общего
					 *          смысла у семерых она иметь не может: арены ведут себя по-разному. Оттого в
					 *          общий договор она не входит вовсе и живёт у тех, у кого арена есть.
					 *
					 * @note Работа эта была помечена устаревшей 02.09.2026, когда `size()`
					 *       выдавала ТО ЖЕ САМОЕ. Пометка снята 03.09.2026 решением по трём
					 *       связкам кодеков: работы разведены по смыслу, и обе нужны
					 *
					 * @return количество узлов дерева документа
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the nodes of the tree of a document
					 *
					 * @details The nodes of an arena are a quantity that depends on the DESIGN of the codec, and across
					 *          the seven it cannot have a common meaning: the arenas behave differently. Therefore it does
					 *          not enter the common contract at all and lives only with those that have an arena.
					 *
					 * @note This work was marked deprecated on 2026-09-02, when `size()` issued THE VERY SAME. The mark
					 *       was removed on 2026-09-03 by a decision across three bundles of codecs: the works are separated
					 *       by meaning, and both are needed
					 *
					 * @return number of the nodes of the tree of the document
					 *
					 * \~
					 */
					size_t nodes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа разбора записи
					 *
					 * @return код отказа разбора записи
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the parsing of a record
					 *
					 * @return error code of the parsing of the record
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения места, где отказ разбора произошёл
					 *
					 * @details Работа эта заведена ради согласия договоров кодеков: у шести
					 *          текстовых кодеков рамки место отказа зовётся тем же именем, и потребитель,
					 *          читающий несколько кодеков, обходится одним кодом.
					 *
					 * @note Вид `abc::location_t` наследует общие поля строки, столбца и
					 *       смещения, но осмысленны у него только смещение и глубина: у
					 *       двоичной записи строк нет вовсе, и общие поля строки и столбца
					 *       остаются нулевыми. Различие это идёт от существа записи, а не от
					 *       разнобоя имён
					 *
					 * @return место, где отказ разбора произошёл
					 *
					 * \~english
					 * @brief Method of the extraction of the location where the failure of the parsing has occurred
					 *
					 * @details This work is introduced for the sake of the agreement of the codec contracts: for six
					 *          text codecs of the frame the location of a failure is called by the same name, and a consumer
					 *          reading several codecs makes do with one code.
					 *
					 * @note The kind `abc::location_t` inherits the common fields of a line, of a column and
					 *       of an offset, but only the offset and the depth carry meaning in it: a binary record
					 *       has no lines at all, and the common fields of a line and of a column remain zero.
					 *       The difference goes from the substance of the record, not from a mismatch of names
					 *
					 * @return location where the failure of the parsing has occurred
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем оператор копирования
					 *
					 * \~english
					 * @brief Enable the copy operator
					 *
					 * \~
					 */
					Document & operator = (const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем оператор перемещения
					 *
					 * \~english
					 * @brief Enable the move operator
					 *
					 * \~
					 */
					Document & operator = (Document &&) noexcept = default;
				public:
					/**
					 * \~russian
					 * @brief Разрешаем конструктор копирования
					 *
					 * \~english
					 * @brief Enable the copy constructor
					 *
					 * \~
					 */
					Document(const Document &) = default;
					/**
					 * \~russian
					 * @brief Разрешаем конструктор перемещения
					 *
					 * \~english
					 * @brief Enable the move constructor
					 *
					 * \~
					 */
					Document(Document &&) noexcept = default;
				public:
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
					 * @param settings настройки разбора документа
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param settings settings of the parsing of a document
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
