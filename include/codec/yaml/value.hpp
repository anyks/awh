/**
 * @file value.hpp
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
 * @brief Заголовочный файл владеющего значения YAML — самостоятельный тип данных,
 *        хранящий дерево значений собственной памятью, собираемый из значений языка и
 *        пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of YAML — a standalone data type
 *        storing a tree of the values by its own memory, assembled from the values of the language and
 *        suitable for the passing outwards as an ordinary value
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_VALUE__
#define __AWH_CODEC_YAML_VALUE__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "document.hpp"

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
			 * @brief Владеющее значение YAML
			 *
			 * @details Тип этот стоит **над** документом, а не вместо него. Документ
			 * разбирает текст, удерживает его оформление и правится путями, отчего
			 * узлы его лежат плоским массивом, а ссылка на узел `Document::Value`
			 * владельцем не является и документ пережить не может
			 *
			 * @details Владеющее значение решает ровно ту задачу, какую ссылка решать не
			 * вправе: оно держит своё поддерево собственной памятью и потому
			 * складывается из значений языка, копируется, передаётся вглубь и **отдаётся
			 * наружу итогом метода**. Оформления исходного текста оно не удерживает:
			 * удержание принадлежит документу, и правка настроек через документ остаётся
			 * единственным путём сохранить чужой текст неприкосновенным
			 *
			 * @details Частности наречия YAML сохранены: у значения есть оформление
			 * записи, правило усечения переводов строк, построение вместилища, метка и
			 * якорь. Обёртка эта общий облик с прочими кодеками делит, а своеобразия
			 * наречия не теряет
			 *
			 * \~english
			 * @brief Owning value of YAML
			 * @details This type stands **above** the document rather than instead of it. The document
			 * parses a text, retains its formatting and is edited by paths, whereby
			 * its nodes lie in a flat array, while a reference to a node `Document::Value`
			 * is not an owner and cannot outlive the document
			 * @details The owning value solves exactly the task which a reference has no
			 * right to solve: it holds its subtree by its own memory and therefore
			 * is assembled from the values of the language, is copied, is passed inwards and **is given away
			 * outwards as the result of a method**. It does not retain the formatting of the source text:
			 * the retention belongs to the document, and the editing of the settings through the document remains
			 * the only way to keep a foreign text intact
			 * @details The particularities of the YAML dialect are preserved: a value has the formatting
			 * of the record, the rule of the chomping of the line breaks, the layout of the container, a tag and
			 * an anchor. This wrapper shares the common shape with the rest of the codecs
			 * while not losing the peculiarities of the dialect
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * @details Держится указателем, пустоту допускающим: значение владеющее
					 * заводится и числом, и строкою, и связку в такие построители не заведёшь.
					 * Журнал назначается извне вызовом setLogger(), и пока он не назначен, отказы
					 * выдаются одним лишь кодом
					 *
					 * \~english
					 * Object for working with logs
					 * @details It is held by a pointer that admits emptiness: an owning value is created
					 * both from a number and from a string, and the pair cannot be put into such builders.
					 * The log is assigned from the outside by a call of setLogger(), and until it is assigned,
					 * the refusals are given away by a code alone
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					// Вид хранимого значения
					kind_t _kind;
				private:
					// Вид числа, опознанный при установке значения
					type_t _type;
				private:
					/**
					 * \~russian
					 * Схема, какою опознан вид числа
					 *
					 * @details Схема хранится значением, а не берётся у документа: значение
					 * владеющее документ пережить обязано, и вид числа его переопределяться
					 * настройками чужого разбора не должен
					 *
					 * \~english
					 * Schema by which the kind of the number is recognized
					 * @details The schema is stored by the value rather than taken from the document: an owning
					 * value must outlive the document, and the kind of its number must not be redefined
					 * by the settings of a foreign parsing
					 *
					 * \~
					 */
					schema_t _schema;
				private:
					/**
					 * \~russian
					 * Число, разобранное из записи значения
					 *
					 * @details Хранится оно рядом с записью, а не вместо неё: запись `0x1F`
					 * обязана вернуться записью `0x1F`, а извлечение числа обязано выдать 31,
					 * и одно другому не замена
					 *
					 * \~english
					 * Number parsed from the record of the value
					 * @details It is stored alongside the record rather than instead of it: the record `0x1F`
					 * must return as the record `0x1F`, while the extraction of the number must give away 31,
					 * and one is not a replacement for the other
					 *
					 * \~
					 */
					numeric_t _number;
				private:
					// Оформление записи значения
					style_t _style;
				private:
					/**
					 * \~russian
					 * Правило усечения переводов строк блочного значения
					 *
					 * @details Умолчанием взято правило сохраняющее, а не отсекающее: у
					 * значения владеющего запись есть само содержимое, и правило это
					 * единственное, при каком содержимое возвращается тем же, сколько бы
					 * переводов строк ни стояло в конце
					 *
					 * @note Потребитель волен назначить иное, и назначает он его знаючи:
					 * правила `CLIP` и `STRIP` переводы строк в конце содержимого теряют
					 *
					 * \~english
					 * Rule of the chomping of the line breaks of a block value
					 * @details The keeping rather than the clipping rule is taken as the default: for an owning
					 * value the record is the content itself, and this rule is the only one under which
					 * the content returns the same, however many line breaks stand at the end
					 * @note The consumer is free to assign another one, and assigns it knowingly:
					 * the `CLIP` and `STRIP` rules lose the line breaks at the end of the content
					 *
					 * \~
					 */
					chomp_t _chomp;
				private:
					// Построение вместилища
					layout_t _layout;
				private:
					// Содержимое значения, записанное тем же видом, каким оно ложится в текст
					string _text;
				private:
					// Якорь значения, пустой - якоря нет
					string _anchor;
				private:
					// Метка значения, пустая - метки нет
					string _tag;
				private:
					/**
					 * \~russian
					 * Имена полей отображения
					 *
					 * @details Перечень этот наполняется лишь у отображения: у перечня
					 * значений имён нет вовсе, и хранить пустые строки ему незачем
					 *
					 * \~english
					 * Names of the fields of a mapping
					 * @details This list is filled only for a mapping: a sequence of the values
					 * has no names at all, and there is no point for it to store empty strings
					 *
					 * \~
					 */
					vector <string> _names;
				private:
					// Значения вместилища в порядке их следования
					vector <Value> _items;
				private:
					/**
					 * \~russian
					 * Указатель поиска пары по имени
					 *
					 * @details Поиск ведётся перебором имён, покуда пар меньше порога, и
					 * указателем далее. Перебор при тысячах пар обращает и сборку, и чтение в
					 * квадратичные: замерено 21.08.2026 - около трёх микросекунд на обращение
					 * при 2500 парах против двадцати при 20000
					 *
					 * @note Указатель заводится ЛЕНИВО и держится значением необязательным:
					 * узел мелкого вместилища не платит за него ни памятью, ни выделением, а
					 * таких узлов в дереве подавляющее большинство
					 *
					 * @note Указатель ведётся приращением, а не перестроением: перестроение на
					 * всякой правке вернуло бы ту же квадратичность, от какой он и заводится
					 *
					 * \~english
					 * Index of the search of a pair by a name
					 * @details The search is conducted by the enumeration of the names while there are fewer
					 * pairs than the threshold, and by the index further on
					 * @note The index is created LAZILY and is maintained by an increment
					 *
					 * \~
					 */
					mutable unique_ptr <unordered_map <string, size_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод разыскания пары по имени
					 *
					 * @param name имя разыскиваемой пары
					 * @return     номер пары, размер вместилища при отсутствии
					 *
					 * \~english
					 * @brief Method of the searching of a pair by a name
					 * @param name name of the pair being searched for
					 * @return number of the pair, size of the container at the absence
					 *
					 * \~
					 */
					size_t locate(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сноса указателя поиска
					 *
					 * @details Сносится указатель при всякой перестановке пар: удаление сдвигает
					 * номера всех пар после удалённой, и починка его обошлась бы дороже, чем
					 * заведение заново при первом же поиске
					 *
					 * \~english
					 * @brief Method of the demolition of the index of the search
					 *
					 * \~
					 */
					void unindex() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод опознания вида числа по тексту значения
					 *
					 * @details Опознание ведётся тем же порядком, каким его ведёт документ:
					 * сличается само число с пределами видов, а не запись его
					 *
					 * \~english
					 * @brief Method of the recognition of the kind of a number by the text of the value
					 * @details The recognition is conducted in the same order as the document conducts it:
					 * the number itself is compared with the limits of the kinds rather than its record
					 *
					 * \~
					 */
					void recognize() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Шаблонный метод извлечения числа затребованным видом
					 *
					 * @tparam T      затребованный вид числа
					 * @param  result переменная, куда помещается извлечённое значение
					 * @return        признак успешности извлечения
					 *
					 * \~english
					 * @brief Template method of the extraction of a number by the demanded kind
					 * @tparam T demanded kind of the number
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					template <typename T>
					bool extract(T & result) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод записи значения в поток записи
					 *
					 * @param writer поток записи, куда ложится значение
					 *
					 * \~english
					 * @brief Method of the writing of the value into a writing stream
					 * @param writer writing stream where the value is placed
					 *
					 * \~
					 */
					void compose(writer_t & writer) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод укладки владеющего значения в дерево документа
					 *
					 * @details Обход укладки вынесен из @c graft() ради целости переноса: тот
					 *          ведёт укладку на копии дерева и подменяет им дерево лишь по
					 *          успехе, а обход идёт вглубь рекурсией - копия на всяком шаге
					 *          стоила бы квадрата
					 *
					 * @param document дерево документа, куда укладывается значение
					 * @param path     путь, по какому укладывается значение
					 * @return         признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the laying of an owning value into a tree of the document
					 * @details The traversal of the laying is taken out of @c graft() for the sake of the
					 * integrity of the grafting: the latter conducts the laying on a copy of the tree
					 * @param document tree of the document the value is laid into
					 * @param path     path the value is laid by
					 * @return         sign of the success of the laying
					 *
					 * \~
					 */
					bool implant(Document & document, const string & path) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод снятия значения со ссылки на узел документа
					 *
					 * @param value ссылка на узел документа
					 *
					 * \~english
					 * @brief Method of the taking of a value from a reference to a node of a document
					 * @param value reference to the node of the document
					 *
					 * \~
					 */
					void absorb(const Document::value_t & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки определённости значения
					 *
					 * @return признак определённости значения
					 *
					 * \~english
					 * @brief Method of the check of the definiteness of the value
					 * @return sign of the definiteness of the value
					 *
					 * \~
					 */
					bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида значения
					 *
					 * @return вид хранимого значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the value
					 * @return kind of the stored value
					 *
					 * \~
					 */
					kind_t kind() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида хранения значения
					 *
					 * @return вид хранения значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the storage of the value
					 * @return kind of the storage of the value
					 *
					 * \~
					 */
					type_t type() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки вида хранения значения
					 *
					 * @param type сличаемый вид хранения, допускающий объединение видов
					 * @return     признак совпадения вида
					 *
					 * \~english
					 * @brief Method of the check of the kind of the storage of the value
					 * @param type kind of the storage being compared, allowing a union of the kinds
					 * @return sign of the coincidence of the kind
					 *
					 * \~
					 */
					bool is(const type_t type) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества значений вместилища
					 *
					 * @return количество значений вместилища, у значения простого - ноль
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the values of the container
					 * @return number of the values of the container, for a simple value — a zero
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
					 * @brief Method of the check of the container for the emptiness
					 * @return sign of the emptiness of the container
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки значения
					 *
					 * \~english
					 * @brief Method of the clearing of the value
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого значения
					 *
					 * @return содержимое значения тем видом, каким оно ложится в текст
					 *
					 * \~english
					 * @brief Method of the extraction of the content of the value
					 * @return content of the value in the same kind as it is placed into a text
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имени поля отображения по номеру
					 *
					 * @param index номер поля отображения
					 * @return      имя поля отображения, пустое - поля с таким номером нет
					 *
					 * \~english
					 * @brief Method of the extraction of the name of a field of a mapping by an index
					 * @param index index of the field of the mapping
					 * @return name of the field of the mapping, an empty one — there is no field with such index
					 *
					 * \~
					 */
					const string & key(const size_t index) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения оформления записи значения
					 *
					 * @return оформление записи значения
					 *
					 * \~english
					 * @brief Method of the extraction of the formatting of the record of the value
					 * @return formatting of the record of the value
					 *
					 * \~
					 */
					style_t style() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки оформления записи значения
					 *
					 * @param style устанавливаемое оформление записи
					 *
					 * \~english
					 * @brief Method of the setting of the formatting of the record of the value
					 * @param style formatting of the record being set
					 *
					 * \~
					 */
					void style(const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения правила усечения переводов строк
					 *
					 * @return правило усечения переводов строк блочного значения
					 *
					 * \~english
					 * @brief Method of the extraction of the rule of the chomping of the line breaks
					 * @return rule of the chomping of the line breaks of a block value
					 *
					 * \~
					 */
					chomp_t chomp() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки правила усечения переводов строк
					 *
					 * @param chomp устанавливаемое правило усечения переводов строк
					 *
					 * \~english
					 * @brief Method of the setting of the rule of the chomping of the line breaks
					 * @param chomp rule of the chomping of the line breaks being set
					 *
					 * \~
					 */
					void chomp(const chomp_t chomp) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения построения вместилища
					 *
					 * @return построение вместилища
					 *
					 * \~english
					 * @brief Method of the extraction of the layout of the container
					 * @return layout of the container
					 *
					 * \~
					 */
					layout_t layout() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки построения вместилища
					 *
					 * @param layout устанавливаемое построение вместилища
					 *
					 * \~english
					 * @brief Method of the setting of the layout of the container
					 * @param layout layout of the container being set
					 *
					 * \~
					 */
					void layout(const layout_t layout) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения якоря значения
					 *
					 * @return якорь значения, пустой - якоря нет
					 *
					 * \~english
					 * @brief Method of the extraction of the anchor of the value
					 * @return anchor of the value, an empty one — there is no anchor
					 *
					 * \~
					 */
					const string & anchor() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки якоря значения
					 *
					 * @param anchor устанавливаемый якорь значения
					 *
					 * \~english
					 * @brief Method of the setting of the anchor of the value
					 * @param anchor anchor of the value being set
					 *
					 * \~
					 */
					void anchor(const string & anchor) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения метки значения
					 *
					 * @return метка значения, пустая - метки нет
					 *
					 * \~english
					 * @brief Method of the extraction of the tag of the value
					 * @return tag of the value, an empty one — there is no tag
					 *
					 * \~
					 */
					const string & tag() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки метки значения
					 *
					 * @param tag устанавливаемая метка значения
					 *
					 * \~english
					 * @brief Method of the setting of the tag of the value
					 * @param tag tag of the value being set
					 *
					 * \~
					 */
					void tag(const string & tag) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения значения мусорного
					 *
					 * @details Значение это принимает на себя запись при неудачном обращении
					 * изменяемом: выдать ссылку метод обязан, а заводить значение по неверному
					 * пути не вправе. Записанное в него пропадает при следующем же неудачном
					 * обращении, и полагаться на него нельзя
					 *
					 * @note Значение это своё у всякого потока: одно на приложение оно обратило
					 *       бы отказ обращения в состязание за общую память
					 *
					 * @return значение мусорное
					 *
					 * \~english
					 * @brief Method of the extraction of a scrap value
					 * @details This value takes upon itself a writing at an unsuccessful mutable
					 * access: a method is obliged to give away a reference but has no right to create a value
					 * by an incorrect path. What is written into it disappears at the very next unsuccessful
					 * access, and one must not rely upon it
					 * @note This value is its own for every thread: one for the whole application it would turn
					 *       a refusal of an access into a contention for a common memory
					 * @return scrap value
					 *
					 * \~
					 */
					static Value & scrap() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения предела роста вместилища по номеру
					 *
					 * @details Обращение по номеру растит вместилище значениями
					 * неопределёнными до затребованного номера, и номер, пришедший извне -
					 * из настроек либо из запроса, - обращается требованием памяти по нему.
					 * Предел этот рост и стережёт: обращение за пределом заводит вместо
					 * значения значение мусорное, а вместилище не растит вовсе
					 *
					 * @note Предел ограждает один лишь рост обращением по номеру. Вместилище,
					 *       собранное разбором либо добавлением значений в конец, ему не
					 *       подвластно: длину разбираемого текста задаёт пользователь рамки,
					 *       и решать за него, сколько значений ему дозволено, рамка не вправе
					 *
					 * @return предел роста вместилища, нуль - предела нет
					 *
					 * \~english
					 * @brief Method of the extraction of the limit of the growth of a container by an index
					 * @details An access by an index grows a container by undefined values up to the requested
					 * index, and an index that came from outside — from the settings or from a request — turns
					 * into a demand of the memory by it. This limit guards that growth: an access beyond the limit
					 * creates a scrap value instead of a value and does not grow the container at all
					 * @note The limit guards only the growth by an access by an index. A container built by a parsing
					 *       or by an addition of values at the end is not subject to it: the length of a text being
					 *       parsed is set by the user of the framework, and the framework has no right to decide
					 *       for them how many values they are allowed
					 * @return limit of the growth of a container, zero — there is no limit
					 *
					 * \~
					 */
					static size_t limit() noexcept;
					/**
					 * \~russian
					 * @brief Метод установки предела роста вместилища по номеру
					 *
					 * @details Предел этот пользователем рамки и ставится: сколько памяти есть у
					 * приложения, ведомо ему одному. Значение по умолчанию - 65536 значений
					 *
					 * @param value устанавливаемый предел, нуль снимает предел вовсе
					 *
					 * \~english
					 * @brief Method of the setting of the limit of the growth of a container by an index
					 * @details This limit is set by the user of the framework: how much memory an application has
					 * is known to it alone. The default value is 65536 values
					 * @param value limit being set, zero removes the limit altogether
					 *
					 * \~
					 */
					static void limit(const size_t value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия поля отображения с указанным именем
					 *
					 * @param name разыскиваемое имя поля отображения
					 * @return     признак наличия поля отображения
					 *
					 * \~english
					 * @brief Method of the check of the presence of a field of a mapping with the indicated name
					 * @param name name of the field of the mapping being searched for
					 * @return sign of the presence of the field of the mapping
					 *
					 * \~
					 */
					bool contains(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разыскания номера пары по имени её
					 *
					 * @details Розыск ведётся указателем имён, когда тот заведён: перебор
					 * обратил бы сборку вместилища вызовами в квадратичную
					 *
					 * @note Выдаётся размер вместилища, когда пары с таким именем нет: способ
					 *       этот тот же, каким отвечает `npos` у строки
					 *
					 * @param name имя разыскиваемой пары
					 * @return     номер пары, размер вместилища при отсутствии
					 *
					 * \~english
					 * @brief Method of the searching of the number of a pair by its name
					 * @details The search is conducted by the index of the names when it is established: an enumeration
					 * would turn the assembling of a container by the calls into a quadratic one
					 * @note The size of the container is issued when there is no pair with such a name
					 * @param name name of the pair being searched for
					 * @return     number of the pair, size of the container at the absence
					 *
					 * \~
					 */
					size_t search(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой:
					 * `/response/users/0/id`. Обращение к отсутствующему пути ничего не
					 * заводит и отдаёт значение неопределённое
					 *
					 * @param path путь к разыскиваемому значению
					 * @return     ссылка на разысканное значение
					 *
					 * \~english
					 * @brief Method of the access to a value by a path
					 * @details The path is written by the parts separated by a slash:
					 * `/response/users/0/id`. An access to an absent path creates nothing
					 * and gives away an undefined value
					 * @param path path to the value being searched for
					 * @return reference to the found value
					 *
					 * \~
					 */
					const Value & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению по пути с заведением недостающего
					 *
					 * @details Недостающие вместилища пути заводятся отображениями, а
					 * часть пути числовая заводит перечень значений. Значение простое,
					 * встреченное на пути вместо вместилища, перерождается вместилищем
					 *
					 * @param path путь к разыскиваемому значению
					 * @return     ссылка на разысканное либо заведённое значение
					 *
					 * \~english
					 * @brief Method of the access to a value by a path with the creation of the missing ones
					 * @details The missing containers of the path are created as mappings, while
					 * a numeric part of the path creates a sequence of the values. A simple value
					 * met on the path instead of a container is reborn as a container
					 * @param path path to the value being searched for
					 * @return reference to the found or created value
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к полю отображения по имени
					 *
					 * @param name имя поля отображения
					 * @return     ссылка на значение поля отображения
					 *
					 * \~english
					 * @brief Method of the access to a field of a mapping by a name
					 * @param name name of the field of the mapping
					 * @return reference to the value of the field of the mapping
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к полю отображения по имени с заведением недостающего
					 *
					 * @details Обращение к отсутствующему имени заводит поле значением
					 * неопределённым, ровно как это делает `nlohmann::json`
					 *
					 * @param name имя поля отображения
					 * @return     ссылка на значение поля отображения
					 *
					 * \~english
					 * @brief Method of the access to a field of a mapping by a name with the creation of a missing one
					 * @details An access to an absent name creates the field with an undefined
					 * value, exactly as `nlohmann::json` does it
					 * @param name name of the field of the mapping
					 * @return reference to the value of the field of the mapping
					 *
					 * \~
					 */
					Value & operator [] (const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению вместилища по номеру
					 *
					 * @param index номер значения во вместилище
					 * @return      ссылка на значение вместилища
					 *
					 * \~english
					 * @brief Method of the access to a value of a container by an index
					 * @param index index of the value in the container
					 * @return reference to the value of the container
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению вместилища по номеру с заведением недостающего
					 *
					 * @details Обращение за границу перечня значений растит его до
					 * затребованного номера значениями неопределёнными
					 *
					 * @param index номер значения во вместилище
					 * @return      ссылка на значение вместилища
					 *
					 * \~english
					 * @brief Method of the access to a value of a container by an index with the creation of a missing one
					 * @details An access beyond the boundary of a sequence of the values grows it up to
					 * the demanded index by the undefined values
					 * @param index index of the value in the container
					 * @return reference to the value of the container
					 *
					 * \~
					 */
					Value & operator [] (const size_t index) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод добавления значения в конец перечня значений
					 *
					 * @details Значение неопределённое перерождается перечнем, а значение
					 * простое перерождению не подлежит и добавление отвечает отказом
					 *
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a value to the end of a sequence of the values
					 * @details An undefined value is reborn as a sequence, while a simple value
					 * is not subject to the rebirth and the addition responds with a refusal
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки поля отображения
					 *
					 * @details Поле с уже занятым именем перезаписывается на своём месте, а
					 * порядок полей отображения при том сохраняется
					 *
					 * @param name  имя поля отображения
					 * @param value устанавливаемое значение поля
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a field of a mapping
					 * @details A field with an already occupied name is overwritten in its place, while
					 * the order of the fields of the mapping is thereby preserved
					 * @param name name of the field of the mapping
					 * @param value value of the field being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления поля отображения рядом с одноимённым
					 *
					 * @details Поле кладётся рядом, а не поверх: разбор с настройкой
					 * `duplicate_t::KEEP` удерживает все вхождения повторяющегося имени, и
					 * воспроизвести такое отображение установкой поля нельзя - она перезаписала
					 * бы одноимённое на его месте
					 *
					 * @note Розыска по имени метод не ведёт вовсе: имя кладётся сразу за
					 *       последним полем отображения
					 *
					 * @param name  имя поля отображения
					 * @param value добавляемое значение поля
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a field of a mapping next to a field of the same name
					 * @details The field is laid next to rather than over: the parsing with the setting
					 * `duplicate_t::KEEP` retains all the occurrences of a repeating name
					 * @param name name of the field of the mapping
					 * @param value value of the field being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool append(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия поля отображения по имени
					 *
					 * @param name имя снимаемого поля отображения
					 * @return     признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a field of a mapping by a name
					 * @param name name of the field of the mapping being removed
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия значения вместилища по номеру
					 *
					 * @param index номер снимаемого значения вместилища
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a value of a container by an index
					 * @param index index of the value of the container being removed
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const size_t index) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения логического значения
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a logical value
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(bool & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения числа
					 *
					 * @details Отказом извлечение завершается лишь тогда, когда значение
					 * числом не является вовсе. Вид хранения извлечению не указ, а сужение
					 * выполняется обычным приведением языка - ровно так, как это делает
					 * ссылка на узел документа
					 *
					 * @details Дробное, извлекаемое целым видом, округляется по правилам
					 * математики с уводом половины от нуля: `1.5` выдаётся двойкой, а `-1.5` -
					 * минус двойкой. Усечения к нулю здесь нет
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a number
					 * @details The extraction ends with a refusal only when the value
					 * is not a number at all. The kind of the storage is not a rule for the extraction, while the narrowing
					 * is performed by the ordinary casting of the language — exactly as a reference
					 * to a node of a document does it
					 * @details A fractional number extracted as an integer kind is rounded by the rules
					 * of mathematics with a half taken away from zero: `1.5` is issued as a two, while `-1.5` —
					 * as a minus two. There is no truncation towards zero here
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(float & result) const noexcept;
					/**
					 * @copydoc awh::codec::yaml::Value::value(int8_t &) const
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строкового значения
					 *
					 * @details Извлечение выдаёт ЗАПИСЬ всякого скалярного значения, каков бы
					 *          ни был разрешённый вид его: число `12` выдаётся записью `12`,
					 *          признак истины - записью `true`, пустое значение - записью `~`
					 *          либо тою, какою оно записано. Вместилище - отображение да
					 *          перечень - отвечает отказом
					 *
					 * @note Правило это у пяти кодеков рамки РАЗНОЕ, и разное намеренно - оно
					 *       следует из устройства наречия, а не выбрано наудачу:
					 *       @li у XML и INI видов нет вовсе, всякое значение там текст, и
					 *           отказывать по виду попросту нечему;
					 *       @li у JSON и TOML виды суть хранение, и извлечение строкою служит
					 *           им проверкою вида ровно так же, как извлечение числом;
					 *       @li у YAML вид есть РАЗРЕШЕНИЕ над записанным скаляром, а не
					 *           хранение его. Запись первична - ею держатся ограда, блочное
					 *           построение и правило усечения переводов строк, - и выдать её
					 *           обязано всякое скалярное значение. Отказ по разрешённому виду
					 *           отнял бы у потребителя доступ к тому, что в файле написано.
					 *
					 * @note Общей проверки на все пять кодеков тут быть не должно: она
					 *       закрепила бы единообразие там, где его нет по устройству
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a string value
					 * @details The extraction issues the RECORD of any scalar value, whatever its resolved
					 * kind: the number `12` is issued by the record `12`, the sign of truth by the record
					 * `true`. A container — a mapping and a list — answers with a refusal
					 * @note This rule differs among the five codecs of the framework deliberately: it follows
					 * from the arrangement of the dialect. XML and INI have no kinds at all; for JSON and TOML
					 * the kinds are the storage; for YAML a kind is a RESOLUTION over the written scalar, and
					 * the record is primary
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста YAML во владеющее значение
					 *
					 * @details Разбор ведётся тем же документом, каким он ведётся всегда:
					 * обёртка лишь снимает с него дерево собственной памятью. Оформление
					 * исходного текста при снятии не удерживается
					 *
					 * @param text разбираемый текст YAML
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a YAML text into an owning value
					 * @details The parsing is conducted by the same document by which it is always conducted:
					 * the wrapper only takes the tree off it by its own memory. The formatting
					 * of the source text is not retained at the taking
					 * @param text YAML text being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста YAML указанными настройками
					 *
					 * @details Настройки нужны прежде всего схемою: запись `0777` наречием 1.1
					 * читается восьмеричной, давая 511, а схемою ядровой - десятичной, давая
					 * 777. Значение, снятое с документа одной схемы, обратно читается той же
					 *
					 * @note Частность эта наречию YAML своя: у кодеков, схем не имеющих,
					 * разбор настроек не требует вовсе
					 *
					 * @param text     разбираемый текст YAML
					 * @param settings настройки разбора текста
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a YAML text with the indicated settings
					 * @details The settings are needed first of all for the schema: the record `0777` is read
					 * by the 1.1 dialect as an octal one, giving 511, while by the core schema — as a decimal one, giving
					 * 777. A value taken from a document of one schema is read back by the same one
					 * @note This particularity is peculiar to the YAML dialect: for the codecs having no schemas,
					 * the parsing does not require the settings at all
					 * @param text YAML text being parsed
					 * @param settings settings of the parsing of the text
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const Document::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста YAML из файла
					 *
					 * @param filename адрес разбираемого файла
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a YAML text from a file
					 * @param filename address of the file being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста YAML из файла указанными настройками
					 *
					 * @param filename адрес разбираемого файла
					 * @param settings настройки разбора текста
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a YAML text from a file with the indicated settings
					 * @param filename address of the file being parsed
					 * @param settings settings of the parsing of the text
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename, const Document::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст YAML
					 *
					 * @return текст YAML
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a YAML text
					 * @return YAML text
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст YAML с указанными настройками
					 *
					 * @param settings настройки записи текста
					 * @return         текст YAML
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a YAML text with the indicated settings
					 * @param settings settings of the writing of the text
					 * @return YAML text
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения в файл
					 *
					 * @param filename адрес записываемого файла
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a file
					 * @param filename address of the file being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод переноса владеющего значения в дерево документа
					 *
					 * @details Перенос ведётся путём: вместилище объявляется у места пустым, а
					 * дети его переносятся по пути внутрь него. Путь ведётся от корня первого
					 * документа, части его делятся косою чертой, а часть внутри перечня есть
					 * номер значения
					 *
					 * @note Имя пары, косую черту несущее, переносу не подлежит и отвечает
					 *       отказом: путь делится ею, и имя такое указывало бы на узел чужой
					 *
					 * @note Числа, отметки времени и двоичное содержимое переносятся дословною
					 *       записью своею: ограда обратила бы их в строки
					 *
					 * @note Записью же они и прочтутся обратно, а чем прочтутся - решает схема
					 *       дерева, куда перенос ведётся: запись `0777` наречием 1.1 есть число
					 *       восьмеричное, а схемою ядровой - десятичное. Перенос в дерево иной
					 *       схемы выдаёт значение иное по праву, а не по дефекту
					 *
					 * @note Имя пары, вторично встреченное, отвечает отказом: отображение вправе
					 *       нести имена повторные, а путь второе вхождение выразить не может -
					 *       он указывает на первое всегда
					 *
					 * @param document дерево документа, куда переносится значение
					 * @param path     путь к месту переноса
					 * @return         признак успешности переноса
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into the tree of a document
					 * @details The grafting is conducted by a path: a container is declared empty at the place, while
					 * its children are grafted by a path inside it. The path is led from the root of the first
					 * document, its parts are divided by a slash, and a part inside a sequence is
					 * the number of a value
					 * @note A name of a pair carrying a slash is not subject to the grafting and answers
					 *       with a refusal: the path is divided by it, and such a name would point at an alien node
					 * @note The numbers, the timestamps and the binary content are grafted by their verbatim
					 *       record: a quoting would turn them into the strings
					 * @note By the record they are read back as well, and what they are read as is decided by the schema
					 *       of the tree whereinto the grafting is conducted: the record `0777` by the dialect 1.1 is an octal
					 *       number while by the core schema — a decimal one
					 * @note A name of a pair met a second time answers with a refusal: a mapping is entitled
					 *       to carry repeated names while a path cannot express the second occurrence
					 * @param document tree of the document whereinto the value is grafted
					 * @param path     path to the place of the grafting
					 * @return         sign of the success of the grafting
					 *
					 * \~
					 */
					bool graft(Document & document, const string & path = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сличения значений
					 *
					 * @details Сличаются вид и содержимое дерева, а оформление записи,
					 * якорь и метка сличению не подлежат: значения эти одинаковы по сути
					 * своей и записаны лишь по-разному
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Method of the comparison of the values
					 * @details The kind and the content of the tree are compared, while the formatting of the record,
					 * the anchor and the tag are not subject to the comparison: such values are identical in their essence
					 * and are merely written differently
					 * @param value value being compared
					 * @return sign of the coincidence of the values
					 *
					 * \~
					 */
					bool operator == (const Value & value) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сличения значений на несовпадение
					 *
					 * @param value сличаемое значение
					 * @return      признак несовпадения значений
					 *
					 * \~english
					 * @brief Method of the comparison of the values for a mismatch
					 * @param value value being compared
					 * @return sign of the mismatch of the values
					 *
					 * \~
					 */
					bool operator != (const Value & value) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор присваивания копированием
					 *
					 * @param value присваиваемое значение
					 * @return      ссылка на текущее значение
					 *
					 * \~english
					 * @brief Copy assignment operator
					 * @param value value being assigned
					 * @return reference to the current value
					 *
					 * \~
					 */
					Value & operator = (const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Оператор присваивания переносом
					 *
					 * @param value переносимое значение
					 * @return      ссылка на текущее значение
					 *
					 * \~english
					 * @brief Move assignment operator
					 * @param value value being moved
					 * @return reference to the current value
					 *
					 * \~
					 */
					Value & operator = (Value && value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки объекта для работы с логами
					 *
					 * @param log объект для работы с логами
					 *
					 * @details Журнал перенимается всяким разбором и всякою записью, значением
					 * заведёнными, и копией значения тоже: назначить его довольно единожды
					 *
					 * \~english
					 * @brief Method of setting the logging object
					 * @param log object for working with logs
					 * @details The log is taken over by every parsing and every writing created by the value,
					 * and by a copy of the value too: it is enough to assign it once
					 *
					 * \~
					 */
					void setLogger(const log_t * log) noexcept;
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
					Value() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор вместилища указанного вида
					 *
					 * @param kind вид заводимого значения
					 *
					 * \~english
					 * @brief Constructor of a container of the indicated kind
					 * @param kind kind of the value being created
					 *
					 * \~
					 */
					explicit Value(const kind_t kind) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор логического значения
					 *
					 * @param value заводимое значение
					 *
					 * \~english
					 * @brief Constructor of a logical value
					 * @param value value being created
					 *
					 * \~
					 */
					Value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор целого значения со знаком
					 *
					 * @param value заводимое значение
					 *
					 * \~english
					 * @brief Constructor of a signed integer value
					 * @param value value being created
					 *
					 * \~
					 */
					Value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор целого значения без знака
					 *
					 * @param value заводимое значение
					 *
					 * \~english
					 * @brief Constructor of an unsigned integer value
					 * @param value value being created
					 *
					 * \~
					 */
					Value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор дробного значения
					 *
					 * @param value заводимое значение
					 *
					 * \~english
					 * @brief Constructor of a fractional value
					 * @param value value being created
					 *
					 * \~
					 */
					Value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор строкового значения
					 *
					 * @param value заводимое значение
					 * @param style оформление записи значения
					 *
					 * \~english
					 * @brief Constructor of a string value
					 * @param value value being created
					 * @param style formatting of the record of the value
					 *
					 * \~
					 */
					Value(const string & value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор строкового значения
					 *
					 * @param value заводимое значение
					 * @param style оформление записи значения
					 *
					 * \~english
					 * @brief Constructor of a string value
					 * @param value value being created
					 * @param style formatting of the record of the value
					 *
					 * \~
					 */
					Value(const char * value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор снятия значения со ссылки на узел документа
					 *
					 * @details Конструктор этот и есть мост от разбора к владению: ссылка
					 * документ пережить не может, а снятое ею значение - может
					 *
					 * @param value ссылка на узел документа
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a reference to a node of a document
					 * @details This constructor is the very bridge from the parsing to the ownership: a reference
					 * cannot outlive the document, while a value taken by it — can
					 * @param value reference to the node of the document
					 *
					 * \~
					 */
					explicit Value(const Document::value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор копирования
					 *
					 * @param value копируемое значение
					 *
					 * \~english
					 * @brief Copy constructor
					 * @param value value being copied
					 *
					 * \~
					 */
					Value(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор переноса
					 *
					 * @param value переносимое значение
					 *
					 * \~english
					 * @brief Move constructor
					 * @param value value being moved
					 *
					 * \~
					 */
					Value(Value && value) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Value() noexcept {}
			} value_t;
			/**
			 * \~russian
			 * @brief Потоковая сборка владеющего значения
			 *
			 * @details Сборка эта есть второй способ построить дерево, стоящий рядом с
			 * путями, а не вместо них. Потребитель, поля подряд заносящий, вложенностей не
			 * порождает и путями не пользуется вовсе: розыск по пути на каждое поле стоит
			 * прохода дерева, а сборке достаточно знать, где она стоит сейчас
			 *
			 * @details Договор её слово в слово повторяет договор потока записи: открыть
			 * вместилище, назвать поле, записать значение, закрыть. Тем «пишу текстом» и
			 * «строю дерево» отличаются одной буквой, и переход с одного на другое не
			 * требует переучиваться
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value
			 * @details This assembly is the second way to build a tree, standing alongside
			 * the paths rather than instead of them. A consumer entering the fields in a row does not
			 * generate the nestings and does not use the paths at all: a search by a path for every field costs
			 * a traversal of the tree, while the assembly only needs to know where it stands now
			 * @details Its contract repeats the contract of the writing stream word for word: open
			 * a container, name a field, write a value, close. Thereby "I write a text" and
			 * "I build a tree" differ by one letter, and the transition from one to the other does not
			 * require relearning
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Builder {
				private:
					// Собираемое значение
					Value _root;
				private:
					/**
					 * \~russian
					 * Путь к вместилищу, сборкой открытому
					 *
					 * @details Путь хранится номерами, а не указателями: значения вместилища
					 * лежат в перечне, и всякое добавление переселяет его в памяти, отчего
					 * указатель на открытое вместилище стал бы висячим на первом же поле
					 *
					 * \~english
					 * Path to the container opened by the assembly
					 * @details The path is stored by the indices rather than by the pointers: the values of a container
					 * lie in a list, and every addition relocates it in the memory, whereby
					 * a pointer to an opened container would become dangling at the very first field
					 *
					 * \~
					 */
					vector <size_t> _path;
				private:
					// Имя поля отображения, значения ожидающее
					string _key;
				private:
					// Признак того, что имя поля отображения назначено
					bool _keyed;
				private:
					/**
					 * Признак того, что поле кладётся рядом с одноимённым
					 *
					 * @note Признак принадлежит имени, а не сборке: всякое назначение имени
					 *       обычным способом его снимает
					 */
					bool _appended;
				private:
					/**
					 * \~russian
					 * Признак того, что сборка завершена
					 *
					 * @details Признак этот отличает вместилище корневое открытое от
					 * закрытого: путь номеров у обоих пуст, и без признака закрытие корня
					 * оставалось бы незамеченным
					 *
					 * \~english
					 * Sign that the assembly is finished
					 * @details This sign distinguishes an opened root container from
					 * a closed one: the path of the indices of both is empty, and without the sign the closing of the root
					 * would remain unnoticed
					 *
					 * \~
					 */
					bool _done;
				private:
					// Якорь, значению предпосылаемый
					string _anchor;
				private:
					// Метка, значению предпосылаемая
					string _tag;
				private:
					/**
					 * \~russian
					 * @brief Метод получения вместилища, сборкой открытого
					 *
					 * @return ссылка на открытое вместилище
					 *
					 * \~english
					 * @brief Method of the obtaining of the container opened by the assembly
					 * @return reference to the opened container
					 *
					 * \~
					 */
					Value & opened() noexcept;
					/**
					 * \~russian
					 * @brief Метод занесения собранного значения во вместилище
					 *
					 * @param value заносимое значение
					 * @return      номер занесённого значения во вместилище
					 *
					 * \~english
					 * @brief Method of the entering of an assembled value into the container
					 * @param value value being entered
					 * @return index of the entered value in the container
					 *
					 * \~
					 */
					size_t deposit(Value && value) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия вместилища указанного вида
					 *
					 * @param value открываемое вместилище
					 * @return      признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a container of the indicated kind
					 * @param value container being opened
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool expand(Value && value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия отображения пар
					 *
					 * @param layout построение открываемого отображения
					 * @return       признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a mapping
					 * @param layout layout of the mapping being opened
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool mapping(const layout_t layout = layout_t::BLOCK) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений
					 *
					 * @param layout построение открываемого перечня
					 * @return       признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a sequence of the values
					 * @param layout layout of the sequence being opened
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool sequence(const layout_t layout = layout_t::BLOCK) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого вместилища
					 *
					 * @return признак успешности закрытия
					 *
					 * \~english
					 * @brief Method of the closing of an opened container
					 * @return sign of the success of the closing
					 *
					 * \~
					 */
					bool close() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод назначения имени поля отображения
					 *
					 * @details Имя стоит до значения, а не при нём: тем сборка и повторяет
					 * поток записи, где имя и значение суть два разных действия
					 *
					 * @param name имя назначаемого поля отображения
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a field of a mapping
					 * @details The name stands before the value rather than with it: thereby the assembly repeats
					 * the writing stream, where the name and the value are two different actions
					 * @param name name of the field of the mapping being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения имени поля, рядом с одноимённым кладомого
					 *
					 * @details Действует как назначение имени, с одной разницей: значение, следом
					 * записанное, кладётся рядом с полем того же имени, а не поверх него. Тем
					 * сборка повторяет отображение, разбором с настройкой `duplicate_t::KEEP`
					 * собранное
					 *
					 * @note Признак добавления принадлежит имени, а не сборке: всякое назначение
					 *       имени обычным способом его снимает. Иначе одно добавление сделало бы
					 *       добавлениями все поля до конца сборки
					 *
					 * @param name имя назначаемого поля отображения
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a field laid next to a field of the same name
					 * @details It acts as the assignment of a name, with one difference: the value written next
					 * is laid next to a field of the same name rather than over it
					 * @param name name of the field of the mapping being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool append(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения якоря, значению предпосылаемого
					 *
					 * @param name имя назначаемого якоря
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of an anchor prefixed to a value
					 * @param name name of the anchor being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool anchor(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения метки, значению предпосылаемой
					 *
					 * @param name имя назначаемой метки
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of a tag prefixed to a value
					 * @param name name of the tag being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool tag(const string & name) noexcept;
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
					 * @brief Метод записи готового значения
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a ready value
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи логического значения
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a logical value
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа со знаком
					 *
					 * @details Число заносится видом своим, а не подстановкою записи: сборка,
					 * запись собирающая, заставила бы потребителя разбирать её обратно
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a signed integer number
					 * @details The number is entered by its own kind rather than by a substitution of a record: an assembly
					 * collecting a record would force the consumer to parse it back
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const int64_t value) noexcept;
					/**
					 * @copydoc awh::codec::yaml::Builder::value(const int64_t)
					 */
					bool value(const uint64_t value) noexcept;
					/**
					 * @copydoc awh::codec::yaml::Builder::value(const int64_t)
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value записываемое значение
					 * @param style оформление записи значения
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value value being written
					 * @param style formatting of the record of the value
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * @copydoc awh::codec::yaml::Builder::value(const string &, const style_t)
					 */
					bool value(const char * value, const style_t style = style_t::PLAIN) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения глубины открытых вместилищ
					 *
					 * @return количество вместилищ, сборкой открытых и не закрытых
					 *
					 * \~english
					 * @brief Method of the extraction of the depth of the opened containers
					 * @return number of the containers opened by the assembly and not closed
					 *
					 * \~
					 */
					size_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса сборки
					 *
					 * \~english
					 * @brief Method of the reset of the assembly
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия собранного значения
					 *
					 * @details Вместилища, открытые и не закрытые, закрываются изъятием сами:
					 * сборка, всё занёсшая, забыть о закрытии вправе, а недостроенное дерево
					 * ей выдавать незачем
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the withdrawal of the assembled value
					 * @details The containers opened and not closed are closed by the withdrawal itself:
					 * an assembly having entered everything has the right to forget about the closing, while there is no point
					 * in giving away an unfinished tree to it
					 * @return assembled value
					 *
					 * \~
					 */
					Value finish() noexcept;
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
					Builder() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Builder() noexcept {}
			} builder_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_VALUE__
