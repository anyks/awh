/**
 * @file document.hpp
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
 * @brief Заголовочный файл документа YAML — дерево, удерживаемое целиком, с обходом
 *        узлов и извлечением значений затребованным видом
 *
 * \~english
 * @brief Header file of a YAML document — a tree held in full with the traversal of the nodes
 *        and the extraction of the values by a demanded kind
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
#include <cstring>
#include <type_traits>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

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
		 * \~english
		 * @brief YAML container namespace
		 *
		 * \~
		 */
		namespace yaml {
			/**
			 * \~russian
			 * @brief Объявление владеющего значения наперёд
			 *
			 * @note Объявление нужно дружбе дерева: владеющее значение описано файлом
			 *       иным, включаемым позже, а имя его дереву требуется здесь
			 *
			 * \~english
			 * @brief Forward declaration of the owning value
			 *
			 * @note The declaration is needed by the friendship of the tree: the owning value is described
			 *       by another file, included later, while its name is required by the tree right here
			 *
			 * \~
			 */
			class Value;

			/**
			 * \~russian
			 * @brief Класс документа YAML, удерживаемый целиком
			 *
			 * @details Дерево держится плоским перечнем узлов, а не россыпью
			 *          самостоятельных вместилищ: узлы ложатся порядком обхода, и переход к соседу
			 *          есть сложение с размахом поддерева. Расход памяти тем выходит одним куском, а
			 *          обход - последовательным чтением.
			 *
			 * @note Текст YAML вправе нести много документов, и дерево удерживает их все:
			 *       `documents()` сказывает число их, а `root(номер)` отдаёт корень каждого
			 *
			 * @note Позиции узлов указывают в УДЕРЖАННЫЙ текст, а не в копию под деревом.
			 *       Хранение удержанного текста даёт 3-6% производительности на чтении после
			 *       правок №92, и плата за это — позиции приблизительны при внешней правке
			 *       исходного буфера после разбора. Договор оставлен 10.10.2026 (задача №91):
			 *       владение остаётся, требования на точную позицию у потребителей дерева нет
			 *
			 * \~english
			 * @brief YAML document class, held in its entirety
			 *
			 * @details The tree is held by a flat list of the nodes rather than by a scattering of the
			 *          independent containers: the nodes lie in the order of the traversal, and the transition
			 *          to a sibling is an addition with the extent of the subtree. The memory expense comes out
			 *          by this as a single piece, and the traversal is a sequential reading.
			 *
			 * @note A YAML text is entitled to carry many documents, and the tree holds all of them:
			 *       `documents()` tells their number, while `root(number)` issues the root of each one
			 *
			 * @note The positions of the nodes point into the RETAINED text rather than into a copy under the
			 *       tree. The storage of the retained text gives 3-6% of performance at the reading after the
			 *       edits #92, and the price for this is that the positions are approximate at an external
			 *       editing of the source buffer after the parsing. The contract was kept on 2026-10-10 (task
			 *       #91): the retention stays, and the consumers of the tree have no demand for an exact
			 *       position
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Владеющее значение, дереву доступ дающее
					 *
					 * @details Доступ этот нужен переносу: ведётся он на копии дерева ради
					 *          целости своей, а код отказа по неудаче обязан достаться дереву
					 *          ИСХОДНОМУ - копия с ним пропадает, и потребитель остался бы с
					 *          отказом без названной причины.
					 *
					 * @note Имя пишется полным: у дерева есть свой вложенный вид `Value` -
					 *       ссылка на узел, - и краткое имя назвало бы дружбой именно его
					 *
					 * \~english
					 * @brief Owning value granted the access to the tree
					 *
					 * @details This access is needed by the grafting: it is conducted on a copy of the tree
					 *          for the sake of its own integrity, while the code of the refusal must reach the
					 *          ORIGINAL tree — the copy vanishes with it.
					 *
					 * @note The name is written out in full: the tree has a nested kind `Value` of its own — a
					 *       reference to a node — and a short name would have made the friendship name exactly that
					 *
					 * \~
					 */
					friend class Value;
				public:
					/**
					 * \~russian
					 * @brief Структура настроек документа
					 *
					 * \~english
					 * @brief Document settings structure
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Признак удержания исходного текста ради дословной перезаписи
						 *
						 * @details Настройки YAML правятся и человеком, и приложением, и перезапись
						 *          обязана сохранять всё, чего правка не касалась: примечания, пустые строки,
						 *          ограду значений, ширину отступов и построение вместилищ скобками. Собрать
						 *          это заново дерево не может - примечаний оно не держит вовсе, - и оттого
						 *          исходный текст удерживается целиком, а поддеревья нетронутые переписываются
						 *          дословно.
						 *
						 * @note Удержание стоит памяти в размер текста, и оттого спрашивается
						 *       прямо: читающему настройки на один раз оно ни к чему
						 *
						 * @note БЕЗ удержания перезапись ведётся деревом заново, и построение
						 *       скобками обращается в построение отступом: `[a, b]` переписывается
						 *       перечнем на отступе. Решено это намеренно - дерево построения не
						 *       хранит, ни чтение его не выдаёт, ни узел не держит, - и удержание
						 *       есть тот самый ответ кодека на нужду сохранить написанное человеком.
						 *       Закреплено проверкой CodecYamlDocument.FlowSurvivesRetainedRewrite
						 *
						 * \~english
						 * Sign of the retention of the source text for the sake of the verbatim rewriting
						 *
						 * @details The YAML settings are edited both by a human and by an application, and the
						 *          rewriting must preserve everything the editing did not touch: the comments, the
						 *          empty lines, the quoting of the values, the width of the indents and the bracket
						 *          construction of the containers. A tree cannot gather this anew - it holds no
						 *          comments at all - and therefore the source text is retained in full, while the
						 *          untouched subtrees are rewritten verbatim.
						 *
						 * @note The retention costs the memory in the size of the text and is therefore asked for
						 *       directly: a reader of the settings for one pass has no need of it
						 *
						 * @note WITHOUT the retention the rewriting is conducted by the tree anew, and the bracket
						 *       construction turns into the construction by indentation: `[a, b]` is rewritten as a
						 *       list on the indent. This is decided deliberately - the tree stores no constructions,
						 *       the reading does not issue them, and a node holds none - and the retention is that
						 *       very answer of the codec to the need to keep what a human has written. Fixed by the
						 *       check CodecYamlDocument.FlowSurvivesRetainedRewrite
						 *
						 * \~
						 */
						bool retain;
						/**
						 * \~russian
						 * Наибольшее допустимое количество узлов раскрытия ссылок, ноль - предел
						 * по умолчанию
						 *
						 * @note Предел этот стережёт от беды, известной под именем миллиарда
						 *       смешков: девять меток, каждая из которых десятикратно повторяет
						 *       предыдущую, раскрываются в миллиард узлов из текста в двести байт.
						 *       Потоковое чтение ссылок не раскрывает вовсе и предела того не
						 *       знает, а дерево раскрывает - ему и стеречь
						 *
						 * \~english
						 * Largest admissible number of the nodes generated by the expansion of the aliases, zero
						 * being the default limit
						 *
						 * @note This limit guards against the trouble known under the name of the «billion laughs»:
						 *       nine anchors, each of them repeating the previous one tenfold, expand into a
						 *       billion nodes out of a text of two hundred bytes. The stream reading does not
						 *       expand the aliases at all and does not know that limit, while the tree does expand
						 *       them - so it is the tree that must guard
						 *
						 * \~
						 */
						uint32_t maxExpansion;
						// Настройки объекта записи текста документа
						writer_t::settings_t writer;
						// Настройки объекта разбора текста документа
						reader_t::settings_t reader;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Settings() noexcept;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Структура свойства узла, предпосланные ему записью
					 *
					 * @note Свойства эти держатся отдельным перечнем, а не полями узла: метки и
					 *       метки типов встречаются единицами на документ, и поля под них сделали
					 *       бы тяжелее всякий узел ради немногих
					 *
					 * \~english
					 * @brief The structure of a node property, as introduced by a record
					 *
					 * @note These properties are held by a separate list rather than by the fields of a node: the
					 *       anchors and the type tags are met by units to a document, and the fields under them would
					 *       make every node heavier for the sake of the few
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Properties {
						// Смещение метки типа в хранилище знаков
						uint32_t tag;
						// Длина метки типа в байтах
						uint32_t tagged;
						// Смещение имени метки в хранилище знаков
						uint32_t anchor;
						/**
						 * \~russian
						 * Смещение метки типа, ИМЕНИ ПАРЫ предпосланной, в хранилище знаков
						 *
						 * @details Метка эта принадлежит имени пары, а не значению её: написание
						 *          `!!str a: b` метит запись `a`, и держать её вместе с меткою значения нельзя -
						 *          они разные. Узла же у имени пары дерево не держит вовсе, оттого метка
						 *          его и кладётся сюда, к узлу самой пары - тем же порядком, каким кладётся
						 *          метка узла имени.
						 *
						 * @warning Прежде метка эта отбрасывалась: держать её было негде. Перезапись
						 *          теряла её молча, и текст `!!str a: b` возвращался записью `a: b`
						 *
						 * \~english
						 * Offset of the tag placed before the NAME OF A PAIR in the storage of characters
						 *
						 * @details The tag belongs to the name of a pair rather than to its value: the notation
						 *          `!!str a: b` marks the record `a`, and holding it together with the tag of the value is
						 *          impossible - they are different. The tree holds no node for the name of a pair at all,
						 *          and that is why its tag is laid here, to the node of the pair itself - by the same order
						 *          by which the tag of the node of the name is laid.
						 *
						 * @warning Formerly this tag was discarded: the rewrite lost it silently, and the text
						 *          `!!str a: b` came back as the record `a: b`
						 *
						 * \~
						 */
						uint32_t keyTag;
						// Длина имени метки в байтах
						uint32_t anchored;
						// Длина метки типа, имени пары предпосланной, в байтах
						uint32_t keyTagged;
						/**
						 * \~russian
						 * Смещение имени метки, ИМЕНИ ПАРЫ предпосланной, в хранилище знаков
						 *
						 * @details Метка эта принадлежит имени пары, а не значению её: написание
						 *          `&m a: b` метит запись `a`, и держать её вместе с меткою значения нельзя -
						 *          они разные. Узла же у имени пары дерево не держит вовсе, оттого метка
						 *          его и кладётся сюда, к узлу самой пары.
						 *
						 * @warning Прежде метка эта отбрасывалась: держать её было негде. Ссылка на
						 *          неё разрешалась записью имени, а вот ПЕРЕЗАПИСЬ её теряла - и текст,
						 *          собранный заново, нёс ссылку на метку, ни разу не объявленную
						 *
						 * \~english
						 * Offset of the name of the anchor placed before the NAME OF A PAIR in the storage of characters
						 *
						 * @details The anchor belongs to the name of a pair rather than to its value: the notation
						 *          `&m a: b` marks the record `a`, and holding it together with the anchor of the value is
						 *          impossible - they are different. The tree holds no node for the name of a pair at all,
						 *          and that is why its anchor is laid here, to the node of the pair itself.
						 *
						 * @warning Formerly this anchor was discarded: there was nowhere to hold it. A reference to it
						 *          was permitted by the notation of the name, but the REWRITE lost it — and the text,
						 *          gathered anew, carried a reference to an anchor never once declared
						 *
						 * \~
						 */
						uint32_t keyAnchor;
						// Длина имени метки, имени пары предпосланной, в байтах
						uint32_t keyAnchored;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Properties() noexcept;
					} props_t;
					/**
					 * \~russian
					 * @brief Структура узла дерева документа
					 *
					 * @note Узел шире узла контейнера JSON, и шире он нарочно: запись значения
					 *       удерживается всегда, даже у числа, разобранного в родной вид.
					 *       Перезапись обязана вернуть `0x1F` записью его, а не числом 31, и без
					 *       записи вернуть её было бы неоткуда
					 *
					 * \~english
					 * @brief Structure of a document tree node
					 *
					 * @note The node is wider than the node of the JSON container, and it is wider on purpose: the
					 *       record of a value is held always, even for a number parsed into a native kind. A rewriting
					 *       has to return `0x1F` as its record and not as the number 31, and without the record there
					 *       would be nothing to return it from
					 *
					 * \~
					 */
					typedef struct Node {
						// Признак того, что узел является парой отображения
						bool keyed;
						// Признак того, что содержимое узла правлено после разбора
						bool touched;
						// Признак того, что узел блочного значения лишился соседа снизу
						bool severed;
						// Вид значения узла документа
						type_t type;
						// Вид записи значения в исходном тексте
						style_t style;
						/**
						 * \~russian
						 * Смещение за концом записи узла в удержанном исходном тексте
						 *
						 * @details Границею служит начало записи следующего узла обхода: всякий
						 *          байт текста принадлежит ровно одному узлу, и примечания, между соседями
						 *          стоящие, достаются тому, над кем они стоят. Держится граница полем, а не
						 *          считается по соседу: снятие узла соседа сдвинуло бы, и байты снятого
						 *          достались бы тому, кто стоял над ним.
						 *
						 * \~english
						 * Offset past the end of the record of a node in the retained source text
						 *
						 * @details The boundary serves by the beginning of the record of the next node of the
						 *          traversal: every byte of the text belongs to exactly one node, and the comments
						 *          standing between the neighbours go to the one above whom they stand. The
						 *          boundary is held by a field and is not computed by the neighbour: the removal of
						 *          a neighbour node would shift it, and the bytes of the removed one would fall to
						 *          the one standing above it.
						 *
						 * \~
						 */
						uint32_t edge;
						// Длина имени пары в байтах
						uint32_t named;
						// Номер свойств узла в перечне свойств, ноль - свойств нет
						uint32_t props;
						// Смещение имени пары в хранилище знаков
						uint32_t offset;
						/**
						 * \~russian
						 * Смещение начала записи узла в удержанном исходном тексте
						 *
						 * @details Считается оно от начала строки, узел открывающей, вместе с
						 *          примечаниями да пустыми строками, ей предшествующими: примечание стоит над
						 *          тем, к чему относится, и уходить обязано вместе с ним. Значение
						 *          `NO_ORIGIN` знаменует, что текст не удержан либо узел взят раскрытием
						 *          ссылки, и записи в тексте за ним не стоит.
						 *
						 * \~english
						 * Offset of the beginning of the record of a node in the retained source text
						 *
						 * @details It is counted from the beginning of the line that opens the node, together with
						 *          the comments and the empty lines standing before it: a comment stands above that
						 *          to which it belongs and has to leave together with it. The value `NO_ORIGIN`
						 *          means that the text was not retained or that the node was taken by the expansion
						 *          of an alias, and no record in the text stands behind it.
						 *
						 * \~
						 */
						uint32_t origin;
						/**
						 * \~russian
						 * Смещение начала собственной строки узла в удержанном тексте
						 *
						 * @details Начало записи узла вправе стоять выше собственной строки его:
						 *          примечания да пустые строки, над узлом стоящие, уходят вместе с ним, а
						 *          черта записи перечня и вовсе переносится к узлу отдельным проходом.
						 *          Собственная же строка есть та, где стоит само содержимое, и запоминать
						 *          её приходится при постановке узла: после переноса черты по началу
						 *          записи её уже не вычислить.
						 *
						 * \~english
						 * Offset of the beginning of the own line of the node in the retained text
						 *
						 * @details The beginning of the record of a node may stand above its own line:
						 *          the comments and the blank lines standing above the node leave together with it, while
						 *          the dash of an entry of a sequence is moved to the node by a separate pass altogether.
						 *          The own line, however, is the one where the content itself stands, and it has to be remembered
						 *          at the placement of the node: after the moving of the dash it can no longer be computed
						 *          from the beginning of the record.
						 *
						 * \~
						 */
						uint32_t dwelling;
						// Разобранное число значения шириною в восемь байтов
						uint32_t number[2];
						/**
						 * \~russian
						 * Количество детей вместилища либо длина записи значения в байтах
						 *
						 * @details Вторым числом у вместилища лежит размах поддерева его - число
						 *          узлов, вместилищем занятых вместе с ним самим.
						 *
						 * \~english
						 * Number of the children of a container or the length of the record of a value in bytes
						 *
						 * @details The second number of a container holds the extent of its subtree - the number
						 *          of the nodes occupied by the container together with itself.
						 *
						 * \~
						 */
						uint32_t content[2];
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместилища либо длины записи
						 *
						 * @return количество детей вместилища либо длина записи значения
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the record
						 *
						 * @return number of the children of a container or the length of the record of the value
						 *
						 * \~
						 */
						AWH_YAML_INLINE uint32_t length() const noexcept {
							// Выводим количество детей вместилища либо длину записи значения
							return this->content[0];
						}
						/**
						 * \~russian
						 * @brief Метод получения размаха поддерева вместилища
						 *
						 * @return количество узлов, вместилищем занятых вместе с ним самим
						 *
						 * \~english
						 * @brief Method of the obtaining of the extent of the subtree of a container
						 *
						 * @return number of the nodes occupied by the container together with itself
						 *
						 * \~
						 */
						AWH_YAML_INLINE uint32_t extent() const noexcept {
							// Выводим размах поддерева вместилища
							return this->content[1];
						}
						/**
						 * \~russian
						 * @brief Метод установки количества детей вместилища либо длины записи
						 *
						 * @param value устанавливаемое количество детей либо длина записи
						 *
						 * \~english
						 * @brief Method of the setting of the number of the children of a container or of the length of the record
						 *
						 * @param value number of the children or the length of the record being set
						 *
						 * \~
						 */
						AWH_YAML_INLINE void length(const uint32_t value) noexcept {
							// Устанавливаем количество детей вместилища либо длину записи
							this->content[0] = value;
						}
						/**
						 * \~russian
						 * @brief Метод установки размаха поддерева вместилища
						 *
						 * @param value устанавливаемый размах поддерева
						 *
						 * \~english
						 * @brief Method of the setting of the extent of the subtree of a container
						 *
						 * @param value extent of the subtree being set
						 *
						 * \~
						 */
						AWH_YAML_INLINE void extent(const uint32_t value) noexcept {
							// Устанавливаем размах поддерева вместилища
							this->content[1] = value;
						}
						/**
						 * \~russian
						 * @brief Шаблон метода получения разобранного числа значения
						 *
						 * @tparam T вид получаемого числа
						 *
						 * \~english
						 * @brief Template for the method of obtaining the parsed numeric value
						 *
						 * @tparam T kind of the number being obtained
						 *
						 * \~
						 */
						template <typename T>
						/**
						 * \~russian
						 * @brief Метод получения разобранного числа значения
						 *
						 * @return разобранное число значения
						 *
						 * \~english
						 * @brief Method of the obtaining of the parsed number of a value
						 *
						 * @return parsed number of the value
						 *
						 * \~
						 */
						AWH_YAML_INLINE T numberOf() const noexcept {
							// Получаемое разобранное число значения
							T result = T();
							// Выполняем перенос разобранного числа значения
							::memcpy(&result, this->number, sizeof(result));
							// Выводим разобранное число значения
							return result;
						}
						/**
						 * \~russian
						 * @brief Шаблон метода установки разобранного числа значения
						 *
						 * @tparam T вид устанавливаемого числа
						 *
						 * \~english
						 * @brief Parsed number value setting method template
						 *
						 * @tparam T kind of the number being set
						 *
						 * \~
						 */
						template <typename T>
						/**
						 * \~russian
						 * @brief Метод установки разобранного числа значения
						 *
						 * @param  value устанавливаемое число значения
						 *
						 * \~english
						 * @brief Method of the setting of the parsed number of a value
						 *
						 * @param value number of the value being set
						 *
						 * \~
						 */
						AWH_YAML_INLINE void numberOf(const T value) noexcept {
							// Выполняем сброс разобранного числа значения
							this->number[0] = 0;
							// Выполняем сброс старшей половины разобранного числа
							this->number[1] = 0;
							// Выполняем перенос устанавливаемого числа значения
							::memcpy(this->number, &value, sizeof(value));
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
				public:
					/**
					 * \~russian
					 * @brief Ссылка на узел дерева документа
					 *
					 * @details Ссылка есть указатель на узел, а не сам узел: копируется она
					 *          даром, а узлы всё это время лежат в документе одним куском.
					 *
					 * @warning Ссылка действительна, покуда документ не перестроен: разбор
					 *          нового текста перестраивает перечень узлов, и всякая ссылка,
					 *          прежде взятая, указывает после того неизвестно куда
					 *
					 * \~english
					 * @brief Reference to a node of the tree of a document
					 *
					 * @details A reference is a pointer to a node rather than the node itself: it is copied
					 *          for free, while the nodes lie in the document as one piece all this time.
					 *
					 * @warning A reference is valid as long as the document is not rebuilt: the parsing of a new
					 *          text rebuilds the list of the nodes, and every reference taken before that points
					 *          afterwards, nobody knows where
					 *
					 * \~
					 */
					typedef class __AWH_SHARED_EXPORT__ Value {
						private:
							/**
							 * \~russian
							 * Документ, дерево какого правится, ссылке доверяется
							 *
							 * @note Правка ведётся документом, а не ссылкою: ссылка на узел смотрит,
							 *       а правит держащий дерево целиком - ему одному ведомы предки узла,
							 *       кои правкой тоже тронуты
							 *
							 * \~english
							 * The document whose tree is being edited, trusted to the reference
							 *
							 * @note The editing is conducted by the document and not by the reference: a reference to
							 *       a node only looks, while the editing is done by the one holding the tree in full -
							 *       it alone knows the ancestors of the node, which the editing has touched as well
							 *
							 * \~
							 */
							friend class Document;
						private:
							// Номер узла в перечне узлов документа
							uint32_t _index;
							/**
							 * \~russian
							 * @brief Клеймо поколения перечня узлов, при заведении ссылки снятое
							 *
							 * @details Ссылка живёт номером узла, а перестроение перечня номера
							 *          смещает: без клейма ссылка, перестроение пережившая, отвечала бы
							 *          годной и отдавала бы содержимое совсем другого узла.
							 *
							 * \~english
							 * @brief Stamp of the generation of the list of the nodes taken at the creation of the reference
							 *
							 * @details A reference lives by the index of a node, while a rebuilding of the list shifts the
							 *          indexes: without a stamp, a reference that had survived a rebuilding would answer as
							 *          fit and would issue the content of an entirely different node.
							 *
							 * \~
							 */
							uint32_t _stamp;
							/**
							 * \~russian
							 * Номер узла за последним узлом вместилища, какому узел принадлежит
							 *
							 * @details Указаний на родителя узел не несёт, а переход к соседу обязан
							 *          останавливаться на границе своего вместилища: без границы обход
							 *          перечня продолжился бы соседом родителя.
							 *
							 * \~english
							 * Number of the node past the last node of the container the node belongs to
							 *
							 * @details A node carries no pointer to its parent, while the transition to a neighbour
							 *          has to stop at the boundary of its own container: without the boundary the
							 *          traversal of a list would go on by the neighbour of the parent.
							 *
							 * \~
							 */
							uint32_t _bound;
						private:
							// Документ, которому принадлежит узел
							const Document * _doc;
						private:
							/**
							 * \~russian
							 * @brief Шаблон метода извлечения числа затребованным видом
							 *
							 * @tparam T вид извлекаемого числа
							 *
							 * \~english
							 * @brief Template for a method to extract a number in the requested format
							 *
							 * @tparam T kind of the number being extracted
							 *
							 * \~
							 */
							template <typename T>
							/**
							 * \~russian
							 * @brief Метод извлечения числа затребованным видом
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a number by a demanded kind
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool extract(T & result) const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод проверки действительности ссылки
							 *
							 * @return признак действительности ссылки
							 *
							 * \~english
							 * @brief Method of the check of the validity of the reference
							 *
							 * @return sign of the validity of the reference
							 *
							 * \~
							 */
							bool valid() const noexcept;
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
							 * @brief Метод извлечения вида значения узла
							 *
							 * @return вид значения узла документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of the value of a node
							 *
							 * @return kind of the value of the node of the document
							 *
							 * \~
							 */
							type_t type() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки вида значения узла
							 *
							 * @param type проверяемый вид значения
							 * @return     признак соответствия вида значения
							 *
							 * \~english
							 * @brief Method of the check of the kind of the value of a node
							 *
							 * @param type kind of the value being checked
							 * @return     sign of the correspondence of the kind of the value
							 *
							 * \~
							 */
							bool is(const type_t type) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида записи значения
							 *
							 * @return вид записи значения в исходном тексте
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of the notation of a value
							 *
							 * @return kind of the notation of the value in the source text
							 *
							 * \~
							 */
							style_t style() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения схемы, над разбором действовавшей
							 *
							 * @details Схема эта настройками задана не всегда: директива
							 *          `%YAML 1.1` переводит разбор на схему наречия 1.1. Знать её надобно
							 *          тому, кто значение с дерева снимает: запись `on` логическою является
							 *          лишь под нею, и перезапись иною схемою переменила бы смысл.
							 *
							 * @return схема, над разбором действовавшая
							 *
							 * \~english
							 * @brief Method of the extraction of the schema which was acting over the parsing
							 *
							 * @details This schema is not always set by the settings: the `%YAML 1.1` directive
							 *          switches the parsing to the schema of the 1.1 dialect. It is needed by whoever
							 *          takes a value off the tree: the record `on` is a logical one only under it,
							 *          and a rewriting by another schema would change the meaning.
							 *
							 * @return schema which was acting over the parsing
							 *
							 * \~
							 */
							schema_t schema() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения количества детей вместилища
							 *
							 * @return количество детей вместилища
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the children of a container
							 *
							 * @return number of the children of the container
							 *
							 * \~
							 */
							size_t size() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки вместилища на пустоту
							 *
							 * @return признак пустоты вместилища
							 *
							 * \~english
							 * @brief Method of the check of a container for the emptiness
							 *
							 * @return sign of the emptiness of the container
							 *
							 * \~
							 */
							bool empty() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени пары отображения
							 *
							 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
							 *          содержимое видами в своё хранилище знаков, а правка его
							 *          наращивает и по мере накопления мусора уплотняет, хранилище
							 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
							 *          снимайте копию
							 *
							 * @return имя пары отображения, пусто у значения перечня
							 *
							 * \~english
							 * @brief Method of the extraction of the name of a pair of a mapping
							 *
							 * @warning The issued views live until the next EDITING of the tree. The tree issues
							 *          the content by the views into its storage of the characters, while an editing grows it
							 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
							 *          before an editing dangles. Should a view be needed longer — take a copy
							 *
							 * @return name of the pair of the mapping, empty for a value of a sequence
							 *
							 * \~
							 */
							string_view name() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения записи значения, исходным текстом данной
							 *
							 * @details Запись выдаётся приведённой к окончательному виду: ограда с
							 *          неё снята, а отменяющие последовательности раскрыты. Запись числа
							 *          выдаётся тою же, какою она в тексте и стояла - `0x1F` останется
							 *          записью `0x1F`, а не обратится числом 31.
							 *
							 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
							 *          содержимое видами в своё хранилище знаков, а правка его
							 *          наращивает и по мере накопления мусора уплотняет, хранилище
							 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
							 *          снимайте копию
							 *
							 * @return запись значения узла
							 *
							 * \~english
							 * @brief Method of the extraction of the record of a value given by the source text
							 *
							 * @details The record is issued brought to the final kind: the quoting is removed from it, and
							 *          the escape sequences are expanded. The record of a number is issued the very same as it
							 *          stood in the text - `0x1F` stays as the record `0x1F` rather than turning into the number
							 *          31.
							 *
							 * @warning The issued views live until the next EDITING of the tree. The tree issues
							 *          the content by the views into its storage of the characters, while an editing grows it
							 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
							 *          before an editing dangles. Should a view be needed longer — take a copy
							 *
							 * @return record of the value of the node
							 *
							 * \~
							 */
							string_view text() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени метки, узлу предпосланной
							 *
							 * @return имя метки узла, пусто при отсутствии её
							 *
							 * \~english
							 * @brief Method of the extraction of the name of the anchor placed before a node
							 *
							 * @return name of the anchor of the node, empty at the absence of it
							 *
							 * \~
							 */
							string_view anchor() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения метки типа, узлу предпосланной
							 *
							 * @return метка типа узла, пусто при отсутствии её
							 *
							 * \~english
							 * @brief Method of the extraction of the tag placed before a node
							 *
							 * @return tag of the node, empty at the absence of it
							 *
							 * \~
							 */
							string_view tag() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения метки узла, ИМЕНИ ПАРЫ предпосланной
							 *
							 * @details Метка эта принадлежит имени пары, а не значению её: написание
							 *          `&m a: b` метит запись `a`. Узла у имени пары дерево не держит, и
							 *          свойства его кладутся к узлу самой пары.
							 *
							 * @return имя метки, имени пары предпосланной, пустое - метки нет
							 *
							 * \~english
							 * @brief Method of the obtaining of the anchor placed before the NAME OF A PAIR
							 *
							 * @details This anchor belongs to the name of a pair rather than to its value: the notation
							 *          `&m a: b` marks the record `a`. The tree holds no node for the name of a pair, and
							 *          the properties of it are laid to the node of the pair itself.
							 *
							 * @return name of the anchor placed before the name of the pair, empty if there is none
							 *
							 * \~
							 */
							string_view keyAnchor() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения метки типа, ИМЕНИ ПАРЫ предпосланной
							 *
							 * @details Метка эта принадлежит имени пары, а не значению её: написание
							 *          `!!str a: b` метит запись `a`
							 *
							 * @return метка типа, имени пары предпосланная, пустая - метки нет
							 *
							 * \~english
							 * @brief Method of the obtaining of the tag placed before the NAME OF A PAIR
							 *
							 * @details This tag belongs to the name of a pair rather than to its value: the notation
							 *          `!!str a: b` marks the record `a`.
							 *
							 * @return tag placed before the name of the pair, empty if there is none
							 *
							 * \~
							 */
							string_view keyTag() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод получения ссылки на пару отображения по имени её
							 *
							 * @param name имя разыскиваемой пары отображения
							 * @return     ссылка на найденную пару, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a pair of a mapping by its name
							 *
							 * @param name name of the pair of the mapping being searched for
							 * @return     reference to the found pair, invalid at the absence
							 *
							 * \~
							 */
							Value operator [] (const string & name) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на значение перечня по номеру его
							 *
							 * @param index номер разыскиваемого значения перечня
							 * @return      ссылка на найденное значение, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a value of a sequence by its index
							 *
							 * @param index index of the value of the sequence being searched for
							 * @return      reference to the found value, invalid at the absence
							 *
							 * \~
							 */
							Value operator [] (const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на узел по пути к нему
							 *
							 * @details Путь записывается тем же порядком, каким записывает его
							 *          контейнер JSON: части его отделяются косою чертою, а числом
							 *          обозначается номер значения перечня. Путь `/server/hosts/0` ведёт к
							 *          первому значению перечня `hosts` отображения `server`
							 *
							 * @param path путь к разыскиваемому узлу
							 * @return     ссылка на найденный узел, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a node by the path to it
							 *
							 * @details The path is written by the same order by which the JSON container writes it: the
							 *          parts of it are separated by a slash, and a number denotes the index of a value of a
							 *          list. The path `/server/hosts/0` leads to the first value of the `hosts` list of the
							 *          `server` mapping.
							 *
							 * @param path path to the node being searched for
							 * @return     reference to the found node, invalid at the absence
							 *
							 * \~
							 */
							Value at(const string & path) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на соседний узел вместилища
							 *
							 * @return ссылка на соседний узел, недействительная за границей вместилища
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to the neighbouring node of a container
							 *
							 * @return reference to the neighbouring node, invalid beyond the boundary of the container
							 *
							 * \~
							 */
							Value next() const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на первого ребёнка вместилища
							 *
							 * @return ссылка на первого ребёнка, недействительная у пустого вместилища
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to the first child of a container
							 *
							 * @return reference to the first child, invalid for an empty container
							 *
							 * \~
							 */
							Value begin() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения логического значения
							 *
							 * @param result извлечённое логическое значение
							 * @return       признак успешного извлечения значения
							 *
							 * \~english
							 * @brief Method of the extraction of a boolean value
							 *
							 * @param result extracted boolean value
							 * @return       sign of the successful extraction of the value
							 *
							 * \~
							 */
							bool value(bool & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в один байт
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer one byte wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в два байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer two bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в четыре байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer four bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в восемь байтов
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer eight bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в один байт
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer one byte wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в два байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer two bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в четыре байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer four bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в восемь байтов
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer eight bytes wide
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения дробного числа одинарной точности
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a floating point number of a single precision
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(float & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения дробного числа двойной точности
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a floating point number of a double precision
							 *
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(double & result) const noexcept;
							/**
							 * \~russian
							 * @brief Шаблон метода извлечения целого числа вида языка
							 *
							 * @tparam T вид извлекаемого числа
							 *
							 * \~english
							 * @brief Integer extraction method template for a language variant
							 *
							 * @tparam T kind of the number being extracted
							 *
							 * \~
							 */
							template <typename T, typename = typename std::enable_if <std::is_integral <T>::value>::type>
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа вида языка
							 *
							 * @warning Приём этот заведён ради видов ЯЗЫКА (long, size_t и прочих), кои
							 *          с видами РАЗРЯДНЫМИ не совпадают. У macOS int64_t есть long long,
							 *          а у Linux - long; перечень одних разрядных видов негоден на обеих
							 *          системах, только по-разному. Извлечение ведётся тождественным
							 *          разрядным видом той же ширины и той же знаковости, потому потери
							 *          разряда тут нет вовсе
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an integer of a language kind
							 *
							 * @warning The device is made for the kinds OF THE LANGUAGE (long, size_t and others), which do
							 *          not match the BIT kinds. Under macOS int64_t is long long, while under Linux it is
							 *          long; a list of only the bit kinds is unfit on both systems, each in its own way. The
							 *          extraction is conducted by an identical bit kind of the same width and the same
							 *          signedness, whereby there is no loss of the bitness here at all
							 * 
							 * @param result extracted number
							 * @return       sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(T & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения строкового значения
							 *
							 * @param result извлечённое строковое значение
							 * @return       признак успешного извлечения значения
							 *
							 * \~english
							 * @brief Method of the extraction of a string value
							 *
							 * @param result extracted string value
							 * @return       sign of the successful extraction of the value
							 *
							 * \~
							 */
							bool value(string & result) const noexcept;
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
							 * @param doc   документ, которому принадлежит узел
							 * @param index номер узла в перечне узлов документа
							 * @param bound номер узла за последним узлом вместилища
							 *
							 * \~english
							 * @brief Constructor
							 *
							 * @param doc   document to which the node belongs
							 * @param index index of the node in the list of the nodes of the document
							 * @param bound index of the node past the last node of the container
							 *
							 * \~
							 */
							explicit Value(const Document * doc, const uint32_t index, const uint32_t bound) noexcept;
					} value_t;
				private:
					/**
					 * \~russian
					 * Открываем ссылке доступ к внутреннему устройству документа
					 *
					 * @note Доступ этот нужен ссылке затем, что живёт она узлами документа, а
					 *       выдавать наружу перечень узлов ради неё одной незачем
					 *
					 * \~english
					 * Opening the inner structure of the document to the reference
					 *
					 * @note This access is needed by the reference because it lives by the nodes of the
					 *       document, and there is no reason to issue the list of the nodes outward for its
					 *       sake alone
					 *
					 * \~
					 */
					friend class Value;
				private:
					/**
					 * \~russian
					 * Признак того, что наречие текста объявлено директивой
					 *
					 * @details Признак этот нужен перезаписи: директива `%YAML 1.1` переводит
					 *          разбор на схему наречия 1.1, и перезапись без неё переменила бы смысл
					 *          документа - запись `on`, логическая под 1.1, вернулась бы строкою.
					 *
					 * \~english
					 * Sign that the dialect of the text is declared by a directive
					 *
					 * @details This sign is needed by the rewriting: the `%YAML 1.1` directive switches
					 *          the parsing to the schema of the 1.1 dialect, and a rewriting without it would change the meaning
					 *          of the document — the record `on`, logical under 1.1, would return as a string.
					 *
					 * \~
					 */
					bool _versioned;
				private:
					/**
					 * \~russian
					 * Длина метки порядка байтов в начале удержанного текста
					 *
					 * @note Смещения событий чтение считает по тексту, метки той лишённому, а
					 *       удержан текст поданным целиком: разница эта и есть длина метки, и
					 *       прибавляется она ко всякому смещению
					 *
					 * \~english
					 * Length of the byte order mark at the beginning of the retained text
					 *
					 * @note The reading counts the offsets of the events over a text stripped of that mark,
					 *       while the text is retained as it was fed, in full: this difference is the length
					 *       of the mark itself, and it is added to every offset
					 *
					 * \~
					 */
					uint32_t _prologue;
					/**
					 * \~russian
					 * @brief Клеймо поколения перечня узлов
					 *
					 * @details Ссылка на узел держит НОМЕР его в перечне, а очистка, вставка и
					 *          снос номера смещают: ссылка, перестроение пережившая, указывала бы на
					 *          совсем другой узел - и отвечала бы годной, отдавая правдоподобное чужое
					 *          содержимое. Ни кода отказа, ни падения при этом нет, и отличить подмену
					 *          потребителю нечем вовсе.
					 *
					 * @details Клеймо растёт при всяком перестроении, ссылка снимает его при
					 *          заведении, а действительность сличает.
					 *
					 * @note Дозапись узла в КОНЕЦ перечня клейма не трогает намеренно: смысла
					 *       прежних номеров она не меняет, и клеймение её обрывало бы ссылки, взятые
					 *       при обходе растущего дерева
					 *
					 * @warning Беда эта найдена у кодеков JSON и XML и передана соседям;
					 *          у YAML она подтвердилась щупом - узел, снятый до повторного разбора,
					 *          отвечал годным и отдавал содержимое НОВОГО документа
					 *
					 * \~english
					 * @brief Stamp of the generation of the list of the nodes
					 *
					 * @details A reference to a node holds the INDEX of it in the list, while a clearing, an
					 *          insertion and a removal shift the indexes: a reference that has survived a rebuilding
					 *          would point at an entirely different node - and would answer as fit, giving out the
					 *          plausible content of another one. There is neither a code of refusal nor a crash by
					 *          this, and the consumer has no means at all to tell the substitution apart.
					 *
					 * @details The stamp grows at every rebuilding, a reference takes it off at its creation, and the
					 *          validity check compares it.
					 *
					 * @note Appending a node to the END of the list touches the stamp deliberately: it changes the
					 *       meaning of no former index, and stamping it would break the references taken during the
					 *       walk of a growing tree
					 *
					 * @warning This trouble was found at the codecs JSON and XML and passed to the neighbours; at
					 *          YAML it was confirmed by a probe — a node removed before a repeated parsing answered
					 *          as fit and issued the content of the NEW document
					 *
					 * \~
					 */
					uint32_t _generation;
				private:
					// Схема, над разбором действовавшая
					schema_t _schema;
					// Настройки разбора документа
					settings_t _settings;
					// Кодировка, какою текст прочитан
					encoding_t _encoding;
					// Положение отказа разбора в исходном тексте
					location_t _location;
				private:
					// Объект потокового чтения записей
					mutable reader_t _reader;
					// Объект записи событий
					mutable writer_t _writer;
				private:
					/**
					 * \~russian
					 * Код ошибки разбора текста
					 *
					 * @note Изменяемым поле объявлено намеренно: запись текста ведётся
					 *       ЗАПРОСОМ - `dump()` объявлен `const`, - а отказ записи обязан
					 *       достаться потребителю. Без изменяемости запись отдавала бы
					 *       пустую строку молча, и содержимое терялось бы без признака.
					 *       Тем же складом устроены документы INI и TOML
					 *
					 * \~english
					 * Code of the error of the parsing of the text
					 *
					 * @note The field is declared mutable on purpose: the writing of the text is conducted
					 *       BY A REQUEST - `dump()` is declared `const` - and a refusal of the writing has
					 *       to reach the consumer. Without the mutability the writing would return an empty
					 *       string silently, and the content would be lost without a sign. The documents of
					 *       INI and TOML are arranged by the same design
					 *
					 * \~
					 */
					mutable error_t _error;
				private:
					/**
					 * \~russian
					 * Удержанный исходный текст, настройкою затребованный
					 *
					 * @note Держится он в кодировке подачи своей: смещения событий чтение
					 *       считает по тексту, приведённому к UTF-8, и дословная
					 *       перезапись вырезает из него с поправкой на метку порядка байтов, а текст в иной кодировке оттого не удерживается
					 *
					 * \~english
					 * The retained source text, demanded by the setting
					 *
					 * @note It is held in the encoding of its own feeding: the reading counts the offsets of
					 *       the events over the text converted to UTF-8, and the verbatim
					 *       rewriting cuts out of it with a correction for the byte order mark, while a text in another encoding is not retained
					 *
					 * \~
					 */
					string _source;
					// Хранилище имён и записей значений
					string _storage;
				private:
					/**
					 * \~russian
					 * Признаки того, что документ черту конца своего в тексте нёс
					 *
					 * @details Довод тот же, что у черты начала: черта необязательна, и
					 *          записывается лишь тогда, когда в исходном тексте стояла.
					 *
					 * \~english
					 * Signs that the document carried the marker of its end in the text
					 *
					 * @details The argument is the same as that of the start marker: the marker is optional, and it
					 *          is written only when it stood in the source text.
					 *
					 * \~
					 */
					vector <bool> _finals;
					/**
					 * \~russian
					 * Признаки того, что документ наречие своё директивой объявил
					 *
					 * @note Директива есть принадлежность документа, а не потока, и стоять она
					 *       обязана перед каждою чертою начала своею: перезапись возвращает её
					 *       ровно тем документам, у каких она стояла
					 *
					 * \~english
					 * Signs that a document declared its dialect by a directive
					 *
					 * @note A directive belongs to a document and not to the stream, and it has to stand
					 *       before the marker of the start of each one: the rewriting returns it exactly to
					 *       those documents before which it stood
					 *
					 * \~
					 */
					vector <bool> _versions;
					/**
					 * \~russian
					 * Признаки того, что документ черту начала своего в тексте нёс
					 *
					 * @details Черта эта необязательна, и записывается она лишь тогда, когда в
					 *          исходном тексте стояла: снять её у текста, её имевшего, значило бы менять
					 *          написание там, где потребитель ничего не менял, - ровно как и навязать её
					 *          тексту, её не имевшему. Довод тот же, что у директивы наречия.
					 *
					 * @note Признак берётся из удержанного текста: чтение черты особым событием
					 *       не выдаёт вовсе, и узнать о ней иначе нечем
					 *
					 * \~english
					 * Signs that the document carried the marker of its start in the text
					 *
					 * @details This marker is optional, and it is written only when it stood in the source text: to
					 *          take it away from a text that had it would mean changing the notation there where the
					 *          consumer changed nothing — exactly as imposing it on a text that had it not. The
					 *          argument is the same as that of the dialect directive.
					 *
					 * @note The sign is taken from the retained text: the reading does not issue the marker by a
					 *       special event at all, and there is no other way to learn of it
					 *
					 * \~
					 */
					vector <bool> _explicits;
				private:
					// Плоский перечень узлов дерева документа
					vector <node_t> _nodes;
					// Перечень свойств узлов, записью им предпосланных
					vector <props_t> _props;
					/**
					 * \~russian
					 * Наречия, каждым документом потока объявленные
					 *
					 * @details Директива `%YAML 1.1` переводит на наречие 1.1 один свой документ,
					 *          а поток вправе нести документы обоих наречий. Схема одна на поток
					 *          отдавала наречие документа последнего всякому узлу: записи `on` да
					 *          `0b1010`, прочтённые наречием 1.1, возвращались строкою у того, кто
					 *          снимал их владеющим значением. Нашёл это ворошитель круговым ходом.
					 *
					 * \~english
					 * Dialects declared by each document of the stream
					 *
					 * @details The `%YAML 1.1` directive switches to the 1.1 dialect one document of its own,
					 *          while a stream is entitled to carry documents of both dialects. A single schema
					 *          over the stream gave the dialect of the last document to every node: the records
					 *          `on` and `0b1010`, read by the 1.1 dialect, returned as strings for whoever took
					 *          them as an owning value. The fuzzer found this by a round trip.
					 *
					 * \~
					 */
					vector <schema_t> _dialects;
				private:
					/**
					 * \~russian
					 * Перечень номеров узлов пар разбираемого отображения
					 *
					 * @details Нужен розыску повторяющихся имён пар при закрытии отображения:
					 *          пары собираются сюда, упорядочиваются по имени и сличаются с соседними,
					 *          отчего розыск стоит столько, сколько стоит сортировка имён, а не квадрат их.
					 *
					 * @note Перечень переиспользуется между отображениями одного разбора: вместимость
					 *       его после первого широкого отображения уже достаточна, и дальнейшие закрытия памяти
					 *       не берут. Чистится он на входе в розыск, а не на выходе, и вне розыска пуст
					 *
					 * \~english
					 * List of the numbers of the nodes of the pairs of a mapping being parsed
					 *
					 * @details Needed by the search of the repeating names of the pairs at the closing of
					 *          a mapping: the pairs are collected here, ordered by name and compared with the
					 *          neighbors, so the search costs a sorting of the names rather than their square.
					 *
					 * @note The list is reused between the mappings of a single parse: its capacity
					 *       after the first wide mapping is already sufficient, and the further closings
					 *       take no memory. It is cleared on entering the search, not on leaving it, and
					 *       outside the search it stands empty
					 *
					 * \~
					 */
					vector <uint32_t> _order;
					// Номера корневых узлов документов текста
					vector <uint32_t> _roots;
					/**
					 * \~russian
					 * Смещения начала документов в удержанном исходном тексте
					 *
					 * @details Перечень этот идёт об руку с перечнем корней: смещение отвечает
					 *          началу строки, документ открывающей, - черте `---` либо первой строке
					 *          содержимого его. Документ первый начинается всегда с нуля: директивы,
					 *          черте предпосланные, принадлежат ему.
					 *
					 * \~english
					 * Offsets of the beginnings of the documents in the retained source text
					 *
					 * @details This list goes hand in hand with the list of the roots: the offset answers the
					 *          beginning of the line that opens the document - the `---` marker or the first
					 *          line of its content. The first document always begins from zero: the directives
					 *          placed before the marker belong to it.
					 *
					 * \~
					 */
					vector <uint32_t> _starts;
				private:
					/**
					 * \~russian
					 * Указатели имён пар отображений, по требованию заведённые
					 *
					 * @details Ключом стоит номер узла отображения, значением - имена детей его,
					 *          на номера узлов отображённые. Заводится указатель лишь тем отображениям,
					 *          у каких пар больше `INDEX_THRESHOLD`, и лишь при первом обращении по имени:
					 *          отображение, к какому по имени не обращались, не платит ничего.
					 *
					 * @warning Имена держатся здесь видами на хранилище, а не своими записями, и
					 *          всякая правка дерева указатели те обращает в прах: перенос записи
					 *          в хранилище вправе переселить его целиком, а вставка узла и снос
					 *          его сдвигают номера. Оттого правка обязана указатели сбрасывать,
					 *          и сброс тот стоит у самих действий - у переноса записи, у вставки
					 *          узла и у сноса его, - а не у каждого способа наружного
					 *
					 * @warning Заведение идёт по требованию, из способа неизменяющего, оттого поле
					 *          и объявлено изменчивым: обращение по имени к одному дереву из двух
					 *          потоков разом требует ограды от потребителя. Дерево, к какому
					 *          обращаются лишь читая и лишь из одного потока, ограды не требует
					 *
					 * \~english
					 * Indexes of the names of the mappings, created on demand
					 *
					 * @details The key here is the number of the node of a mapping, the value — the names of its children
					 *          mapped onto the numbers of the nodes. An index is created only for those mappings which have
					 *          more pairs than `INDEX_THRESHOLD`, and only at the first access by a name: a mapping
					 *          which has not been accessed by a name pays nothing.
					 *
					 * @warning The names are held here as the views onto the storage rather than as their own records, and
					 *          any editing of the tree turns those indexes into a dust: a transfer of a record into
					 *          the storage is entitled to relocate it in full, while an insertion of a node and a removal
					 *          of it shift the numbers. Hence the editing must reset the indexes, and that reset stands
					 *          at the actions themselves — at the transfer of a record, at the insertion of a node and at
					 *          the removal of it — rather than at every outward method
					 *
					 * @warning The mechanism operates on demand, stemming from a non-mutating approach—which is why the field
					 *          is designated as mutable: accessing a specific tree by name from two threads simultaneously
					 *          requires synchronization on the part of the consumer. A tree accessed solely for reading and
					 *          from only a single thread requires no such synchronization
					 *
					 * \~
					 */
					mutable unordered_map <uint32_t, unordered_map <string_view, uint32_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод переноса записи в хранилище знаков
					 *
					 * @param text переносимая запись
					 * @return     смещение перенесённой записи в хранилище
					 *
					 * \~english
					 * @brief Method of the transfer of a record into the storage of the characters
					 *
					 * @param text record being transferred
					 * @return     offset of the transferred record in the storage
					 *
					 * \~
					 */
					uint32_t deposit(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод постройки дерева по выданным чтением событиям
					 *
					 * @param reader чтение, события выдающее
					 * @return       признак успешной постройки дерева
					 *
					 * \~english
					 * @brief Method of the building of the tree by the events issued by the reading
					 *
					 * @param reader reading issuing the events
					 * @return       sign of the successful building of the tree
					 *
					 * \~
					 */
					bool digest(reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по поддереву узла
					 *
					 * @param writer сборка текста
					 * @param index  номер узла, поддерево какого собирается
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the subtree of a node
					 *
					 * @param writer assembling of the text
					 * @param index  index of the node whose subtree is being assembled
					 *
					 * \~
					 */
					void compose(writer_t & writer, const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения границы записи узла в удержанном исходном тексте
					 *
					 * @details Границею служит начало записи следующего узла обхода: всякий байт
					 *          текста принадлежит ровно одному узлу, и примечания, между соседями стоящие,
					 *          достаются тому, над кем они стоят.
					 *
					 * @param index номер узла, границы записи какого получаются
					 * @return      смещение за концом записи узла, `NO_ORIGIN` - записи нет
					 *
					 * \~english
					 * @brief Method of the obtaining of the boundary of the record of a node in the retained source text
					 *
					 * @details The boundary is the beginning of the record of the next node of the traversal: every
					 *          byte of the text belongs to exactly one node, and the comments standing between the
					 *          neighbours fall to the one above whom they stand.
					 *
					 * @param index index of the node whose boundary of the record is being obtained
					 * @return      offset past the end of the record of the node
					 *
					 * \~
					 */
					uint32_t bound(const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод опознания предисловия, блочному значению соседа сверху принадлежащего
					 *
					 * @param index  номер узла, предисловие которому предпослано
					 * @param origin начало предисловия узла в удержанном исходном тексте
					 * @return       отступ строки блочного значения соседа сверху либо `npos`
					 *
					 * \~english
					 * @brief Method of the recognition of a preface belonging to the block value of the neighbour above
					 *
					 * @param index  index of the node the preface is placed before
					 * @param origin beginning of the preface of the node in the retained source text
					 * @return       indentation of the line of the block value of the neighbour above, or `npos`
					 *
					 * \~
					 */
					size_t blocked(const uint32_t index, const uint32_t origin) const noexcept;
					/**
					 * \~russian
					 * @brief Метод дословной записи пролёта соседних узлов исходными байтами
					 *
					 * @details Пролёт переписывается дословно лишь тогда, когда ни один узел его
					 *          правкой не тронут и отступ записи совпадает с отступом исходным: иначе
					 *          перенесённые байты легли бы не на своё место.
					 *
					 * @param writer сборка текста
					 * @param first  номер первого узла переписываемого пролёта
					 * @param last   номер последнего узла переписываемого пролёта
					 * @param entry  признак того, что пролёт переписывается записями перечня
					 * @return       признак успешной дословной записи пролёта
					 *
					 * \~english
					 * @brief Method of the verbatim writing of a run of the neighbouring nodes by the source bytes
					 *
					 * @details The run is rewritten verbatim only when none of its nodes is touched by an editing
					 *          and the indentation of the writing matches the source indentation: otherwise the
					 *          transferred bytes would fall not in their own place.
					 *
					 * @param writer assembling of the text
					 * @param first  index of the first node of the run being rewritten
					 * @param last   index of the last node of the run being rewritten
					 * @param entry  sign of the fact that the run is rewritten by entries of a sequence
					 * @return       sign of the successful verbatim writing of the run
					 *
					 * \~
					 */
					bool verbatim(writer_t & writer, const uint32_t first, const uint32_t last, const bool entry, const bool gapped) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки детей вместилища пролётами нетронутых
					 *
					 * @param writer сборка текста
					 * @param index  номер узла вместилища, дети какого собираются
					 *
					 * \~english
					 * @brief Method of the assembling of the children of a container by the runs of the untouched ones
					 *
					 * @param writer assembling of the text
					 * @param index  index of the node of the container whose children are being assembled
					 *
					 * \~
					 */
					bool enter(writer_t & writer, const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи предисловия узла дословными исходными байтами
					 *
					 * @param writer сборка текста
					 * @param index  номер узла, предисловие какого выдаётся
					 * @param gapped признак разрыва удержанного текста перед записью узла
					 *
					 * \~english
					 * @brief Method of the output of the preface of a node by the verbatim source bytes
					 *
					 * @param writer assembling of the text
					 * @param index  index of the node whose preface is being output
					 * @param gapped sign of the gap of the retained text before the record of the node
					 *
					 * \~
					 */
					void preamble(writer_t & writer, const uint32_t index, const bool gapped) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения начала собственной строки узла
					 *
					 * @details Начало записи узла считается вместе с примечаниями да пустыми
					 *          строками, ему предшествующими, а строка собственная стоит за ними: розыск
					 *          её пропускает строки пустые и несущие одно примечание.
					 *
					 * @param index номер узла, собственная строка какого разыскивается
					 * @return      смещение начала собственной строки, `NO_ORIGIN` - записи нет
					 *
					 * \~english
					 * @brief Method of the obtaining of the beginning of the own line of a node
					 *
					 * @details The beginning of the record of a node is counted together with the comments and the
					 *          blank lines preceding it, while its own line stands after them: the search for it
					 *          skips the lines that are empty and that carry a comment alone.
					 *
					 * @param index index of the node whose own line is being sought
					 * @return      offset of the beginning of the own line of the node
					 *
					 * \~
					 */
					uint32_t leading(const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод подрезки границ записей узлов, в снимаемый упирающихся
					 *
					 * @param index номер снимаемого узла
					 *
					 * \~english
					 * @brief Method of the trimming of the boundaries of the records of the nodes bordering
					 *        the removed one
					 *
					 * @param index index of the node being removed
					 *
					 * \~
					 */
					void tighten(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения конца содержимого блочного значения, документ замыкающего
					 *
					 * @param root номер корневого узла документа
					 * @return     смещение за концом содержимого блока, `NO_ORIGIN` - блока нет
					 *
					 *
					 * \~english
					 * @brief Method of the obtaining of the end of the content of a block scalar closing
					 *        the document
					 *
					 * @param root number of the root node of the document
					 * @return     offset past the end of the content of the block, `NO_ORIGIN` - no block
					 *
					 * \~
					 */
					uint32_t trailing(const uint32_t root) const noexcept;
					/**
					 * \~russian
					 * @brief Метод опознания блочного значения, хвост свой сохраняющего
					 *
					 * @param index номер опознаваемого узла
					 * @return      признак блочного значения, хвост свой сохраняющего
					 *
					 *
					 * \~english
					 * @brief Method of the recognition of a block scalar that keeps its own tail
					 *
					 * @param index index of the node being recognised
					 * @return      sign of a block scalar that keeps its own tail
					 *
					 * \~
					 */
					bool hanging(const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод отнесения пустых строк к узлам, под ними стоящим
					 *
					 * @details Пустая строка, узлу предшествующая, отделяет его от соседа сверху и
					 *          уходить обязана вместе с ним. Чтение о таких строках не извещает - простое
					 *          значение, выдачи ожидающее, считает их складками своими, - и разыскиваются
					 *          они по удержанному тексту прямо.
					 *
					 * @note Розыск не заходит за узел, блочное значение несущий: пустые строки в
					 *       конце его суть содержимое, правилом усечения сохранённое, а вовсе не
					 *       отступ перед соседом
					 *
					 * \~english
					 * @brief Method of the attribution of the empty lines to the nodes standing under them
					 *
					 * @details An empty line preceding a node separates it from the neighbour above and has to
					 *          leave together with it. The reading does not inform of such lines - a plain value
					 *          awaiting the issuing counts them as its own folds - and they are searched for straight
					 *          over the retained text.
					 *
					 * @note The search does not go past a node carrying a block scalar: the empty lines at its end
					 *       are the content, kept by the rule of truncation, and not at all the indent before a
					 *       neighbour
					 *
					 * \~
					 */
					void spread() noexcept;
					/**
					 * \~russian
					 * @brief Метод пометки узла и предков его правлеными
					 *
					 * @param index номер помечаемого узла
					 *
					 * \~english
					 * @brief Method of the marking of a node and of its ancestors as edited
					 *
					 * @param index index of the node being marked
					 *
					 * \~
					 */
					void mark(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени и значения узла в хранилище знаков
					 *
					 * @details Вид значения решается тем же телом, каким решает его разбор: запись
					 *          кладётся в хранилище, а разрешение да сужение числа зовутся над нею.
					 *          Иначе два свода правил разошлись бы, и число, правкой поставленное, читалось
					 *          бы иначе, нежели то же число, текстом прочитанное.
					 *
					 * @param index номер записываемого узла
					 * @param name  имя пары, пусто у значения перечня
					 * @param text  запись значения, исходным текстом данная
					 * @param style ограда, какою обносится значение
					 *
					 * \~english
					 * @brief Method of the writing of the name and of the value of a node into the storage of the characters
					 *
					 * @details The kind of the value is resolved by the very same body by which the parsing resolves
					 *          it: the record is laid into the storage, and the resolution and the narrowing of the
					 *          number are called upon it. Otherwise the two sets of rules would diverge, and a
					 *          number placed by an editing would read otherwise than the selfsame number read from
					 *          the text.
					 *
					 * @param index index of the node being written
					 * @param name  name of the pair, empty for a value of a sequence
					 * @param text  record of the value as given by the source text
					 * @param style quoting by which the value is enclosed
					 *
					 * \~
					 */
					void inscribe(const uint32_t index, const string_view name, const string_view text, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения узла последним ребёнком вместилища
					 *
					 * @param owner номер вместилища, ребёнок какого заводится
					 * @return      номер заведённого узла
					 *
					 * \~english
					 * @brief Method of the creation of a node as the last child of a container
					 *
					 * @param owner index of the container whose child is being created
					 * @return      index of the created node
					 *
					 * \~
					 */
					uint32_t implant(const uint32_t owner) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления дерева правленым при уходе метки
					 *
					 * @details Метка, правкой уходящая, оставила бы ссылки на себя без
					 *          объявления, а ссылки эти держатся дословным переносом исходных байтов:
					 *          раскрытие их дерево помнит, а запись `*метка` вернулась бы в перезапись
					 *          как есть. Оттого дерево целиком объявляется правленым - раскрытия
					 *          соберутся заново значениями своими, и ссылок в тексте не останется.
					 *
					 * @note Правило это едино для снятия узла и для замены его скалярным
					 *       значением: уходит поддерево и там, и там, а разойдись оно
					 *       телами - один из путей потерял бы его молча
					 *
					 * @param index  номер первого узла уходящего поддерева
					 * @param extent размах уходящего поддерева в узлах
					 *
					 * \~english
					 * @brief Method of the declaring of a tree as edited at the leaving of an anchor
					 *
					 * @details An anchor leaving by an editing would leave the references to it without a
					 *          declaration, and those references are held by the verbatim transfer of the source
					 *          bytes: the tree remembers their expansion, and the record `*anchor` would come back
					 *          into the rewrite as it stands. Whereby the whole tree is declared edited — the
					 *          expansions will be gathered anew by their own values, and no references will remain
					 *          in the text.
					 *
					 * @note The rule is one for the removal of a node and for the replacement of it by a scalar
					 *       value: a subtree leaves in both cases, and were it split into two bodies, one of the
					 *       paths would lose it silently
					 *
					 * @param index  index of the first node of the leaving subtree
					 * @param extent extent of the leaving subtree in the nodes
					 *
					 * \~
					 */
					void disown(const uint32_t index, const uint32_t extent) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения конца последней строки записи поддерева
					 *
					 * @details Границы записей узлов к этому времени ещё не сочтены: считаются
					 *          они по началам записей, а начала эти правятся переносом черты. Оттого
					 *          нижний предел переноса берётся не границею прежней записи, а концом
					 *          последней строки её - тем, что известно и до счёта границ.
					 *
					 * @param index    номер узла, поддерево какого обходится
					 * @param boundary предел, ниже какого опускаться нельзя
					 * @return         смещение за концом последней строки записи
					 *
					 * \~english
					 * @brief Method of the obtaining of the end of the last line of the record of a subtree
					 *
					 * @details The boundaries of the records of the nodes are not yet counted by this time: they are
					 *          counted by the beginnings of the records, while those beginnings are changed by the
					 *          moving of the dash. Whereby the lower bound of the moving is taken not by the
					 *          boundary of the former record, but by the end of its last line — that which is known
					 *          even before the counting of the boundaries.
					 *
					 * @param index    index of the node whose subtree is being traversed
					 * @param boundary limit below which it is not allowed to descend
					 * @return         offset past the end of the last line of the record
					 *
					 * \~
					 */
					uint32_t closing(const uint32_t index, const uint32_t boundary) const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия узла вместе с поддеревом его
					 *
					 * @param index номер снимаемого узла
					 * @return      признак успешного снятия узла
					 *
					 * \~english
					 * @brief Method of the removal of a node together with its subtree
					 *
					 * @param index index of the node being removed
					 * @return      sign of the successful removal of the node
					 *
					 * \~
					 */
					bool extract(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод отделения блоков с сохранением хвоста от соседей снизу
					 *
					 * \~english
					 * @brief Method of the severing of the blocks with the tail preserved from the neighbours below
					 *
					 * \~
					 */
					void sever() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора повторяющихся имён пар отображения
					 *
					 * @details Описание языка имена пар отображения объявляет неповторимыми, а
					 *          обхождение с повторами оставляет читающему: правило берётся из настроек
					 *          разбора, и по умолчанию повтор отвергается отказом.
					 *
					 * @note Разбор ведётся при закрытии отображения, а не при заведении пары:
					 *       имя пары становится известно раньше значения её, но поддерево
					 *       значения дописывается позже, и снимать пару прежде конца нельзя
					 *
					 * @param parent  номер узла разбираемого отображения пар
					 * @param anchors имена меток, документом объявленные, вместе с номерами узлов их
					 * @param reader  поток разбора, положение отказа несущий
					 * @return        признак успешного разбора
					 *
					 * \~english
					 * @brief Method of the handling of the repeating names of the pairs of a mapping
					 *
					 * @details The description of the language declares the names of the pairs of a mapping
					 *          non-repeating, while the treatment of the repeats is left to the consumer: the rule
					 *          is taken from the settings of the parsing, and by default a repeat is refused.
					 *
					 * @note The handling is conducted at the closing of a mapping and not at the creation of a pair:
					 *       the name of a pair becomes known before its value, but the subtree of the value is
					 *       appended later, and a pair cannot be removed before the end
					 *
					 * @param parent  index of the node of the mapping being handled
					 * @param anchors names of the anchors declared by the document, with the indexes of their nodes
					 * @param reader  parsing stream carrying the location of a refusal
					 * @return        sign of the successful handling
					 *
					 * \~
					 */
					bool deduplicate(const uint32_t parent, std::unordered_map <std::string, uint32_t> & anchors, const reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод розыска узла по пути к нему с заведением недостающего
					 *
					 * @param path   путь к разыскиваемому узлу
					 * @param index  номер найденного либо заведённого узла
					 * @param create признак заведения узла, розыском не найденного
					 * @return       признак успешного розыска узла
					 *
					 * \~english
					 * @brief Method of the search of a node by the path to it with the creation of a missing one
					 *
					 * @param path   path to the node being sought
					 * @param index  index of the found or created node
					 * @param create sign of the creation of a node not found by the search
					 * @return       sign of the successful search of the node
					 *
					 * \~
					 */
					bool place(const string & path, uint32_t & index, const bool create) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки свойств узла дерева
					 *
					 * @param index  номер узла, свойства которому ставятся
					 * @param anchor устанавливаемая метка узла, пустая - метку снять
					 * @param tag    устанавливаемая метка типа, пустая - метку снять
					 *
					 * \~english
					 * @brief Method of the setting of the properties of a node of the tree
					 *
					 * @param index  index of the node whose properties are being set
					 * @param anchor anchor of the node being set, empty to remove the anchor
					 * @param tag    type tag being set, empty to remove the tag
					 *
					 * \~
					 */
					void endow(const uint32_t index, const string_view anchor, const string_view tag) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения узла записью его без ограды
					 *
					 * @note Ограда не решается содержимым, а не ставится вовсе: запись числа,
					 *       логического значения да пустоты обязана вернуться обратным чтением тем
					 *       же видом, а ограда обратила бы её в строку
					 *
					 * @param path путь к устанавливаемому узлу
					 * @param text устанавливаемая запись значения
					 * @return     признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of a node by its record without the quoting
					 *
					 * @note The quoting is not decided by the content and is omitted altogether: the record of a
					 *       number, of a logical value and of emptiness is obliged to come back by the reading as
					 *       the very same kind, while a quoting would turn it into a string
					 *
					 * @param path path to the node being set
					 * @param text record of the value being set
					 * @return     sign of the successful setting of the value
					 *
					 * \~
					 */
					bool settle(const string & path, const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия детей узла
					 *
					 * @details Вместилище, значением иным заменяемое, детей своих лишается:
					 *          держать их некуда, и записаны они уже не будут.
					 *
					 * @param index номер узла, детей лишаемого
					 *
					 * \~english
					 * @brief Method of the removal of the children of a node
					 *
					 * @details A container being replaced by another value loses its children:
					 *          there is nowhere to hold them, and they will not be written any more.
					 *
					 * @param index number of the node being deprived of the children
					 *
					 * \~
					 */
					void prune(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения узла заданною оградою
					 *
					 * @param index номер устанавливаемого узла
					 * @param text  устанавливаемая запись значения
					 * @param style ограда, какою обносится значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of a node by a given quoting
					 *
					 * @param index index of the node being set
					 * @param text  record of the value being set
					 * @param style quoting by which the value is enclosed
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool assign(const uint32_t index, const string_view text, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи узла вместе с примечаниями, ему предпосланными
					 *
					 * @details Узел, правкой тронутый, собирается заново, а примечания над ним
					 *          стоящие переносятся дословно: правка значения примечания над ним не
					 *          касается, и терять его было бы неправдой.
					 *
					 * @param writer сборка текста
					 * @param index  номер записываемого узла
					 *
					 * \~english
					 * @brief Method of the writing of a node together with the comments placed before it
					 *
					 * @details A node touched by an editing is gathered anew, while the comments standing above it
					 *          are transferred verbatim: an editing of the value touches no comment above the node,
					 *          and to lose it would be an untruth.
					 *
					 * @param writer assembling of the text
					 * @param index  index of the node being written
					 *
					 * \~
					 */
					void produce(writer_t & writer, const uint32_t index, const bool preface = true, const bool gapped = false) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста в дерево документа
					 *
					 * @param text разбираемый текст
					 * @return     признак успешного разбора текста
					 *
					 * \~english
					 * @brief Method of the parsing of a text into the tree of a document
					 *
					 * @param text text being parsed
					 * @return     sign of the successful parsing of the text
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения текста из файла в дерево документа
					 *
					 * @param filename путь к читаемому файлу
					 * @return         признак успешного чтения текста
					 *
					 * \~english
					 * @brief Method of the reading of a text from a file into the tree of a document
					 *
					 * @param filename path to the file being read
					 * @return         sign of the successful reading of the text
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по дереву документа
					 *
					 * @return собранный текст документа
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the tree of a document
					 *
					 * @return assembled text of the document
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод пометки узла правленым
					 *
					 * @details Помечается им не один узел, а весь путь до корня документа: запись
					 *          узла лежит внутри записи родителя его, и дословно переписать родителя после
					 *          правки ребёнка уже нельзя. Узел помеченный собирается заново, а соседи его,
					 *          правкой не тронутые, переписываются дословными исходными байтами.
					 *
					 * @note Пометка нужна лишь при удержании исходного текста: без него собирается
					 *       заново весь документ, и различать тронутое от нетронутого не для чего
					 *
					 * @param value ссылка на помечаемый узел
					 * @return      признак успешной пометки узла
					 *
					 * \~english
					 * @brief Method of the marking of a node as edited
					 *
					 * @details It marks not one node but the whole path up to the root of the document: the record
					 *          of a node lies inside the record of its parent, and to rewrite the parent verbatim after
					 *          an editing of a child is no longer possible. A marked node is gathered anew, while its
					 *          neighbours, untouched by the editing, are rewritten by the verbatim bytes of the source.
					 *
					 * @note The marking is needed only at the retention of the source text: without it the whole
					 *       document is gathered anew, and there is nothing to tell the touched from the untouched
					 *
					 * @param value reference to the node being marked
					 * @return      sign of the successful marking of the node
					 *
					 * \~
					 */
					bool touch(const value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки строкового значения по пути к нему
					 *
					 * @details Ограда выбирается содержимым, коли не задана прямо: запись `12`,
					 *          значением строковым поставленная, ограду получает - иначе обратное чтение
					 *          вернуло бы её числом. Путь ведётся от корня первого документа, части его
					 *          делятся косою чертой, а часть внутри перечня есть номер значения.
					 *
					 * @note Узла по пути может и не быть: недостающая пара отображения заводится
					 *       последнею, а значение перечня добавляется концом его
					 *
					 * @note Вместилища по пути ЗАВОДЯТСЯ, и заводятся цепочкою: постановка по
					 *       `/а/б/в` у дерева, ни `а`, ни `б` не имеющего, даёт оба отображения
					 *       и значение при них. Замерено 09.09.2026 опытом по трём кодекам:
					 *       INI, TOML и YAML поступают тут ОДИНАКОВО, каждый своим написанием
					 *
					 * @warning Прежде здесь стояло обратное - «части пути, кроме последней,
					 *          обязаны быть налицо, вместилищ по пути не заводится», - и было
					 *          оно ложно замером. Хуже того, кодек INI ссылался на правило это
					 *          как на взятое у YAML дословно, а бридж повторял его третий раз:
					 *          ложь, никем не перемеренная, разошлась по трём местам
					 *
					 * @note Довод в пользу НЕзаведения был и остаётся весом: заведение
					 *       недостающего звена обращает опечатку в пути в новую ветвь дерева
					 *       молча. Но записывать надлежит то, что дерево делает, а желаемое
					 *       проводить правкой кода, и правкой по всем трём кодекам разом
					 *
					 * @note Заведение цепочки закреплено проверками во всех трёх кодеках,
					 *       дабы расхождение это впредь не молчало
					 *
					 * @note Звено, картою НЕ являющееся, вместилищем не подменяется: постановка
					 *       по пути сквозь скаляр отвергается, причина зовётся кодом, а дерево
					 *       остаётся нетронутым. Замерено 09.09.2026: YAML и TOML отвергают
					 *       оба, у INI случай не возникает вовсе - раздел и ключ там разные
					 *       пространства имён, а не звенья одного пути
					 *
					 * @warning Кодеки syslog и CEF поступают тут ИНАЧЕ: вместилище заводят, а
					 *          прежний скаляр теряют молча. Расхождение вынесено к тем кодекам
					 *          09.09.2026 и решением не закрыто; здесь оно записано затем,
					 *          чтобы будущий свод пяти кодеков не выводился из одного YAML
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое строковое значение
					 * @param style ограда, какою обносится значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a string value by the path to it
					 *
					 * @details The quoting is chosen by the content when it is not given outright: the record `12`,
					 *          placed by a string value, receives a quoting — otherwise the reading back would have
					 *          brought it back as a number. The path is led from the root of the first document, its
					 *          parts are divided by a slash, and a part inside a sequence is the index of the value.
					 *
					 * @note There may be no node by the path: a missing pair of a mapping is created as the last
					 *       one, while a value of a sequence is appended to its end
					 *
					 * @note The containers by the path ARE CREATED, and created by a chain: a placement by `/a/b/c`
					 *       at a tree holding neither `a` nor `b` gives both mappings and a value with them. Measured
					 *       on 2026-09-09 by an experiment across three codecs: INI, TOML and YAML act here ALIKE,
					 *       each by its own notation
					 *
					 * @warning Formerly the opposite stood here — "the parts of a path except the last one are
					 *          obliged to be present, the containers by the path are not created" — and it was false
					 *          by measurement. Worse still, the codec INI referred to this rule as taken from YAML
					 *          verbatim, and a bridge repeated it for the third time: a falsehood, remeasured by
					 *          nobody, spread across three places
					 *
					 * @note The argument for NOT creating was and stays weighty: creating a missing link turns a
					 *       typo in a path into a new branch of the tree silently. But one ought to write down what
					 *       the tree does, and to carry the desirable through an editing of the code, and an editing
					 *       across all the three codecs at once
					 *
					 * @note The creation of a chain is fixed by checks in all the three codecs, so that this
					 *       divergence would not be silent hereafter
					 *
					 * @note A link that is NOT a map is not replaced by a container: a placement by a path through a
					 *       scalar is refused, the reason is named by a code, and the tree stays untouched. Measured
					 *       on 2026-09-09: YAML and TOML both refuse, while at INI the case does not arise at all —
					 *       the section and the key there are different namespaces, not links of one path
					 *
					 * @warning The codecs syslog and CEF act here DIFFERENTLY: they create a container but lose the
					 *          former scalar silently. The divergence was assigned to those codecs on 2026-09-09 and
					 *          is not closed by a decision; it is written down here so that a future survey of the
					 *          five codecs would not be derived from YAML alone
					 *
					 * @param path  path to the node being set
					 * @param value string value being set
					 * @param style quoting by which the value is enclosed
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const string_view value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки владеющего значения по пути
					 *
					 * @details Ход этот есть тот же перенос, какой ведёт `value_t::graft()`,
					 *          и зовётся он вторым концом: у дерева, а не у значения. Заведены
					 *          ОБА конца намеренно - у одних кодеков рамки правка стояла ходом
					 *          значения, у других ходом документа, и потребитель, два кодека
					 *          рядом читающий, разыскивал ход не там, где тот лежал.
					 *
					 * @param path  путь постановки значения
					 * @param value ставимое владеющее значение
					 * @return      признак успешной постановки значения
					 *
					 * \~english
					 * @brief Method of setting an owning value by a path
					 *
					 * @details This course is the very same transfer which `value_t::graft()` leads, and it is
					 *          called by the second end: that of the tree, and not that of the value. BOTH ends were
					 *          made deliberately — at some codecs of the framework the editing stood as a course of
					 *          the value, at others as a course of the document, and a consumer reading two codecs
					 *          side by side sought the course not where it lay.
					 *
					 * @param path  path of the setting of the value
					 * @param value owning value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const codec::yaml::Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки строкового значения узла записью дословною
					 *
					 * @details Перегрузка эта заведена от ЗАПАДНИ языка: без неё написание
					 *          `set(путь, "значение")` уходило в перегрузку от логического значения -
					 *          приведение `const char *` к `bool` есть приведение стандартное, а к
					 *          `string_view` пользовательское, и первое побеждает молча. Дерево получало
					 *          `ключ: true` вместо строки, и отказа при том не было никакого.
					 *
					 * @note Устроено складом кодека TOML, где перегрузка эта заведена от того же
					 *       довода. Пустой указатель обращается в пустую строку
					 *
					 * @param path  путь к устанавливаемому узлу дерева документа
					 * @param value устанавливаемое значение узла
					 * @param style вид записи устанавливаемого значения
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief The method for setting the string value of a node by a literal record
					 *
					 * @details This overload was made because of a TRAP of the language: without it the notation
					 *          `set(path, "value")` went into the overload of the logical value — the conversion of
					 *          `const char *` to `bool` is standard, while that to `string_view` is user-defined,
					 *          and the former wins silently. The tree received `key: true` instead of a string, and
					 *          there was no refusal at all.
					 *
					 * @note It is arranged as the make of the codec TOML, where this overload is made from the
					 *       selfsame argument. A null pointer turns into an empty string
					 *
					 * @param path  the path to the node being set in the document tree
					 * @param value the value being set for the node
					 * @param style the record kind of the value being set
					 * @return      the result of the operation
					 *
					 * \~
					 */
					bool set(const string & path, const char * value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки логического значения по пути к нему
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое логическое значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a boolean value by the path to it
					 *
					 * @param path  path to the node being set
					 * @param value boolean value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки целого значения по пути к нему
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое целое значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of an integer value by the path to it
					 *
					 * @param path  path to the node being set
					 * @param value integer value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки дробного значения по пути к нему
					 *
					 * @note Записывается оно семнадцатью значащими разрядами - столько нужно,
					 *       чтобы всякое число двойной точности вернулось обратным чтением тем же
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое дробное значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a floating point value by the path to it
					 *
					 * @note It is written by seventeen significant digits — that much is needed so that every
					 *       double-precision number comes back by the reading as the very same one
					 *
					 * @param path  path to the node being set
					 * @param value floating point value being set
					 * @return      sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки пустого значения по пути к нему
					 *
					 * @note Пустое значение от снятия узла отлично: пара остаётся налицо, а
					 *       значения при ней нет
					 *
					 * @param path путь к устанавливаемому узлу
					 * @return     признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of an empty value by the path to it
					 *
					 * @note An empty value is distinct from the removal of a node: the pair stays present, while
					 *       there is no value at it
					 *
					 * @param path path to the node being set
					 * @return     sign of the successful setting of the value
					 *
					 * \~
					 */
					bool reset(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки свойств узла дерева по пути к нему
					 *
					 * @details Свойства узла - метка его да метка типа - переносом значения прежде
					 *          терялись: укладка вела запись одними значениями, а свойств узлу поставить было
					 *          нечем. Двоичное содержимое тем теряло вид свой - `!!binary 12:30`, в чужое
					 *          дерево перенесённое, наречием 1.1 читалось обратно числом семьсот пятьдесят.
					 *
					 * @note Пустая запись свойство СНИМАЕТ: тем зовущий волен и снять метку, а не
					 *       только поставить
					 *
					 * @param path   путь к узлу, свойства которому ставятся
					 * @param anchor устанавливаемая метка узла, пустая - метку снять
					 * @param tag    устанавливаемая метка типа, пустая - метку снять
					 * @return       признак успешной установки свойств
					 *
					 * \~english
					 * @brief Method of the setting of the properties of a node of the tree by the path to it
					 *
					 * @details The properties of a node — its anchor and its type tag — were lost by the transfer of
					 *          a value before: the laying led the records by the values alone, and there was nothing
					 *          with which to set the properties to a node. The binary content thereby lost its own
					 *          kind — `!!binary 12:30`, transferred into a foreign tree, was read back by the 1.1
					 *          dialect as the number seven hundred and fifty.
					 *
					 * @note An empty record REMOVES a property: thereby the caller is free to remove an anchor, and
					 *       not only to set one
					 *
					 * @param path   path to the node the properties are being set to
					 * @param anchor anchor being set, empty to remove it
					 * @param tag    tag being set, empty to remove it
					 * @return       sign of the successful setting of the properties
					 *
					 * \~
					 */
					bool endow(const string & path, const string_view anchor, const string_view tag) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления узла вместилищем
					 *
					 * @details Объявленное вместилище пусто, а наполняется оно установкою
					 *          значений по пути внутрь него: части пути, кроме последней, обязаны быть
					 *          налицо, и объявление это их и заводит. Прежнее содержимое узла
					 *          объявлением снимается - вместе со всем поддеревом его.
					 *
					 * @note Узла по пути может и не быть: заводится он тем же порядком, каким
					 *       заводит его установка значения
					 *
					 * @param path     путь к объявляемому узлу
					 * @param sequence признак объявления перечня значений заместо отображения пар
					 * @return         признак успешного объявления вместилища
					 *
					 * \~english
					 * @brief Method of the declaring of a node as a container
					 *
					 * @details The declared container is empty and is filled by the setting of the
					 *          values by a path inside it: the parts of a path except the last one must be
					 *          present, and this declaring establishes them. The previous content of the node
					 *          is removed by the declaring — together with the whole of its subtree.
					 *
					 * @note The node by the path may be absent: it is established by the same order by which
					 *       the setting of a value establishes it
					 *
					 * @param path     path to the node being declared
					 * @param sequence flag of the declaring of a sequence of the values instead of a mapping of the pairs
					 * @return         sign of the successful declaring of the container
					 *
					 * \~
					 */
					bool arrange(const string & path, const bool sequence = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения дословною записью
					 *
					 * @details Ограды запись не получает никакой: кладётся она в дерево как
					 *          подана, и вид значения решается обратным чтением её. Тем и отличается
					 *          установка эта от строковой: та ограду подбирает содержимым, дабы запись
					 *          `12` числом не обернулась.
					 *
					 * @note Способ этот берут значения, вида родного не имеющие вовсе -
					 *       отметки времени, двоичное содержимое и числа, ни в один родной вид
					 *       не вместимые: записью своею они и держатся
					 *
					 * @param path   путь к устанавливаемому узлу
					 * @param record устанавливаемая запись значения
					 * @return       признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value by a verbatim record
					 *
					 * @details The record receives no quoting at all: it is laid into the tree as it is given, and
					 *          the kind of the value is decided by the reading back of it. By this the setting differs
					 *          from the string one: that one picks the quoting by the content, so that the record `12`
					 *          does not turn into a number.
					 *
					 * @note This way is taken by the values having no native kind at all —
					 *       the timestamps, the binary content and the numbers not fitting into any native kind
					 *
					 * @param path   path to the node being set
					 * @param record record of the value being set
					 * @return       sign of the successful setting of the value
					 *
					 * \~
					 */
					bool imprint(const string & path, const string_view record) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия узла вместе с поддеревом его
					 *
					 * @details Снимается узел целиком - вместе с именем пары, значением её и всем
					 *          поддеревом. Записи его в исходном тексте достаются небытию: перезапись их не
					 *          переносит, а примечания, над узлом стоявшие, уходят вместе с ним.
					 *
					 * @param path путь к снимаемому узлу
					 * @return     признак успешного снятия узла
					 *
					 * \~english
					 * @brief Method of the removal of a node together with its subtree
					 *
					 * @details A node is removed in full — together with the name of the pair, its value and the
					 *          whole subtree. Its records in the source text fall to nothing: the rewrite does not
					 *          carry them, and the comments that stood above the node leave together with it.
					 *
					 * @param path path to the node being removed
					 * @return     sign of the successful removal of the node
					 *
					 * \~
					 */
					bool erase(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по дереву документа заданными настройками
					 *
					 * @param settings настройки записи собираемого текста
					 * @return         собранный текст документа
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the tree of a document by given settings
					 *
					 * @param settings settings of the writing of the text being assembled
					 * @return         assembled text of the document
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи текста документа в файл
					 *
					 * @param filename путь к записываемому файлу
					 * @return         признак успешной записи текста
					 *
					 * \~english
					 * @brief Method of the writing of the text of a document into a file
					 *
					 * @param filename path to the file being written
					 * @return         sign of the successful writing of the text
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества документов текста
					 *
					 * @return количество документов, текстом несомых
					 *
					 * \~english
					 * @brief Method of the obtaining of the number of the documents of a text
					 *
					 * @return number of the documents carried by the text
					 *
					 * \~
					 */
					size_t documents() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кодировки, какою текст прочитан
					 *
					 * @details Опознаётся она по метке порядка байтов, а при отсутствии её - по
					 *          расположению нулевых байтов в первых четырёх октетах, либо навязывается
					 *          настройками прямо.
					 *
					 * @note Удержание исходного текста работает лишь у кодировки UTF-8: смещения
					 *       событий чтение считает по тексту, к ней приведённому, и дословный
					 *       перенос из текста иной кодировки лёг бы вперемешку
					 *
					 * @return кодировка, какою текст прочитан
					 *
					 * \~english
					 * @brief Method of the obtaining of the encoding by which the text has been read
					 *
					 * @details It is recognised by the byte order mark, while in its absence — by the arrangement of
					 *          the zero bytes in the first four octets, or it is imposed by the settings outright.
					 *
					 * @note The retention of the source text works only at the UTF-8 encoding: the reading counts
					 *       the offsets of the events over the text brought to it, and a verbatim transfer from a
					 *       text of another encoding would fall in a jumble
					 *
					 * @return encoding by which the text has been read
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения ссылки на корень первого документа текста
					 *
					 * @return ссылка на корень документа
					 *
					 * \~english
					 * @brief Method of the obtaining of a reference to the root of the first document of a text
					 *
					 * @return reference to the root of the document
					 *
					 * \~
					 */
					value_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения ссылки на корень документа по номеру его
					 *
					 * @param index номер документа в потоке текста
					 * @return      ссылка на корень документа
					 *
					 * \~english
					 * @brief Method of the obtaining of a reference to the root of a document by its index
					 *
					 * @param index index of the document in the stream of the text
					 * @return      reference to the root of the document
					 *
					 * \~
					 */
					value_t root(const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к узлу дерева по пути
					 *
					 * @details Ход этот общий у всех семи кодеков рамки, и спрашивается он у
					 *          ДОКУМЕНТА, а не у значения: общие проверки договора зовут
					 *          `document.at(путь)`, и кодек, такого хода не несущий, в перебор
					 *          по семерым не входит вовсе.
					 *
					 * @note Путь берётся по описанию RFC 6901: звено внутри перечня есть номер,
					 *       а внутри отображения - имя, хотя бы и записанное цифрами
					 *
					 * @param path путь до разыскиваемого узла
					 * @return     разысканный узел дерева документа
					 *
					 * \~english
					 * @brief Method of accessing a node of the tree by a path
					 *
					 * @details This course is common to all the seven codecs of the framework, and it is asked of
					 *          the DOCUMENT, and not of the value: the common checks of the contract call
					 *          `document.at(path)`, and a codec carrying no such course does not enter the survey of
					 *          the seven at all.
					 *
					 * @note The path is taken by the description of RFC 6901: a link inside a sequence is an index,
					 *       while inside a mapping it is a name, even one written as digits
					 *
					 * @param path path to the node being searched for
					 * @return     found node of the tree of the document
					 *
					 * \~
					 */
					value_t at(const string & path) const noexcept;
							/**
					 * \~russian
					 * @brief Метод проверки наличия значения по пути
					 *
					 * @details Ход этот общий у всех семи кодеков рамки. Путь берётся по описанию
					 *          RFC 6901: звено внутри перечня есть номер, а внутри отображения -
					 *          имя, хотя бы и записанное цифрами.
					 *
					 * @param path путь до проверяемого значения
					 * @return     признак наличия значения по пути
					 *
					 * \~english
					 * @brief Method of checking the presence of a value by a path
					 *
					 * @details This course is common to all the seven codecs of the framework. The path is taken by
					 *          the description of RFC 6901: a link inside a sequence is an index, while inside a
					 *          mapping it is a name, even one written as digits.
					 *
					 * @param path path to the value being checked
					 * @return     sign of the presence of the value by the path
					 *
					 * \~
					 */
					bool has(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перечисления звеньев пути к детям значения
					 *
					 * @details Выдаются НЕ ИМЕНА детей, а ЗВЕНЬЯ ПУТИ к ним: такие, что
					 *          `at(путь + "/" + звено)` неминуемо ведёт к тому самому ребёнку. У
					 *          вместилища пар звеном служит имя пары, у перечня значений - номер.
					 *
					 * @details Ход этот вместе с `at()` даёт ПОЛНЫЙ ОБХОД документа, общий у
					 *          всех кодеков рамки. Обход ЗАМКНУТ - всякое выданное звено годно для
					 *          `at()`, - и замкнутость эта держится на отменяющих записях RFC 6901:
					 *          имя, косую черту либо тильду несущее, выдаётся ЗАПИСАННЫМ (`~1` и
					 *          `~0`), а не как есть.
					 *
					 * @note Перечисление НИКОГДА не отвечает отказом: значение простое, пустое
					 *       вместилище и путь в никуда отвечают пустым перечнем ОДИНАКОВО, а
					 *       разнятся они ходом `has()`
					 *
					 * @warning Замкнутость эта держится на документе, СТАНДАРТУ ОТВЕЧАЮЩЕМ: YAML
					 *          1.2 требует имена пар отображения уникальными, и умолчание разбора
					 *          повтор отвергает. Под послаблением `duplicate_t::KEEP`, повторы
					 *          удерживающим, обход РАЗОМКНУТ по природе: имя выдаётся столько раз,
					 *          сколько пар его несут, а розыск по имени отдаёт первую из них -
					 *          вторая недостижима ничем. Замерено; беспечная проверка того не
					 *          ловит, ибо `has()` и `valid()` на таком звене отвечают истиной
					 *
					 * @warning Держать это умолчанием нельзя: мост, кодек деревом обходящий,
					 *          потерял бы пары молча. Чинить сменою грамматики звена - выдачей
					 *          `имя[1]` - отказано намеренно: звено перестало бы быть именем, и
					 *          у кодека завелась бы своя запись пути, которой нет у прочих.
					 *          Послабление это стоит ВНЕ стандарта, и цена его - разомкнутый
					 *          обход - есть цена самого послабления, а не порок хода
					 *
					 * @param path путь до значения
					 * @return     перечень звеньев пути к детям значения
					 *
					 * \~english
					 * @brief Method of the enumeration of the tokens of the path to the children of a value
					 *
					 * @details Not THE NAMES of the children are given but THE TOKENS OF THE PATH to them: such
					 *          that `at(path + "/" + token)` inevitably leads to that very child. For a container of
					 *          pairs the token serves by the name of a pair, for a list of values - by the index.
					 *
					 * @details This course together with `at()` gives the FULL TRAVERSAL of a document, common to
					 *          all the codecs of the framework. The traversal is CLOSED — every issued link is fit
					 *          for `at()` — and this closedness holds on the escaping records of RFC 6901: a name
					 *          carrying a slash or a tilde is issued WRITTEN (`~1` and `~0`), and not as it stands.
					 *
					 * @note The enumeration NEVER answers with a refusal: a plain value, an empty container and a
					 *       path into nowhere answer an empty list ALIKE, while they differ by the course `has()`
					 *
					 * @warning This closedness holds on a document ANSWERING THE STANDARD: YAML 1.2 requires the
					 *          names of the pairs of a mapping to be unique, and the default of the parsing refuses
					 *          a repeat. Under the indulgence `duplicate_t::KEEP`, holding the repeats, the traversal
					 *          is UNCLOSED by nature: a name is issued as many times as there are pairs carrying it,
					 *          while the search by a name gives the first of them — the second is unreachable by
					 *          anything. Measured; a careless check does not catch it, for `has()` and `valid()` on
					 *          such a link answer with truth
					 *
					 * @warning It cannot be held by default: a bridge walking a codec by a tree would lose the pairs
					 *          silently. To mend it by changing the grammar of a link — issuing `name[1]` — is
					 *          refused deliberately: the link would cease to be a name, and the codec would have a
					 *          notation of its own for a path, which none of the others have. This indulgence stands
					 *          OUTSIDE the standard, and its price — an unclosed traversal — is the price of the
					 *          indulgence itself, and not a flaw of the course
					 *
					 * @param path path to the value
					 * @return     list of the tokens of the path to the children of the value
					 *
					 * \~
					 *
					 * @see Договор этот закреплён проверкою `CodecContract.EnumerationAndLookupFormAClosedTraversal`
					 *
					 */
					vector <string> keys(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества узлов дерева документа
					 *
					 * @return количество узлов дерева
					 *
					 * \~english
					 * @brief Method of the obtaining of the number of the nodes of the tree of a document
					 *
					 * @return number of the nodes of the tree
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества узлов дерева документа
					 *
					 * @details Величина эта зависит от УСТРОЙСТВА кодека, а не от разобранного
					 *          текста: узлы дерева YAML сжимаются правкою, у иных кодеков нет.
					 *          Оттого имя ей своё, а в общий договор кодеков рамки она не
					 *          входит вовсе - там `size()` означает детей корня у всех семи.
					 *
					 * @note Прежде этот счёт выдавала `size()`. Смена смысла согласована
					 *       с прочими кодеками 03.09.2026: под одним именем лежали три
					 *       разных величины - узлы арены у YAML, разделы у INI, таблицы у TOML
					 *
					 * @return количество узлов дерева документа
					 *
					 * \~english
					 * @brief Method of getting the count of the nodes of the tree of the document
					 *
					 * @details This quantity depends on the ARRANGEMENT of the codec, and not on the parsed text:
					 *          the nodes of a YAML tree are compacted by an editing, while other codecs do not.
					 *          Whereby it has a name of its own, and it does not enter the common contract of the
					 *          codecs of the framework at all — there `size()` means the children of the root in all
					 *          the seven.
					 *
					 * @note Formerly this count was issued by `size()`. The change of the meaning was agreed with
					 *       the other codecs on 2026-09-03: under one name lay three different quantities — the
					 *       nodes of the arena at YAML, the sections at INI, the tables at TOML
					 *
					 * @return count of the nodes of the tree of the document
					 *
					 * \~
					 */
					size_t nodes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева документа на пустоту
					 *
					 * @return признак пустоты дерева документа
					 *
					 * \~english
					 * @brief Method of the check of the tree of a document for the emptiness
					 *
					 * @return sign of the emptiness of the tree of the document
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора текста
					 *
					 * @return код ошибки разбора текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the error code of the parsing of a text
					 *
					 * @return error code of the parsing of a text
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения отказа в исходном тексте
					 *
					 * @return положение отказа разбора в исходном тексте
					 *
					 * \~english
					 * @brief Method of the obtaining of the position of a refusal in the source text
					 *
					 * @return position of the refusal of the parsing in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения отказа разбора
					 *
					 * @details Имя это общее у всех семи кодеков рамки. Прежде у YAML ход звался
					 *          `location()`, и потребитель, два кодека рядом читающий, разыскивал
					 *          его не тем именем.
					 *
					 * @note Прежнее имя оставлено посредником и НЕ снесено: снос задел бы
					 *       потребителей, а согласование имён того не требует
					 *
					 * @return положение отказа разбора в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the location of the failure of the parsing
					 *
					 * @details This name is common to all the seven codecs of the framework. Formerly at YAML the
					 *          course was named `location()`, and a consumer reading two codecs side by side sought
					 *          it by the wrong name.
					 *
					 * @note The former name is left as a mediator and is NOT removed: a removal would touch the
					 *       consumers, while the agreeing of the names does not ask for it
					 *
					 * @return location of the failure of the parsing in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса дерева документа
					 *
					 * \~english
					 * @brief Method of the reset of the tree of a document
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек записи, отвечающих состоявшемуся разбору
					 *
					 * @details Выводит настройки записи из того разбора, какой состоялся: схему
					 *          разрешения видов скалярных значений, запись директивы наречия да черты
					 *          начала и конца документа. Прочитанное этими настройками записывается так,
					 *          что читается обратно без потерь.
					 *
					 * @note Ход этот нужен зовущему, берущему `writer_t` сам: схема разбора
					 *       выводится из ТЕКСТА - директива `%YAML 1.1` переводит разбор на наречие
					 *       1.1, - и ход `settings()` её не отдаёт вовсе, выводя настройки заданные
					 *
					 * @return настройки записи текста документа
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing corresponding to the parsing that has taken place
					 *
					 * @details It deduces the settings of the writing from the very parsing which has taken place:
					 *          the schema of the resolution of the kinds of the scalar values, the notation of the
					 *          directive of the dialect and the markers of the start and of the end of the document.
					 *          What has been read is written by these settings such that it reads back without
					 *          losses.
					 *
					 * @note This course is needed by a caller taking a `writer_t` on its own: the schema of the
					 *       parsing is deduced from the TEXT — the directive `%YAML 1.1` turns the parsing to the
					 *       1.1 dialect — and the course `settings()` does not issue it at all, deducing the
					 *       settings given
					 *
					 * @return settings of the writing of a text of a document
					 *
					 * \~
					 */
					writer_t::settings_t writing() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек документа
					 *
					 * @param settings устанавливаемые настройки документа
					 * @return         признак принятия настроек документа
					 *
					 * \~english
					 * @brief Method of setting the settings of a document
					 *
					 * @param settings settings of a document being set
					 * @return         sign of the acceptance of the settings of a document
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
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
