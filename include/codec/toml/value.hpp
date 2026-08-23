/**
 * @file value.hpp
 * @date 2026-08-20
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
 * @brief Заголовочный файл владеющего значения TOML — самостоятельный тип данных,
 *        хранящий дерево значений собственной памятью, собираемый из значений языка и
 *        пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of TOML — a standalone data type
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
#ifndef __AWH_CODEC_TOML_VALUE__
#define __AWH_CODEC_TOML_VALUE__

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
		 * @brief Пространство имён контейнера TOML
		 *
		 * \~english
		 * @brief TOML container namespace
		 *
		 * \~
		 */
		namespace toml {
			/**
			 * \~russian
			 * @brief Владеющее значение TOML
			 *
			 * @details Тип этот стоит **над** деревом настроек, а не вместо него. Дерево
			 * разбирает текст, удерживает оформление его и правится по путям, причём узлы
			 * его лежат плоским перечнем, а взгляд на узел владельцем не является и дерево
			 * пережить не может
			 *
			 * @details Владеющее значение решает ровно ту задачу, какую взгляду решать не
			 * дано: оно держит своё поддерево собственной памятью и оттого собирается из
			 * значений языка, копируется, передаётся внутрь и **отдаётся наружу итогом
			 * метода**. Оформления исходного текста оно не удерживает: удержание есть дело
			 * дерева, и правка чужого настроечного файла через дерево остаётся
			 * единственным способом сохранить чужой текст целым
			 *
			 * @details Своеобразия наречия TOML сохранены: значение несёт запись строки,
			 * систему счисления записи целого числа, признак записи перечня несколькими
			 * строками, а вид отметки времени различает четыре её разновидности. Обёртка
			 * эта общий облик с прочими кодеками делит, а своеобразия наречия не теряет
			 *
			 * @note Тип значения и вид хранимого здесь одно и то же поле, тогда как у
			 *       кодека YAML их два. Расхождение это устройством наречия: у YAML запись
			 *       значения и вид его различаются - `0x1F` есть запись, а число тридцать
			 *       один есть вид, - тогда как TOML вид записывает при разборе однозначно
			 *
			 * \~english
			 * @brief Owning value of TOML
			 * @details This type stands **above** the settings tree rather than instead of it. The tree
			 * parses a text, retains its formatting and is edited by paths, whereby its nodes
			 * lie in a flat list, while a view onto a node is not an owner and cannot outlive
			 * the tree
			 * @details The owning value solves exactly the task which a view has no right to
			 * solve: it holds its subtree by its own memory and therefore is assembled from the values
			 * of the language, is copied, is passed inwards and **is given away outwards as the result of a
			 * method**. It does not retain the formatting of the source text: the retention is the business of
			 * the tree, and the editing of a foreign settings file through the tree remains the only way
			 * to keep a foreign text intact
			 * @details The particularities of the TOML dialect are preserved: a value carries the notation of a
			 * string, the radix of the record of an integer, the flag of the writing of an array in several
			 * lines, while the kind of a timestamp distinguishes its four varieties. This wrapper shares
			 * the common shape with the rest of the codecs while not losing the peculiarities of the dialect
			 * @note The type of the value and the kind of the stored one are here one and the same field, whereas
			 *       the YAML codec has two of them. This divergence is due to the arrangement of the dialect: with YAML
			 *       the record of a value and its kind differ — `0x1F` is a record while the number thirty-one
			 *       is a kind — whereas TOML records the kind unambiguously at the parsing
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
					// Тип хранимого значения
					type_t _type;
				private:
					/**
					 * \~russian
					 * Запись строкового значения
					 *
					 * @details Запись хранится значением, а не берётся у дерева: владеющее
					 * значение дерево пережить обязано, и запись его переопределяться
					 * настройками чужого разбора не должна
					 *
					 * \~english
					 * Notation of the string value
					 * @details The notation is stored by the value rather than taken from the tree: an owning
					 * value must outlive the tree, and its notation must not be redefined by
					 * the settings of a foreign parsing
					 *
					 * \~
					 */
					string_t _quoting;
				private:
					/**
					 * \~russian
					 * Система счисления записи целого числа
					 *
					 * @details Хранится она рядом с числом, а не вместо него: запись `0x1F`
					 * обязана вернуться записью `0x1F`, а извлечение числа обязано выдать 31,
					 * и одно другому не замена
					 *
					 * \~english
					 * Radix of the record of the integer
					 * @details It is stored alongside the number rather than instead of it: the record `0x1F`
					 * must return as the record `0x1F`, while the extraction of the number must give away 31,
					 * and one is not a replacement for the other
					 *
					 * \~
					 */
					radix_t _radix;
				private:
					// Логическое значение
					bool _boolean;
				private:
					/**
					 * \~russian
					 * Признак записи перечня несколькими строками
					 *
					 * @note Держится ради записи в текст: расстановка строк выбрана
					 *       потребителем, и запись обязана её соблюдать
					 *
					 * \~english
					 * Flag of the writing of an array in several lines
					 * @note It is kept for the sake of the writing into a text: the arrangement of the lines has been
					 *       chosen by the consumer, and the writing is obliged to observe it
					 *
					 * \~
					 */
					bool _multiline;
				private:
					// Целое число со знаком
					int64_t _integer;
				private:
					// Число с плавающей точкой
					double _real;
				private:
					// Отметка времени
					stamp_t _stamp;
				private:
					// Содержимое строкового значения, хранимое собственной памятью
					string _text;
				private:
					/**
					 * \~russian
					 * Имена пар таблицы
					 *
					 * @details Перечень этот наполняется лишь у таблицы: у перечня значений
					 * имён нет вовсе, и хранить пустые строки ему незачем
					 *
					 * \~english
					 * Names of the pairs of a table
					 * @details This list is filled only for a table: an array of the values has no names
					 * at all, and there is no point for it to store empty strings
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
					 * @param name   имя пары, пустое - значение записывается без имени
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a writing stream
					 * @param writer writing stream where the value is placed
					 * @param name   name of the pair, an empty one — the value is written without a name
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool compose(writer_t & writer, const string_view name, const bool keyed = false) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод снятия значения с дерева настроек по составному имени
					 *
					 * @details Способ этот заменяет мост через взгляд на узел, каким его
					 * ведут кодеки JSON и YAML: у дерева настроек TOML взгляда на узел нет
					 * вовсе, и обращение к нему ведётся составным именем
					 *
					 * @param document дерево настроек, откуда снимается значение
					 * @param path     составное имя снимаемого значения
					 * @return         признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the taking of a value from a settings tree by a compound name
					 * @details This way replaces the bridge through a view onto a node, as the JSON and YAML
					 * codecs conduct it: the settings tree of TOML has no view onto a node at all,
					 * and the addressing to it is conducted by a compound name
					 * @param document settings tree wherefrom the value is taken
					 * @param path     compound name of the value being taken
					 * @return sign of the success of the taking
					 *
					 * \~
					 */
					bool absorb(const Document & document, const vector <string_view> & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки определённости значения
					 *
					 * @details Неопределённым значение бывает двояко: вновь заведённым, чей
					 * тип ещё не назначен, и выданным обращением по имени, какого нет.
					 * Различать эти два случая договор не требует - оба означают, что
					 * значения здесь нет
					 *
					 * @return признак определённости значения
					 *
					 * \~english
					 * @brief Method of the check of the definiteness of the value
					 * @details A value is indefinite in two ways: a newly created one whose type has not yet
					 * been assigned, and one given away by an addressing by a name which does not exist.
					 * The contract does not require distinguishing these two cases — both mean that there
					 * is no value here
					 * @return sign of the definiteness of the value
					 *
					 * \~
					 */
					bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения типа значения
					 *
					 * @return тип хранимого значения
					 *
					 * \~english
					 * @brief Method of the extraction of the type of the value
					 * @return type of the stored value
					 *
					 * \~
					 */
					type_t type() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки соответствия значения затребованному типу
					 *
					 * @param type сличаемый тип значения
					 * @return     признак соответствия значения затребованному типу
					 *
					 * \~english
					 * @brief Method of the check of the correspondence of the value to the demanded type
					 * @param type type of the value being compared
					 * @return sign of the correspondence of the value to the demanded type
					 *
					 * \~
					 */
					bool is(const type_t type) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества значений вместилища
					 *
					 * @note Значение, вместилищем не являющееся, отвечает нулём: своего
					 *       содержимого у него нет, а единицею считать себя самоё значило бы
					 *       смешать пустое вместилище с непустым простым значением
					 *
					 * @return количество значений вместилища
					 *
					 * \~english
					 * @brief Method of the extraction of the quantity of the values of a container
					 * @note A value which is not a container answers with a zero: it has no content of its own,
					 *       while to count itself as a unit would mean to mix up an empty container with
					 *       a non-empty simple value
					 * @return quantity of the values of the container
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки пустоты вместилища
					 *
					 * @return признак пустоты вместилища
					 *
					 * \~english
					 * @brief Method of the check of the emptiness of a container
					 * @return sign of the emptiness of the container
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса значения в исходное состояние
					 *
					 * \~english
					 * @brief Method of the reset of the value into the initial state
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого строкового значения
					 *
					 * @note Выдаётся содержимое, приведённое к окончательному виду, а не
					 *       запись его: управляющие последовательности основной записи уже
					 *       разобраны, а обрамляющие кавычки сняты
					 *
					 * @return содержимое строкового значения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a string value
					 * @note The content brought to its final form is given away rather than its record:
					 *       the escape sequences of the basic notation have already been parsed, while
					 *       the enclosing quotes have been removed
					 * @return content of the string value
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имени пары таблицы по номеру
					 *
					 * @param index порядковый номер пары таблицы
					 * @return      имя пары таблицы
					 *
					 * \~english
					 * @brief Method of the extraction of the name of a pair of a table by an index
					 * @param index ordinal index of the pair of the table
					 * @return name of the pair of the table
					 *
					 * \~
					 */
					const string & key(const size_t index) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения записи строкового значения
					 *
					 * @return запись строкового значения
					 *
					 * \~english
					 * @brief Method of the extraction of the notation of a string value
					 * @return notation of the string value
					 *
					 * \~
					 */
					string_t quoting() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки записи строкового значения
					 *
					 * @param quoting устанавливаемая запись строкового значения
					 *
					 * \~english
					 * @brief Method of the setting of the notation of a string value
					 * @param quoting notation of the string value being set
					 *
					 * \~
					 */
					void quoting(const string_t quoting) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения системы счисления записи целого числа
					 *
					 * @return система счисления записи целого числа
					 *
					 * \~english
					 * @brief Method of the extraction of the radix of the record of an integer
					 * @return radix of the record of the integer
					 *
					 * \~
					 */
					radix_t radix() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки системы счисления записи целого числа
					 *
					 * @param radix устанавливаемая система счисления
					 *
					 * \~english
					 * @brief Method of the setting of the radix of the record of an integer
					 * @param radix radix being set
					 *
					 * \~
					 */
					void radix(const radix_t radix) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения признака записи перечня несколькими строками
					 *
					 * @return признак записи перечня несколькими строками
					 *
					 * \~english
					 * @brief Method of the extraction of the flag of the writing of an array in several lines
					 * @return flag of the writing of the array in several lines
					 *
					 * \~
					 */
					bool multiline() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки признака записи перечня несколькими строками
					 *
					 * @param multiline устанавливаемый признак
					 *
					 * \~english
					 * @brief Method of the setting of the flag of the writing of an array in several lines
					 * @param multiline flag being set
					 *
					 * \~
					 */
					void multiline(const bool multiline) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения отметки времени
					 *
					 * @note Значаща отметка лишь у четырёх видов её: со смещением часового
					 *       пояса, без смещения, местной даты и местного времени. У прочих
					 *       видов значения она обнулена
					 *
					 * @return отметка времени
					 *
					 * \~english
					 * @brief Method of the extraction of a timestamp
					 * @note The timestamp is significant only for its four kinds: with an offset of the time
					 *       zone, without an offset, of a local date and of a local time. For the other kinds
					 *       of a value it is zeroed
					 * @return timestamp
					 *
					 * \~
					 */
					const stamp_t & stamp() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки отметки времени
					 *
					 * @param stamp устанавливаемая отметка времени
					 * @param type  вид отметки времени
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a timestamp
					 * @param stamp timestamp being set
					 * @param type  kind of the timestamp
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool stamp(const stamp_t & stamp, const type_t type) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод извлечения значения мусорного
					 *
					 * @details Значение это принимает на себя запись при неудачном обращении
					 * изменяемом: обращение такое обязано выдать ссылку, а завести значение
					 * ему нечем
					 *
					 * @note Значение это своё у всякого потока: одно на приложение оно
					 *       обратило бы запись мимо цели в гонку между потоками
					 *
					 * @return значение мусорное
					 *
					 * \~english
					 * @brief Method of the extraction of a scrap value
					 * @details This value takes upon itself a writing at an unsuccessful mutable
					 * addressing: such an addressing is obliged to give away a reference, while it has
					 * nothing wherewith to create a value
					 * @note This value is its own for every thread: one for the whole application it would turn
					 *       a writing past the target into a race between the threads
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
					 * неопределёнными до затребованного номера, и предел этот рост стережёт.
					 * Нуль снимает его вовсе
					 *
					 * @note Предел ограждает один лишь рост обращением по номеру.
					 *       Вместилище, разбором построенное, ему не подчинено: длину его
					 *       задаёт разбираемый текст
					 *
					 * @return предел роста вместилища по номеру
					 *
					 * \~english
					 * @brief Method of the extraction of the limit of the growth of a container by an index
					 * @details An access by an index grows a container by undefined values up to the requested
					 * index, and this limit guards that growth. A zero removes it entirely
					 * @note The limit guards only the growth by an access by an index. A container built by a parsing
					 * is not subject to it: its length is given by the text being parsed
					 * @return limit of the growth of a container by an index
					 *
					 * \~
					 */
					static size_t limit() noexcept;
					/**
					 * \~russian
					 * @brief Метод установки предела роста вместилища по номеру
					 *
					 * @details Предел этот пользователем рамки и ставится: сколько памяти
					 * есть у приложения, ведомо ему одному
					 *
					 * @param value устанавливаемый предел роста
					 *
					 * \~english
					 * @brief Method of the setting of the limit of the growth of a container by an index
					 * @details This limit is set by the user of the framework: how much memory an application has
					 * is known to it alone
					 * @param value limit of the growth being set
					 *
					 * \~
					 */
					static void limit(const size_t value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия пары таблицы по имени
					 *
					 * @param name имя искомой пары таблицы
					 * @return     признак наличия пары таблицы
					 *
					 * \~english
					 * @brief Method of the check of the presence of a pair of a table by a name
					 * @param name name of the pair of the table being sought
					 * @return sign of the presence of the pair of the table
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
					 * @brief Метод обращения к вложенному значению по пути
					 *
					 * @details Путь задаётся частями, отделёнными косой чертой: «/server/port».
					 * Часть, состоящая из одних десятичных цифр, читается порядковым номером
					 * значения перечня
					 *
					 * @note Значение, по пути не найденное, выдаётся значением неопределённым,
					 *       а не отказом: обращение к отсутствующему есть отсутствие значения,
					 *       и проверять его надлежит через `valid()`
					 *
					 * @param path путь до искомого значения
					 * @return     найденное значение
					 *
					 * \~english
					 * @brief Method of the addressing to a nested value by a path
					 * @details The path is given by parts separated by a slash: «/server/port».
					 * A part consisting of decimal digits alone is read as the ordinal index of a value
					 * of an array
					 * @note A value not found by the path is given away as an indefinite value rather than
					 *       as a refusal: an addressing to an absent one is the absence of a value,
					 *       and it ought to be checked through `valid()`
					 * @param path path to the value being sought
					 * @return found value
					 *
					 * \~
					 */
					const Value & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения вложенного значения по пути
					 *
					 * @details Недостающие вместилища заводятся по дороге: часть пути,
					 * числом не являющаяся, заводит таблицу, а числовая - перечень
					 *
					 * @param path путь до заводимого значения
					 * @return     заведённое значение
					 *
					 * \~english
					 * @brief Method of the creation of a nested value by a path
					 * @details The lacking containers are created along the way: a part of the path which is
					 * not a number creates a table, while a numeric one — an array
					 * @param path path to the value being created
					 * @return created value
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор обращения к паре таблицы по имени
					 *
					 * @param name имя искомой пары таблицы
					 * @return     найденное значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a pair of a table by a name
					 * @param name name of the pair of the table being sought
					 * @return found value
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к паре таблицы по имени с заведением
					 *
					 * @note Отличие от постоянного вида намеренно: изменяемый заводит пару
					 *       недостающую, ибо ему предстоит присваивание, а постоянный лишь
					 *       ищет - заведение при чтении наращивало бы таблицу молча
					 *
					 * @param name имя искомой пары таблицы
					 * @return     найденное либо заведённое значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a pair of a table by a name with a creation
					 * @note The difference from the constant kind is deliberate: the mutable one creates a lacking
					 *       pair, for an assignment lies ahead of it, while the constant one only seeks —
					 *       a creation at a reading would grow the table silently
					 * @param name name of the pair of the table being sought
					 * @return found or created value
					 *
					 * \~
					 */
					Value & operator [] (const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к значению перечня по номеру
					 *
					 * @param index порядковый номер значения перечня
					 * @return      найденное значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a value of an array by an index
					 * @param index ordinal index of the value of the array
					 * @return found value
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к значению перечня по номеру с заведением
					 *
					 * @param index порядковый номер значения перечня
					 * @return      найденное либо заведённое значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a value of an array by an index with a creation
					 * @param index ordinal index of the value of the array
					 * @return found or created value
					 *
					 * \~
					 */
					Value & operator [] (const size_t index) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод добавления значения в конец перечня
					 *
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a value to the end of an array
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки пары таблицы
					 *
					 * @details Имя занятое перезаписывается на прежнем месте: порядок пар
					 * задан потребителем, и перестановка их при перезаписи меняла бы вид
					 * записанного текста без его на то воли
					 *
					 * @note Имя пустое отвергается отказом, а не подставляется: пара без
					 *       имени в таблице невозможна, и молчаливая подстановка скрыла бы
					 *       ошибку у потребителя
					 *
					 * @param name  имя устанавливаемой пары
					 * @param value устанавливаемое значение
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a pair of a table
					 * @details An occupied name is overwritten in its former place: the order of the pairs is
					 * given by the consumer, and their rearrangement at an overwriting would change the appearance
					 * of the written text without its will for that
					 * @note An empty name is rejected with a refusal rather than substituted: a pair without
					 *       a name is impossible in a table, and a silent substitution would conceal
					 *       a mistake of the consumer
					 * @param name  name of the pair being set
					 * @param value value being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления пары таблицы без перезаписи
					 *
					 * @note Отличие от установки в том, что имя занятое отвергается отказом:
					 *       добавление есть заявление о новизне имени, и молчаливая
					 *       перезапись противоречила бы ему
					 *
					 * @param name  имя добавляемой пары
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a pair of a table without an overwriting
					 * @note The difference from the setting is that an occupied name is rejected with a refusal:
					 *       an addition is a declaration of the novelty of the name, and a silent overwriting
					 *       would contradict it
					 * @param name  name of the pair being added
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool append(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления пары таблицы по имени
					 *
					 * @param name имя удаляемой пары
					 * @return     признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a pair of a table by a name
					 * @param name name of the pair being removed
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления значения вместилища по номеру
					 *
					 * @param index порядковый номер удаляемого значения
					 * @return      признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a value of a container by an index
					 * @param index ordinal index of the value being removed
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
					 * числом не является вовсе. Вид хранения извлечению не указ: целое
					 * выдаётся и дробным видом, а дробное - и целым
					 *
					 * @details Дробное, извлекаемое целым видом, округляется по правилам
					 * математики с уводом половины от нуля: `1.5` выдаётся двойкой, а `-1.5` -
					 * минус двойкой. Целое, за отрезок затребованного вида выходящее,
					 * переносится младшими разрядами, а дробное вне его пределов выдаётся
					 * пределом: приведение такое стандарт зовёт неопределённым поведением,
					 * а неопределённого поведения в кодеке не будет
					 *
					 * @note Договор этот общий у всех пяти кодеков рамки, и сличает их
					 *       между собою `CodecContract.NumberExtraction`
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a number
					 * @details The extraction ends with a refusal only when the value is not a number
					 * at all. The kind of the storage is not a directive to the extraction: an integer is issued
					 * also as a fractional kind, and a fractional one — also as an integer
					 * @details A fractional number extracted as an integer kind is rounded by the rules
					 * of mathematics with a half taken away from zero: `1.5` is issued as a two, while `-1.5` —
					 * as a minus two. An integer going beyond the range of the requested kind is carried over
					 * by the lower bits, while a fractional one beyond its limits is issued as the limit: such a
					 * conversion is called an undefined behaviour by the standard, and there will be no
					 * undefined behaviour in the codec
					 * @note This contract is common to all five codecs of the framework, and they are compared
					 *       among themselves by `CodecContract.NumberExtraction`
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(float & result) const noexcept;
					/**
					 * @copydoc awh::codec::toml::Value::value(int8_t &) const
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого строкового значения
					 *
					 * @note Отказом извлечение завершается у всякого значения, строковым не
					 *       являющегося: выдавать число записью его значило бы подменять
					 *       извлечение записью в текст
					 *
					 * @param result переменная, куда помещается извлечённое содержимое
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a string value
					 * @note The extraction ends with a refusal for every value which is not a string one:
					 *       to give away a number as its record would mean to substitute the extraction
					 *       with a writing into a text
					 * @param result variable where the extracted content is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста настроек во владеющее значение
					 *
					 * @param text разбираемый текст настроек
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a settings text into an owning value
					 * @param text settings text being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста настроек во владеющее значение с настройками
					 *
					 * @param text     разбираемый текст настроек
					 * @param settings настройки разбора
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a settings text into an owning value with settings
					 * @param text     settings text being parsed
					 * @param settings settings of the parsing
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const Document::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения текста настроек из файла во владеющее значение
					 *
					 * @param filename имя читаемого файла
					 * @return         признак успешности чтения
					 *
					 * \~english
					 * @brief Method of the reading of a settings text from a file into an owning value
					 * @param filename name of the file being read
					 * @return sign of the success of the reading
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения текста настроек из файла с настройками
					 *
					 * @param filename имя читаемого файла
					 * @param settings настройки разбора
					 * @return         признак успешности чтения
					 *
					 * \~english
					 * @brief Method of the reading of a settings text from a file with settings
					 * @param filename name of the file being read
					 * @param settings settings of the parsing
					 * @return sign of the success of the reading
					 *
					 * \~
					 */
					bool load(const string & filename, const Document::settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи владеющего значения текстом настроек
					 *
					 * @note Оформление исходного текста при этом не восстанавливается:
					 *       удержание его есть дело дерева настроек, и правка чужого файла
					 *       без потери расстановки и замечаний ведётся только через дерево
					 *
					 * @return записанный текст настроек
					 *
					 * \~english
					 * @brief Method of the writing of an owning value as a settings text
					 * @note The formatting of the source text is thereby not restored: its retention
					 *       is the business of the settings tree, and the editing of a foreign file
					 *       without the loss of the arrangement and of the remarks is conducted only through the tree
					 * @return written settings text
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи владеющего значения текстом настроек с настройками
					 *
					 * @param settings настройки записи
					 * @return         записанный текст настроек
					 *
					 * \~english
					 * @brief Method of the writing of an owning value as a settings text with settings
					 * @param settings settings of the writing
					 * @return written settings text
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи владеющего значения в файл
					 *
					 * @param filename имя записываемого файла
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an owning value into a file
					 * @param filename name of the file being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод переноса владеющего значения в дерево настроек
					 *
					 * @details Перенос ведётся составным именем: у дерева настроек TOML
					 * взгляда на узел нет вовсе, и место переноса задаётся именем
					 *
					 * @note Перечни переносятся наравне с прочим: у места объявляется перечень
					 *       пустой, а значения его доливаются - вложенные перечни и встроенные
					 *       таблицы тем же способом. Перенос поверх готового дерева перечня не
					 *       наращивает: объявление кладёт его заново
					 *
					 * @param document дерево настроек, куда переносится значение
					 * @param path     составное имя места переноса
					 * @return         признак успешности переноса
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into a settings tree
					 * @details The grafting is conducted by a compound name: the settings tree of TOML has
					 * no view onto a node at all, and the place of the grafting is given by a name
					 * @note The arrays are grafted on a par with the rest: an empty array is declared at the place
					 *       while its values are appended — the nested arrays and the inline tables in the same
					 *       way. A grafting on top of a ready tree does not grow an array:
					 *       the declaring lays it down anew
					 * @param document settings tree whereinto the value is grafted
					 * @param path     compound name of the place of the grafting
					 * @return sign of the success of the grafting
					 *
					 * \~
					 */
					bool graft(Document & document, const vector <string_view> & path) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сборки содержимого значения для правки дерева настроек
					 *
					 * @param result собираемое содержимое значения
					 *
					 * \~english
					 * @brief Method of the assembling of the content of a value for an editing of a settings tree
					 * @param result content of the value being assembled
					 *
					 * \~
					 */
					void contented(content_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод наполнения составного значения, у места уже объявленного
					 *
					 * @param document дерево настроек, куда переносится значение
					 * @param path     составное имя места наполняемого значения
					 * @return         признак успешности наполнения
					 *
					 * \~english
					 * @brief Method of the filling of a compound value already declared at a place
					 * @param document settings tree whereinto the value is grafted
					 * @param path     compound name of the place of the value being filled
					 * @return         sign of the success of the filling
					 *
					 * \~
					 */
					bool inflate(Document & document, const vector <string_view> & path) const noexcept;
				public:
				public:
					/**
					 * \~russian
					 * @brief Оператор сличения значений
					 *
					 * @details Сличается суть значения, а не запись его: система счисления,
					 * запись строки и расстановка строк перечня сличению не подлежат, а
					 * `0x1F` и `31` суть одно число
					 *
					 * @note Сличение таблиц порядка пар НЕ учитывает, а сличение перечней
					 *       учитывает: таблица есть отображение имён на значения, и порядок
					 *       записи её значением не является
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values
					 * @details The essence of the value is compared rather than its record: the radix,
					 * the notation of a string and the arrangement of the lines of an array are not subject
					 * to the comparison, while `0x1F` and `31` are one number
					 * @note The comparison of the tables does NOT take the order of the pairs into account, while
					 *       the comparison of the arrays does: a table is a mapping of the names onto the values,
					 *       and the order of its writing is not a value
					 * @param value value being compared
					 * @return sign of the coincidence of the values
					 *
					 * \~
					 */
					bool operator == (const Value & value) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор сличения значений на расхождение
					 *
					 * @param value сличаемое значение
					 * @return      признак расхождения значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values for a divergence
					 * @param value value being compared
					 * @return sign of the divergence of the values
					 *
					 * \~
					 */
					bool operator != (const Value & value) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор присваивания значения
					 *
					 * @param value присваиваемое значение
					 * @return      ссылка на текущее значение
					 *
					 * \~english
					 * @brief Operator of the assignment of a value
					 * @param value value being assigned
					 * @return reference to the current value
					 *
					 * \~
					 */
					Value & operator = (const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Оператор присваивания значения переносом
					 *
					 * @param value переносимое значение
					 * @return      ссылка на текущее значение
					 *
					 * \~english
					 * @brief Operator of the assignment of a value by a move
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
					 * @brief Конструктор вместилища затребованного типа
					 *
					 * @note Конструктор объявлен явным намеренно: без того перечисление
					 *       типов приводилось бы к значению молча, и `push(type_t::TABLE)`
					 *       читалось бы добавлением таблицы там, где написана опечатка
					 *
					 * @param type тип заводимого значения
					 *
					 * \~english
					 * @brief Constructor of a container of the demanded type
					 * @note The constructor is declared explicit deliberately: without that the enumeration
					 *       of the types would be converted to a value silently, and `push(type_t::TABLE)`
					 *       would read as an addition of a table there where a typo has been written
					 * @param type type of the value being created
					 *
					 * \~
					 */
					explicit Value(const type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор логического значения
					 *
					 * @param value устанавливаемое логическое значение
					 *
					 * \~english
					 * @brief Constructor of a logical value
					 * @param value logical value being set
					 *
					 * \~
					 */
					Value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор целого числа
					 *
					 * @param value устанавливаемое целое число
					 * @param radix система счисления записи целого числа
					 *
					 * \~english
					 * @brief Constructor of an integer
					 * @param value integer being set
					 * @param radix radix of the record of the integer
					 *
					 * \~
					 */
					Value(const int64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор целого числа без знака
					 *
					 * @note Вид этот заведён не ради своего хранения, а ради однозначности
					 *       вызова: целых чисел без знака у наречия TOML нет, и без такого
					 *       вида запись `Value(5u)` расходилась бы между целым, дробным и
					 *       логическим видами приведением равной силы, отчего сборка
					 *       отвечала бы двусмысленностью. Число ложится целым со знаком
					 *       приведением языка
					 *
					 * @param value устанавливаемое целое число без знака
					 * @param radix система счисления записи целого числа
					 *
					 * \~english
					 * @brief Constructor of an unsigned integer
					 * @note This kind is created not for the sake of its own storage but for the sake of the
					 *       unambiguity of the call: the TOML dialect has no unsigned integers, and without such
					 *       a kind the record `Value(5u)` would diverge between the integer, the fractional and
					 *       the logical kinds by a conversion of an equal rank, whereby the build would answer
					 *       with an ambiguity. The number is laid as a signed integer by a conversion of the language
					 * @param value unsigned integer being set
					 * @param radix radix of the record of the integer
					 *
					 * \~
					 */
					Value(const uint64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор числа с плавающей точкой
					 *
					 * @param value устанавливаемое число с плавающей точкой
					 *
					 * \~english
					 * @brief Constructor of a floating-point number
					 * @param value floating-point number being set
					 *
					 * \~
					 */
					Value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор строкового значения
					 *
					 * @param value   устанавливаемое содержимое
					 * @param quoting запись строкового значения
					 *
					 * \~english
					 * @brief Constructor of a string value
					 * @param value   content being set
					 * @param quoting notation of the string value
					 *
					 * \~
					 */
					Value(const string & value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор строкового значения из строки языка
					 *
					 * @note Конструктор этот заведён рядом с принимающим `string`
					 *       намеренно: без него запись `Value("текст")` уходила бы к
					 *       конструктору логического значения через приведение указателя
					 *
					 * @param value   устанавливаемое содержимое
					 * @param quoting запись строкового значения
					 *
					 * \~english
					 * @brief Constructor of a string value from a string of the language
					 * @note This constructor is created next to the one accepting a `string` deliberately:
					 *       without it the record `Value("текст")` would go to the constructor of a logical
					 *       value through a conversion of the pointer
					 * @param value   content being set
					 * @param quoting notation of the string value
					 *
					 * \~
					 */
					Value(const char * value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор снятия значения с дерева настроек
					 *
					 * @param document дерево настроек, откуда снимается значение
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a settings tree
					 * @param document settings tree wherefrom the value is taken
					 *
					 * \~
					 */
					explicit Value(const Document & document) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор снятия значения с дерева настроек по составному имени
					 *
					 * @param document дерево настроек, откуда снимается значение
					 * @param path     составное имя снимаемого значения
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a settings tree by a compound name
					 * @param document settings tree wherefrom the value is taken
					 * @param path     compound name of the value being taken
					 *
					 * \~
					 */
					Value(const Document & document, const vector <string_view> & path) noexcept;
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
			 * путями, а не вместо них. Потребитель, пары подряд заносящий, вложенностей не
			 * порождает и путями не пользуется вовсе: розыск по пути на всякую пару стоит
			 * прохода дерева, а сборке достаточно знать, где она стоит сейчас
			 *
			 * @details Договор её слово в слово повторяет договор потока записи: открыть
			 * вместилище, назвать пару, записать значение, закрыть. Тем «пишу текстом» и
			 * «строю дерево» отличаются одной буквой, и переход с одного на другое не
			 * требует переучиваться
			 *
			 * @note Вместилища зовутся по наречию: у TOML это `table` и `array`, тогда как
			 *       у YAML те же вместилища зовутся отображением и перечнем. Имя берётся у
			 *       наречия, а не сводится к общему: потребитель кодека читает описание
			 *       наречия, а не рамки
			 *
			 * @note Записи пустого значения у сборки нет вовсе, тогда как договор рамки её
			 *       поминает: пустого значения нет у самого наречия TOML - пара без
			 *       значения там записана быть не может, а описание прямо велит опускать
			 *       пару целиком. Заводить `null()`, отвечающий одним отказом, значило бы
			 *       обещать потребителю то, чего наречие не умеет
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value
			 * @details This assembly is the second way to build a tree, standing alongside
			 * the paths rather than instead of them. A consumer entering the pairs in a row does not
			 * generate the nestings and does not use the paths at all: a search by a path for every pair costs
			 * a traversal of the tree, while the assembly only needs to know where it stands now
			 * @details Its contract repeats the contract of the writing stream word for word: open
			 * a container, name a pair, write a value, close. Thereby "I write a text" and
			 * "I build a tree" differ by one letter, and the transition from one to the other does not
			 * require relearning
			 * @note The containers are called by the dialect: with TOML these are `table` and `array`, whereas
			 *       with YAML the same containers are called a mapping and a sequence. The name is taken from
			 *       the dialect rather than reduced to a common one: a consumer of a codec reads the description
			 *       of the dialect rather than of the framework
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
					 * указатель на открытое вместилище стал бы висячим на первой же паре
					 *
					 * \~english
					 * Path to the container opened by the assembly
					 * @details The path is stored by the indices rather than by the pointers: the values of a container
					 * lie in a list, and every addition relocates it in the memory, whereby
					 * a pointer to an opened container would become dangling at the very first pair
					 *
					 * \~
					 */
					vector <size_t> _path;
				private:
					// Имя пары таблицы, значения ожидающее
					string _key;
				private:
					// Признак того, что имя пары таблицы назначено
					bool _keyed;
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
					 * a closed one: the path of the indices of both is empty, and without the sign the closing
					 * of the root would remain unnoticed
					 *
					 * \~
					 */
					bool _done;
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
					 * @brief Метод открытия вместилища затребованного типа
					 *
					 * @param value открываемое вместилище
					 * @return      признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a container of the demanded type
					 * @param value container being opened
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool expand(Value && value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия таблицы
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a table
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool table() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений
					 *
					 * @param multiline признак записи перечня несколькими строками
					 * @return          признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of an array of the values
					 * @param multiline flag of the writing of the array in several lines
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool array(const bool multiline = false) noexcept;
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
					 * @brief Метод назначения имени пары таблицы
					 *
					 * @note Имя пустое отвергается отказом, а имя, назначенное дважды
					 *       подряд, - тоже: и то и другое есть ошибка у потребителя, и
					 *       молчаливое приятие её увело бы значение не туда
					 *
					 * @param name назначаемое имя пары таблицы
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a pair of a table
					 * @note An empty name is rejected with a refusal, and a name assigned twice
					 *       in a row — likewise: both the one and the other are a mistake of the consumer,
					 *       and a silent acceptance of it would take the value not where it belongs
					 * @param name name of the pair of the table being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
				public:
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
					 * @brief Метод записи целого числа со знаком
					 *
					 * @param value записываемое целое число
					 * @param radix система счисления записи целого числа
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a signed integer
					 * @param value integer being written
					 * @param radix radix of the record of the integer
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const int64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа без знака
					 *
					 * @note Вид этот заведён ради однозначности вызова, а не ради своего
					 *       хранения: целых чисел без знака у наречия TOML нет
					 *
					 * @param value записываемое целое число без знака
					 * @param radix система счисления записи целого числа
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an unsigned integer
					 * @note This kind is created for the sake of the unambiguity of the call rather than for the sake
					 *       of its own storage: the TOML dialect has no unsigned integers
					 * @param value unsigned integer being written
					 * @param radix radix of the record of the integer
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const uint64_t value, const radix_t radix = radix_t::DECIMAL) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числа с плавающей точкой
					 *
					 * @param value записываемое число с плавающей точкой
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a floating-point number
					 * @param value floating-point number being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value   записываемое содержимое
					 * @param quoting запись строкового значения
					 * @return        признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value   content being written
					 * @param quoting notation of the string value
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * @copydoc awh::codec::toml::Builder::value(const string &, const string_t)
					 */
					bool value(const char * value, const string_t quoting = string_t::BASIC) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи отметки времени
					 *
					 * @param value записываемая отметка времени
					 * @param type  вид отметки времени
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a timestamp
					 * @param value timestamp being written
					 * @param type  kind of the timestamp
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const stamp_t & value, const type_t type) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения глубины открытых вместилищ
					 *
					 * @return глубина открытых вместилищ
					 *
					 * \~english
					 * @brief Method of the extraction of the depth of the opened containers
					 * @return depth of the opened containers
					 *
					 * \~
					 */
					size_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса сборки в исходное состояние
					 *
					 * \~english
					 * @brief Method of the reset of the assembly into the initial state
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия собранного значения
					 *
					 * @note Сборка при этом сбрасывается: собранное значение уходит
					 *       потребителю переносом, и оставлять сборке пустую его оболочку
					 *       значило бы отдать её дважды
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the withdrawal of the assembled value
					 * @note The assembly is thereby reset: the assembled value goes away to the consumer
					 *       by a move, and to leave its empty shell to the assembly would mean
					 *       to give it away twice
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

#endif // __AWH_CODEC_TOML_VALUE__
