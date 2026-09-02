/**
 * @file value.hpp
 * @date 2026-08-19
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
 * @brief Заголовочный файл владеющего значения бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the owning value of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_VALUE__
#define __AWH_CODEC_ABC_VALUE__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
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
#include "document.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"

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
			 * @brief Класс владеющего значения документа
			 *
			 * @details Значение владеет своим поддеревом и складывается: его кладут в
			 * вместилища, передают наружу из работ и собирают из ничего. Дерево документа
			 * тем и отличается: `Document::Value` есть взгляд - указатель на документ вместе
			 * с номером узла, - и документ он не переживает
			 *
			 * @details **Отличия от владеющего значения текстовых кодеков.** Имя поля
			 * отображения выдаётся значением, а не строкой: у ABC именем вправе стоять
			 * значение любого вида, кроме вместимого. Обращение по имени-строке при этом
			 * сохраняется, ибо строка именем - обиход, а прочие виды - частность
			 *
			 * @details **Возвратность.** Очистка, разрушение, перенесение из дерева и укладка
			 * в запись ведутся без возвратности: дерево на десятки тысяч уровней иначе сорвало
			 * бы стек. Копирование же и присваивание возвратны по устройству вместилища
			 * значений, и глубина им предел
			 *
			 * @details **Чего значение не делает.** Оформления исходной записи оно не
			 * удерживает: неопределённая длина вместимого, будучи разобрана, обращается в
			 * объявленную. Правка записи с сохранением того, как она была уложена, - дело
			 * документа, а не значения
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 * лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 * равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 * тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 * четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 * ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 * свободно
			 *
			 * \~english
			 * @brief Class of an owning value of a document
			 * @details A value owns its own subtree and is composable: it is put into the containers,
			 * passed outward from the works and assembled from nothing. The tree of a document differs in that:
			 * `Document::Value` is a view — a pointer to the document together with the number of the node, —
			 * and it does not outlive the document
			 * @details **Differences from the owning value of the textual codecs.** The name of a field
			 * of a mapping is issued by a value rather than by a string: in ABC a value of any kind except
			 * a container has the right to stand as a name. The access by a name-string is thereby
			 * preserved, for a string as a name is the custom, while the other kinds are a particularity
			 * @details **Recursion.** The clearing, the destruction, the transfer from a tree and the laying
			 * into a record are conducted without a recursion: a tree of tens of thousands of levels would otherwise break
			 * the stack. The copying and the assignment, however, are recursive by the structure of the container
			 * of the values, and the depth is a limit to them
			 * @details **What the value does not do.** It does not retain the formatting of the source
			 * record: an indefinite length of a container, being parsed, turns into a declared one. The editing
			 * of a record with the preservation of how it was laid is the business of the document rather than of the value
			 *
			 * \~
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 * holds a lock, and one must not judge the others by it. A measurement of 25.08.2026, one `Fetcher`
			 * on four threads: thirteen reports of TSan and nineteen records of four hundred read wrongly,
			 * silently. An own object per thread over a SHARED source of the reading: zero reports,
			 * zero divergences
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				public:
					/**
					 * \~russian
					 * @brief Метод установки объекта логирования
					 *
					 * @details Значение есть данные, а не работающий модуль: пара доставляется
					 * ему вызовом, а не конструктором, иначе неявное приведение вида
					 * `value_t v = "текст"` стало бы невозможным. Устройство это взято у
					 * `awh::Buffer`, где решён тот же вопрос
					 *
					 * @param log объект работы с логами
					 *
					 * \~english
					 * @brief Method setting the logging object
					 *
					 * @param log object for working with logs
					 *
					 * \~
					 */
					void setLogger(const log_t * log) noexcept;
				private:
					// Объект работы с логами
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * @brief Число значения, хранимое родным видом
					 *
					 * \~english
					 * @brief Number of a value stored by a native kind
					 *
					 * \~
					 */
					typedef union Numeric {
						// Логическое значение
						bool flag;
						// Целое со знаком, а у отметки времени - её значение
						int64_t integer;
						// Целое без знака
						uint64_t natural;
						// Дробное число
						double real;
					} numeric_t;
				private:
					// Вид узла значения
					kind_t _kind;
				private:
					// Вид значения
					type_t _type;
				private:
					// Число значения, хранимое родным видом
					numeric_t _number;
				private:
					/**
					 * \~russian
					 * Содержимое строки, двоичных данных, опознавателя либо октеты величины
					 *
					 * @details Поле это одно на четыре употребления намеренно: строка числа не
					 * хранит, число октетов строки не имеет, а разводить их по полям значило бы
					 * держать пустое поле у всякого значения
					 *
					 * \~english
					 * Content of a string, of binary data, of an identifier or the octets of a magnitude
					 * @details This field is one for the four usages deliberately: a string does not store a number,
					 * a number does not have the octets of a string, and to separate them by the fields would mean
					 * to keep an empty field for every value
					 *
					 * \~
					 */
					string _text;
				private:
					// Десятичный порядок величины числа неограниченной ширины
					int64_t _exponent;
				private:
					// Признак того, что величина числа меньше нуля
					bool _negative;
				private:
					/**
					 * \~russian
					 * Имена полей отображения
					 *
					 * @details Имя есть такое же значение, а не строка: именем вправе стоять
					 * число, отметка времени и опознаватель. У перечня вместилище это пусто и
					 * памяти не занимает
					 *
					 * \~english
					 * Names of the fields of a mapping
					 * @details A name is the same value rather than a string: a number, a time stamp
					 * and an identifier have the right to stand as a name. For a sequence this container is empty and
					 * does not occupy any memory
					 *
					 * \~
					 */
					vector <Value> _keys;
				private:
					// Значения вместимого
					vector <Value> _items;
				private:
					/**
					 * \~russian
					 * Указатель поиска поля отображения по имени
					 *
					 * @details Поиск ведётся перебором имён, покуда полей меньше `INDEX_LIMIT`,
					 * и указателем далее. Перебор при тысячах полей обращает и сборку, и чтение
					 * в квадратичные: замерено 21.08.2026 - 3.26 мкс на установку при 2500
					 * полях против 18.90 при 20000, и столько же на обращение
					 *
					 * @note Указатель заводится ЛЕНИВО и держится значением необязательным:
					 * узел мелкого отображения не платит за него ни памятью, ни выделением,
					 * а таких узлов в дереве подавляющее большинство
					 *
					 * @note Указатель ведётся приращением, а не перестроением: перестроение на
					 * всякой правке вернуло бы ту же квадратичность, от какой он и заводится
					 *
					 * \~english
					 * Index of the search of a field of a mapping by a name
					 * @details The search is conducted by the enumeration of the names while there are fewer
					 * fields than `INDEX_LIMIT`, and by the index further on
					 * @note The index is created LAZILY: a node of a small mapping does not pay for it
					 * @note The index is maintained by an increment rather than by a rebuilding
					 *
					 * \~
					 */
					mutable unique_ptr <unordered_map <string, size_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод разыскания поля отображения по имени
					 *
					 * @details Перебор либо указатель - смотря по количеству полей. Способ
					 * выбирается здесь одним местом, дабы пути поиска не разошлись
					 *
					 * @param name имя разыскиваемого поля отображения
					 * @return     номер поля отображения, `size()` при отсутствии
					 *
					 * \~english
					 * @brief Method of the searching of a field of a mapping by a name
					 * @param name name of the field of the mapping being searched for
					 * @return number of the field of the mapping, `size()` at the absence
					 *
					 * \~
					 */
					size_t locate(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сноса указателя поиска
					 *
					 * @details Сносится указатель при всякой перестановке полей: удаление
					 * сдвигает номера всех полей после удалённого, и починка его обошлась бы
					 * дороже, чем заведение заново при первом же поиске
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
					 * @brief Метод разбора звена пути на номер значения
					 *
					 * @details Разбор отвергает запись целиком, а не приводит её к пределу:
					 * вместимого такой длины не бывает вовсе. Ведущий нуль номером не является,
					 * а является именем поля
					 *
					 * @param segment разбираемое звено пути
					 * @param result  разобранный номер значения
					 * @return        признак того, что звено является номером
					 *
					 * \~english
					 * @brief Method of the parsing of a link of a path into a number of a value
					 * @details The parsing rejects the record as a whole rather than bringing it to the limit:
					 * a container of such a length does not exist at all. A leading zero is not a number
					 * but is the name of a field
					 * @param segment link of the path being parsed
					 * @param result parsed number of the value
					 * @return sign that the link is a number
					 *
					 * \~
					 */
					static bool indexed(const string_view segment, size_t & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод поверки родственности имён полей отображения
					 *
					 * @details Тождество ИМЕНИ строже равенства значений, и строгость эта взята
					 * у самой записи: имена там сличаются полной записью, а целое `42` и дробное
					 * `42.0` записываются РАЗНЫМИ метками, то есть суть разные имена. Оператор
					 * равенства же сличает числа величиною поверх вида хранения - и правильно
					 * делает, ибо он о ЗНАЧЕНИЯХ, а не об именах
					 *
					 * @note Без поверки этой заведение поля отвечало успехом, ПОТЕРЯВ поле:
					 * замерено щупом 30.08.2026 - внесение имён `42` и `42.0` оба отвечали
					 * успехом, а полей у отображения выходило ОДНО. Меж тем сборщик записи оба
					 * имени укладывает, и разбор такой записи в значение даёт ДВА поля: собрать
					 * заведением то, что разбирается чтением, было нельзя
					 *
					 * @note Расходятся ровно два перехода: целое с дробным да целое неограниченной
					 * ширины с десятичным. Отметка времени с целым не схлопывается - разряд вида
					 * у неё свой
					 *
					 * @param first  первое сличаемое имя поля
					 * @param second второе сличаемое имя поля
					 * @return       признак того, что имена принадлежат одному виду записи
					 *
					 * \~english
					 * @brief Method of the checking of the kinship of the names of the fields of a mapping
					 * @details The identity of a NAME is stricter than the equality of the values, and this
					 * strictness is taken from the record itself: the integer `42` and the real `42.0` are
					 * written by DIFFERENT tags, that is, they are different names
					 * @param first first name of a field being compared
					 * @param second second name of a field being compared
					 * @return sign that the names belong to one kind of the record
					 *
					 * \~
					 */
					[[nodiscard]] static bool akin(const Value & first, const Value & second) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения предела роста вместимого
					 *
					 * @details Предел ограждает **один лишь** рост обращением по номеру:
					 * `place("/1000000000")` иначе завёл бы миллиард значений и завершил бы
					 * работу нехваткой памяти. Разбору записи и добавлению в конец предел не
					 * указ - длину их задаёт сам потребитель
					 *
					 * @note Предел наглухо прописан быть не может: AWH есть рамка, а не служба,
					 * и сколько памяти будет у приложения, ведомо его создателю. Оттого предел
					 * ставится потребителем и имеет значение по умолчанию, а ноль снимает его
					 * вовсе
					 *
					 * @return предел роста вместимого
					 *
					 * \~english
					 * @brief Method of the extraction of the limit of the growth of a container
					 * @details The limit guards **only** the growth by an access by a number:
					 * `place("/1000000000")` would otherwise create a billion of the values and would terminate
					 * the work by a shortage of the memory. The limit is not a directive to the parsing of a record and to the appending
					 * to the end — their length is set by the consumer itself
					 * @note The limit cannot be prescribed rigidly: AWH is a framework rather than a service,
					 * and how much memory an application will have is known to its creator. Therefore the limit
					 * is set by the consumer and has a default value, while zero removes it
					 * altogether
					 * @return limit of the growth of a container
					 *
					 * \~
					 */
					static size_t limit() noexcept;
					/**
					 * \~russian
					 * @brief Метод установки предела роста вместимого
					 *
					 * @param value устанавливаемый предел, ноль - без предела
					 *
					 * \~english
					 * @brief Method of the setting of the limit of the growth of a container
					 * @param value limit being set, zero — without a limit
					 *
					 * \~
					 */
					static void limit(const size_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения ссылки на отсутствующее значение
					 *
					 * @return ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a reference to an absent value
					 * @return reference to an absent value
					 *
					 * \~
					 */
					static const Value & undefined() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения ссылки на отбросное значение
					 *
					 * @details Ссылка эта выдаётся изменяемым обращением там, где завести
					 * значение не удалось. Записанное в неё пропадает, но обращение к ней
					 * законно, и падения не происходит
					 *
					 * @return ссылка на отбросное значение
					 *
					 * \~english
					 * @brief Method of the extraction of a reference to a scrap value
					 * @details This reference is issued by a mutable access where a value
					 * could not be created. That written into it disappears, but an access to it
					 * is lawful, and no crash occurs
					 * @return reference to a scrap value
					 *
					 * \~
					 */
					static Value & scrap() noexcept;
					/**
					 * \~russian
					 * @brief Метод копирования значения без возвратности
					 *
					 * @details Копирование вместимого вместе с детьми возвратно по устройству
					 * вместилища, и дерево в десятки тысяч уровней срывало им стек. Обход ведётся
					 * своим вместилищем пар «откуда - куда», как то сделано у очистки и укладки
					 *
					 * @param value копируемое значение
					 *
					 * \~english
					 * @brief Method of the copying of a value without a recursion
					 * @details The copying of a container together with its children is recursive by the design
					 * of the storage, and a tree of tens of thousands of levels tore the stack by it
					 * @param value value being copied
					 *
					 * \~
					 */
					void clone(const Value & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки действительности значения
					 *
					 * @return признак действительности значения
					 *
					 * \~english
					 * @brief Method of the checking of the validity of a value
					 * @return sign of the validity of the value
					 *
					 * \~
					 */
					[[nodiscard]] bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида узла значения
					 *
					 * @return вид узла значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the node of a value
					 * @return kind of the node of the value
					 *
					 * \~
					 */
					kind_t kind() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида значения
					 *
					 * @return вид значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of a value
					 * @return kind of the value
					 *
					 * \~
					 */
					type_t type() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки принадлежности значения к виду
					 *
					 * @param type вид значения, сборный либо точный
					 * @return     признак принадлежности значения к виду
					 *
					 * \~english
					 * @brief Method of the checking of the belonging of a value to a kind
					 * @param type kind of the value, composite or exact
					 * @return sign of the belonging of the value to the kind
					 *
					 * \~
					 */
					[[nodiscard]] bool is(const type_t type) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества значений вместимого
					 *
					 * @return количество значений вместимого
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the values of a container
					 * @return number of the values of the container
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки пустоты значения
					 *
					 * @return признак пустоты значения
					 *
					 * \~english
					 * @brief Method of the checking of the emptiness of a value
					 * @return sign of the emptiness of the value
					 *
					 * \~
					 */
					[[nodiscard]] bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки значения
					 *
					 * @note Очистка ведётся без возвратности: дерево на десятки тысяч уровней
					 * иначе сорвало бы стек уже при разрушении значения
					 *
					 * \~english
					 * @brief Method of the clearing of a value
					 * @note The clearing is conducted without a recursion: a tree of tens of thousands of levels
					 * would otherwise break the stack already at the destruction of the value
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого значения
					 *
					 * @return содержимое значения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a value
					 * @return content of the value
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения десятичного порядка величины
					 *
					 * @return десятичный порядок величины
					 *
					 * \~english
					 * @brief Method of the extraction of the decimal exponent of a magnitude
					 * @return decimal exponent of the magnitude
					 *
					 * \~
					 */
					int64_t exponent() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки того, что величина меньше нуля
					 *
					 * @return признак того, что величина меньше нуля
					 *
					 * \~english
					 * @brief Method of the checking that a magnitude is less than zero
					 * @return sign that the magnitude is less than zero
					 *
					 * \~
					 */
					[[nodiscard]] bool negative() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения цифр числа неограниченной ширины
					 *
					 * @details Целое любой ширины и десятичное несут величину октетами, а не
					 * родным числом: ни одно числовое извлечение их не берёт, и без этого вида
					 * число, разобранное в дерево, было бы можно лишь перенести, но не прочесть.
					 * Знак берётся `negative()`, десятичный разряд - `exponent()`
					 *
					 * @param result извлекаемые цифры числа старшим октетом вперёд
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of the digits of a number of an unlimited width
					 * @details An integer of any width and a decimal carry the magnitude by octets rather than
					 * by a native number: no numeric extraction takes them
					 * @param result extracted digits of the number, the most significant octet first
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool digits(vector <uint8_t> & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имени поля отображения по его номеру
					 *
					 * @param index номер пары отображения
					 * @return      имя поля отображения
					 *
					 * \~english
					 * @brief Method of the extraction of the name of a field of a mapping by its number
					 * @param index number of the pair of the mapping
					 * @return name of the field of the mapping
					 *
					 * \~
					 */
					const Value & key(const size_t index) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия поля отображения по имени
					 *
					 * @param name имя поля отображения
					 * @return     признак наличия поля отображения
					 *
					 * \~english
					 * @brief Method of the checking of the presence of a field of a mapping by a name
					 * @param name name of the field of the mapping
					 * @return sign of the presence of the field of the mapping
					 *
					 * \~
					 */
					[[nodiscard]] bool contains(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения по пути
					 *
					 * @details Звенья пути разделяются косой чертой. Звено из одних цифр без
					 * ведущего нуля есть номер значения вместимого, прочие - имя поля отображения
					 *
					 * @param path путь к значению
					 * @return     ссылка на значение либо ссылка на отсутствующее значение
					 *
					 * \~english
					 * @brief Method of the extraction of a value by a path
					 * @details The links of a path are separated by a slash. A link of digits alone without
					 * a leading zero is the number of a value of a container, the others are the name of a field of a mapping
					 * @param path path to the value
					 * @return reference to the value or a reference to an absent value
					 *
					 * \~
					 */
					const Value & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения значения по пути
					 *
					 * @details Недостающие звенья пути заводятся по дороге. Рост вместимого по
					 * номеру ограждён пределом: обращение по номеру, превышающему предел,
					 * выдаёт отбросное значение, а не заводит вместимое затребованной длины
					 *
					 * @param path путь к значению
					 * @return     ссылка на заведённое значение
					 *
					 * \~english
					 * @brief Method of the creation of a value by a path
					 * @details The missing links of a path are created along the way. The growth of a container by
					 * a number is guarded by a limit: an access by a number exceeding the limit
					 * issues a scrap value rather than creating a container of the demanded length
					 * @param path path to the value
					 * @return reference to the created value
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор извлечения значения поля отображения по имени
					 *
					 * @param name имя поля отображения
					 * @return     ссылка на значение поля отображения
					 *
					 * \~english
					 * @brief Operator of the extraction of a value of a field of a mapping by a name
					 * @param name name of the field of the mapping
					 * @return reference to the value of the field of the mapping
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор заведения значения поля отображения по имени
					 *
					 * @param name имя поля отображения
					 * @return     ссылка на значение поля отображения
					 *
					 * \~english
					 * @brief Operator of the creation of a value of a field of a mapping by a name
					 * @param name name of the field of the mapping
					 * @return reference to the value of the field of the mapping
					 *
					 * \~
					 */
					Value & operator [] (const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Оператор извлечения значения вместимого по номеру
					 *
					 * @param index номер значения вместимого
					 * @return      ссылка на значение вместимого
					 *
					 * \~english
					 * @brief Operator of the extraction of a value of a container by a number
					 * @param index number of the value of the container
					 * @return reference to the value of the container
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор заведения значения вместимого по номеру
					 *
					 * @param index номер значения вместимого
					 * @return      ссылка на значение вместимого
					 *
					 * \~english
					 * @brief Operator of the creation of a value of a container by a number
					 * @param index number of the value of the container
					 * @return reference to the value of the container
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
					 * @brief Method of the appending of a value to the end of a sequence
					 * @param value value being appended
					 * @return sign of the success of the appending
					 *
					 * \~
					 */
					[[nodiscard]] bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления поля в отображение
					 *
					 * @details Занятое имя перезаписывается на прежнем месте, а не переносится
					 * в конец: порядок полей есть часть записи, и перенос менял бы её
					 *
					 * @param name  имя поля отображения
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a field into a mapping
					 * @details An occupied name is overwritten in its former place rather than transferred
					 * to the end: the order of the fields is a part of the record, and a transfer would change it
					 * @param name name of the field of the mapping
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					[[nodiscard]] bool insert(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления поля в отображение с именем любого вида
					 *
					 * @details Именем поля запись ABC дозволяет всякое НЕВМЕСТИМОЕ значение, а не
					 * одну лишь строку: целое именем и короче, и сличается быстрее. Вместимое
					 * же именем отвергается - розыск по такому имени требовал бы сличения
					 * поддеревьев, и цена его несоразмерна получаемому
					 *
					 * @note Занятое имя разыскивается сличением ЗНАЧЕНИЙ, а не видов хранения:
					 * `UINT64(42)` при пересборке сужается до `UINT8`, и сличение видами
					 * объявило бы равные имена разными
					 *
					 * @param name  имя поля отображения
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a field into a mapping with a name of any kind
					 * @details The record of ABC allows any NON-CONTAINER value as the name of a field rather than
					 * a string alone: an integer is both shorter as a name and faster to compare. A container
					 * as a name is rejected: a lookup by such a name would demand a comparison of the subtrees,
					 * and its price is disproportionate to what is gained
					 * @note An occupied name is looked up by a comparison of the VALUES rather than of the kinds
					 * of the storage: `UINT64(42)` narrows to `UINT8` upon a reassembling, and a comparison by
					 * the kinds would declare equal names different
					 * @param name name of the field of the mapping
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					[[nodiscard]] bool insert(const Value & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления поля в отображение с именем строковым литералом
					 *
					 * @details Работа эта заведена РАЗРЕШЕНИЯ РАДИ: обращение `const char *` в
					 * `Value` и в `std::string` стоит собирателю одинаково, и вызов с литералом
					 * без неё двусмыслен. Ср. тот же случай у конструктора `Value(const char *)`
					 *
					 * @param name  имя поля отображения
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a field into a mapping with a string literal as a name
					 * @details This work is introduced FOR THE SAKE OF THE RESOLUTION: the conversion of
					 * `const char *` into `Value` and into `std::string` costs the compiler the same, and a call
					 * with a literal is ambiguous without it. Compare the same case at the constructor `Value(const char *)`
					 * @param name name of the field of the mapping
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					[[nodiscard]] bool insert(const char * name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления поля отображения по имени
					 *
					 * @param name имя поля отображения
					 * @return     признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a field of a mapping by a name
					 * @param name name of the field of the mapping
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					[[nodiscard]] bool erase(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления значения вместимого по номеру
					 *
					 * @param index номер значения вместимого
					 * @return      признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a value of a container by a number
					 * @param index number of the value of the container
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					[[nodiscard]] bool erase(const size_t index) noexcept;
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
					 * @param result value being extracted
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool value(bool & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения числа видом целого со знаком
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 *
					 * @details Договор извлечения есть ПРИВЕДЕНИЕ, а не отказ при несоответствии.
					 * Отказом отвечается лишь значение, числом не являющееся вовсе, да число
					 * неограниченной ширины. Правил приведения ДВА, и разводит их наличие у
					 * числа дробной части
					 *
					 * @note Число, дробной части НЕ имеющее и лежащее в отрезке от `-2^63` до
					 * `2^64`, переносится МЛАДШИМИ РАЗРЯДАМИ - тем же правилом, каким переносится
					 * целое между видами. Оттого `-1` целым и `-1.0` дробным дают один и тот же
					 * итог, а `2^63` дробным видом целого со знаком даёт `-2^63`, а не предел
					 * его. Правило это и обеспечивает независимость итога от НАПИСАНИЯ числа.
					 * Закреплено `CodecAbcValue.WholeRealMatchesInteger` и
					 * `CodecAbcDocument.LayersAgreeOnNumberEdges`
					 *
					 * @note Число, дробную часть имеющее либо в отрезок тот не попадающее,
					 * округляется (`300.5` даёт `301`) и прижимается к краю затребованного вида
					 * (`1e30` даёт предел). Прижатие стоит ПОСЛЕ округления нарочно: округлить
					 * прежде значило бы приводить к целому виду число, ему не отвечающее
					 *
					 * @warning Различать точное от утраченного условием «дробной части нет» выше
					 * `2^53` НЕЛЬЗЯ: там всякое дробное уже целое, и условие истинно всегда.
					 * Извлечение от того не страдает - оно выдаёт то, что в значении лежит, - но
					 * поверять им ЦЕЛОСТЬ переноса нельзя. Указано Василием 31.08.2026 по случаю
					 * его кодеков, где такая поверка стояла и теряла разряды признаком успеха
					 *
					 * @note Виды `EXTENDED` и `DECIMAL` отвергаются НАМЕРЕННО: величина их в
					 * машинный вид помещаться не обязана вовсе, и работа у них своя - `digits`.
					 * Разделение полное: `value` берёт целое и дробное и отвергает эти два,
					 * `digits` берёт эти два и отвергает всё прочее
					 * \~english
					 * @brief Method of the extraction of a number by the kind of an integer with a sign
					 * @param result value being extracted
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool value(int64_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения числа видом целого без знака
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 *
					 * @details Договор извлечения есть ПРИВЕДЕНИЕ, а не отказ при несоответствии.
					 * Отказом отвечается лишь значение, числом не являющееся вовсе, да число
					 * неограниченной ширины. Правил приведения ДВА, и разводит их наличие у
					 * числа дробной части
					 *
					 * @note Число, дробной части НЕ имеющее и лежащее в отрезке от `-2^63` до
					 * `2^64`, переносится МЛАДШИМИ РАЗРЯДАМИ - тем же правилом, каким переносится
					 * целое между видами. Оттого `-1` целым и `-1.0` дробным дают один и тот же
					 * итог, а `2^63` дробным видом целого со знаком даёт `-2^63`, а не предел
					 * его. Правило это и обеспечивает независимость итога от НАПИСАНИЯ числа.
					 * Закреплено `CodecAbcValue.WholeRealMatchesInteger` и
					 * `CodecAbcDocument.LayersAgreeOnNumberEdges`
					 *
					 * @note Число, дробную часть имеющее либо в отрезок тот не попадающее,
					 * округляется (`300.5` даёт `301`) и прижимается к краю затребованного вида
					 * (`1e30` даёт предел). Прижатие стоит ПОСЛЕ округления нарочно: округлить
					 * прежде значило бы приводить к целому виду число, ему не отвечающее
					 *
					 * @warning Различать точное от утраченного условием «дробной части нет» выше
					 * `2^53` НЕЛЬЗЯ: там всякое дробное уже целое, и условие истинно всегда.
					 * Извлечение от того не страдает - оно выдаёт то, что в значении лежит, - но
					 * поверять им ЦЕЛОСТЬ переноса нельзя. Указано Василием 31.08.2026 по случаю
					 * его кодеков, где такая поверка стояла и теряла разряды признаком успеха
					 *
					 * @note Виды `EXTENDED` и `DECIMAL` отвергаются НАМЕРЕННО: величина их в
					 * машинный вид помещаться не обязана вовсе, и работа у них своя - `digits`.
					 * Разделение полное: `value` берёт целое и дробное и отвергает эти два,
					 * `digits` берёт эти два и отвергает всё прочее
					 * \~english
					 * @brief Method of the extraction of a number by the kind of an integer without a sign
					 * @param result value being extracted
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool value(uint64_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения числа видом дробного
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 *
					 * @details Договор извлечения есть ПРИВЕДЕНИЕ, а не отказ при несоответствии.
					 * Отказом отвечается лишь значение, числом не являющееся вовсе, да число
					 * неограниченной ширины. Правил приведения ДВА, и разводит их наличие у
					 * числа дробной части
					 *
					 * @note Число, дробной части НЕ имеющее и лежащее в отрезке от `-2^63` до
					 * `2^64`, переносится МЛАДШИМИ РАЗРЯДАМИ - тем же правилом, каким переносится
					 * целое между видами. Оттого `-1` целым и `-1.0` дробным дают один и тот же
					 * итог, а `2^63` дробным видом целого со знаком даёт `-2^63`, а не предел
					 * его. Правило это и обеспечивает независимость итога от НАПИСАНИЯ числа.
					 * Закреплено `CodecAbcValue.WholeRealMatchesInteger` и
					 * `CodecAbcDocument.LayersAgreeOnNumberEdges`
					 *
					 * @note Число, дробную часть имеющее либо в отрезок тот не попадающее,
					 * округляется (`300.5` даёт `301`) и прижимается к краю затребованного вида
					 * (`1e30` даёт предел). Прижатие стоит ПОСЛЕ округления нарочно: округлить
					 * прежде значило бы приводить к целому виду число, ему не отвечающее
					 *
					 * @warning Различать точное от утраченного условием «дробной части нет» выше
					 * `2^53` НЕЛЬЗЯ: там всякое дробное уже целое, и условие истинно всегда.
					 * Извлечение от того не страдает - оно выдаёт то, что в значении лежит, - но
					 * поверять им ЦЕЛОСТЬ переноса нельзя. Указано Василием 31.08.2026 по случаю
					 * его кодеков, где такая поверка стояла и теряла разряды признаком успеха
					 *
					 * @note Виды `EXTENDED` и `DECIMAL` отвергаются НАМЕРЕННО: величина их в
					 * машинный вид помещаться не обязана вовсе, и работа у них своя - `digits`.
					 * Разделение полное: `value` берёт целое и дробное и отвергает эти два,
					 * `digits` берёт эти два и отвергает всё прочее
					 * \~english
					 * @brief Method of the extraction of a number by the kind of a fractional one
					 * @param result value being extracted
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого значения строкой
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a value by a string
					 * @param result value being extracted
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					[[nodiscard]] bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перенесения значения из дерева документа
					 *
					 * @note Перенесение ведётся без возвратности: дерево на десятки тысяч
					 * уровней иначе сорвало бы стек
					 *
					 * @param value переносимое значение дерева документа
					 *
					 * \~english
					 * @brief Method of the transfer of a value from a tree of a document
					 * @note The transfer is conducted without a recursion: a tree of tens of thousands of
					 * levels would otherwise break the stack
					 * @param value value of a tree of a document being transferred
					 *
					 * \~
					 */
					void absorb(const Document::value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод укладки значения в собираемую запись
					 *
					 * @param writer сборщик бинарной записи
					 * @return       признак успешности укладки
					 *
					 * \~english
					 * @brief Method of the laying of a value into an assembled record
					 * @param writer assembler of a binary record
					 * @return sign of the success of the laying
					 *
					 * \~
					 */
					[[nodiscard]] bool compose(writer_t & writer) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора записи во владеющее значение
					 *
					 * @param buffer буфер разбираемой записи
					 * @param size   размер разбираемой записи в октетах
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a record into an owning value
					 * @param buffer buffer of the record being parsed
					 * @param size size of the record being parsed in octets
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool parse(const void * buffer, const size_t size) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора записи затребованными настройками разбора
					 *
					 * @details Настройки нужны там, где запись собрана укладом, какой разбор
					 * умолчанием отвергает: повтор имени поля отображения отвергается умолчанием,
					 * и без этого вида работы владеющее значение такой записи не прочло бы вовсе
					 *
					 * @param buffer   буфер разбираемой записи
					 * @param size     размер разбираемой записи
					 * @param settings настройки разбора записи
					 * @return         признак успешного разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a record by the demanded settings of the parsing
					 * @details The settings are needed where a record is assembled by an order which the parsing
					 * refuses by default
					 * @param buffer buffer of the record being parsed
					 * @param size size of the record being parsed
					 * @param settings settings of the parsing of the record
					 * @return flag of the successful parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool parse(const void * buffer, const size_t size, const reader_t::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из владеющего значения
					 *
					 * @warning Собранная запись равна разобранной по СМЫСЛУ, но не по октетам:
					 * значение хранит состав вместимого, а не вид записи его, и вместимое
					 * неопределённой длины укладывается обратно определённым. Так, отображение
					 * `BF 42 D0 B0 01 42 D0 B1 02 DF` (10 окт.) выходит записью в 9 октетов
					 *
					 * @warning Подпись контейнера считается по ОКТЕТАМ: запись, прочитанная во
					 * владеющее значение и уложенная обратно, подписи своей более не отвечает
					 *
					 * @return собранная запись
					 *
					 * \~english
					 * @brief Method of the assembling of a record from an owning value
					 * @warning The assembled record equals the parsed one by the MEANING but not by the octets:
					 * a container of an indefinite length is laid back as a definite one
					 * @warning The signature of a container is computed over the OCTETS: a record read into
					 * an owning value and laid back no longer agrees with its signature
					 * @return assembled record
					 *
					 * \~
					 */
					vector <uint8_t> dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из владеющего значения с поводом отказа
					 *
					 * @details Вид этот заведён ради повода: `dump()` отвечает на отказ пустой
					 * записью, а по ней не узнать, что именно не легло - непригодный узел,
					 * повторяющееся имя поля либо превышенная глубина
					 *
					 * @param result собранная запись
					 * @param error  код отказа, если сборка не удалась
					 * @return       признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembling of a record from an owning value with a reason of the refusal
					 * @details This kind is introduced for the sake of the reason: `dump()` answers a refusal
					 * by an empty record, and by it one cannot learn what exactly has not been laid
					 * @param result assembled record
					 * @param error code of the refusal if the assembling has not succeeded
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool dump(vector <uint8_t> & result, error_t & error) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи из владеющего значения затребованными настройками
					 *
					 * @details Вид этот заведён СИММЕТРИИ РАДИ с `parse(buffer, size, settings)`:
					 * разбор настройки принимал, а сборка их не принимала вовсе, и строгий вид
					 * записи, порог укладки ссылкой, объявление размаха и запрет повторяющихся
					 * имён через `dump` были НЕДОСТИЖИМЫ. Дорога через `compose(writer)` их даёт,
					 * но требует завести сборщик самому
					 *
					 * @warning Строгий вид тут не прихоть: подпись контейнера считается по
					 * ОКТЕТАМ, и запись, собранная без строгого вида, октет в октет не
					 * повторяется. Кому нужна повторимость - тому нужны эти настройки
					 *
					 * @param result   собранная запись
					 * @param error    код отказа, если сборка не удалась
					 * @param settings настройки сборки записи
					 * @return         признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembling of a record from an owning value by the demanded settings
					 * @details This kind is introduced FOR THE SAKE OF THE SYMMETRY with `parse(buffer, size, settings)`
					 * @param result assembled record
					 * @param error code of the refusal if the assembling has not succeeded
					 * @param settings settings of the assembling of the record
					 * @return sign of the success of the assembling
					 *
					 * \~
					 */
					[[nodiscard]] bool dump(vector <uint8_t> & result, error_t & error,
					 const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса владеющего значения в дерево документа
					 *
					 * @details Работа эта заведена ЕДИНООБРАЗИЯ РАДИ: у прочих кодеков она
					 * зовётся так же, и потребителю, пишущему поверх нескольких кодеков,
					 * не приходится помнить, у какого из них перенос зовётся иначе
					 *
					 * @warning **Стоит она полного круга через октеты** и правкою на месте
					 * НЕ является: значение укладывается в запись, а запись разбирается в
					 * дерево наново. У прочих кодеков дерево умеет правку, и перенос кладёт
					 * себя её вызовами; у ABC правки дерева нет и не будет - она завела бы
					 * второй путь укладки записи, мимо строгого вида, порога укладки ссылкой,
					 * порога объявления размаха и подбора метода сжатия
					 *
					 * @note Дерево поданного документа перед переносом очищается: перенос
					 * заменяет его целиком, а не доливает к нему
					 *
					 * @note Прямая дорога у ABC иная и дешевле: собрать значение сборкою
					 * `builder_t`, уложить его `compose(writer)` либо `dump()`, внести в
					 * контейнер `editor_t::append` с последующей фиксацией
					 *
					 * @param document дерево документа, куда переносится значение
					 * @return         признак успешности переноса
					 *
					 *
					 * @details **Круг этот идёт УМОЛЧАНИЯМИ обеих настроек**: укладка ведётся настройками
					 * сборки по умолчанию, разбор - настройками разбора по умолчанию, и подать свои через
					 * эту дверь нельзя. Отсюда следствие, видимое наружу: значение, несущее строку с
					 * негодной последовательностью UTF-8, переносом ОТВЕРГАЕТСЯ - поверка кодировки у
					 * сборки объявлена умолчанием. Замерено 01.09.2026 и закреплено проверкой
					 * `CodecAbcValue.GraftUsesDefaultSettings`
					 *
					 * @note Дорога настраиваемая складывается из тех же двух половин, взятых порознь:
					 * `dump(result, error, settings)` со своими настройками сборки и `Document::parse(...,
					 * settings)` со своими настройками разбора. Ею и переносят то, что умолчаниям не
					 * отвечает
					 *
					 * \~english
					 * @brief Method of the transfer of an owning value into a tree of a document
					 * @details This work is introduced FOR THE SAKE OF THE UNIFORMITY: at the other codecs it is
					 * called the same, and a consumer writing over several codecs does not have to remember
					 * at which of them the transfer is called otherwise
					 * @warning **It costs a full circle through the octets** and is NOT an editing in place:
					 * the value is laid into a record, and the record is parsed into a tree anew. At the other
					 * codecs the tree is capable of editing, and the transfer lays itself by its calls; at ABC
					 * there is no editing of the tree and there will not be — it would introduce a second path
					 * of the laying of a record, past the strict kind, the threshold of the laying by a reference,
					 * the threshold of the declaration of the span and the selection of the method of the compression
					 * @note The tree of the submitted document is cleared before the transfer: the transfer replaces
					 * it as a whole rather than adds to it
					 * @note The direct road at ABC is different and cheaper: to assemble the value by the assembling
					 * `builder_t`, to lay it by `compose(writer)` or `dump()`, to bring it into a container by
					 * `editor_t::append` with a subsequent commit
					 * @param document tree of the document the value is transferred into
					 * @return sign of the success of the transfer
					 *
					 * \~
					 */
					[[nodiscard]] bool graft(Document & document) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор сличения значений
					 *
					 * @note Сличение отображений порядка полей не учитывает, а сличение
					 * перечней - учитывает
					 *
					 * @param value сличаемое значение
					 * @return      признак равенства значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values
					 * @note The comparison of the mappings does not take the order of the fields into account, while the comparison
					 * of the sequences does
					 * @param value value being compared
					 * @return sign of the equality of the values
					 *
					 * \~
					 */
					bool operator == (const Value & value) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор сличения значений на неравенство
					 *
					 * @param value сличаемое значение
					 * @return      признак неравенства значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values for an inequality
					 * @param value value being compared
					 * @return sign of the inequality of the values
					 *
					 * \~
					 */
					bool operator != (const Value & value) const noexcept;
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
					Value() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param kind вид узла заводимого значения
					 *
					 * \~english
					 * @brief Constructor
					 * @param kind kind of the node of the value being created
					 *
					 * \~
					 */
					explicit Value(const kind_t kind) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value заводимое логическое значение
					 *
					 * \~english
					 * @brief Constructor
					 * @param value logical value being created
					 *
					 * \~
					 */
					Value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value заводимое целое со знаком
					 *
					 * \~english
					 * @brief Constructor
					 * @param value integer with a sign being created
					 *
					 * \~
					 */
					Value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value заводимое целое без знака
					 *
					 * \~english
					 * @brief Constructor
					 * @param value integer without a sign being created
					 *
					 * \~
					 */
					Value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value заводимое дробное число
					 *
					 * \~english
					 * @brief Constructor
					 * @param value fractional number being created
					 *
					 * \~
					 */
					Value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value заводимая строка
					 *
					 * \~english
					 * @brief Constructor
					 * @param value string being created
					 *
					 * \~
					 */
					Value(const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @details Конструктор этот необходим: без него `Value("текст")` завёл бы
					 * значение логическое, а не строковое. Обращение указателя в логическое
					 * значение стандартное, а в строку - пользовательское, и стандартное
					 * побеждает молча
					 *
					 * @param value заводимая строка
					 *
					 * \~english
					 * @brief Constructor
					 * @details This constructor is necessary: without it `Value("text")` would create
					 * a logical value rather than a string one. The conversion of a pointer into a logical
					 * value is a standard one, while into a string it is a user-defined one, and the standard one
					 * wins silently
					 * @param value string being created
					 *
					 * \~
					 */
					Value(const char * value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор от указателя запрещён
					 *
					 * @details Конструктор от истинности неявен намеренно - `insert("к", true)`
					 * без него не собрать, - и всякий указатель проходил бы в него стандартным
					 * преобразованием, молча обращаясь в ИСТИНУ. Ловушка эта тем острее, что
					 * всякий иной разряд кодека берёт журнал конструктором, и `value_t v(log)`
					 * собиралось бы молча. Строковый литерал сюда не попадает: `Value(const char *)`
					 * стоит рядом и как не шаблонный предпочитается. Запрет закреплён
					 * проверкой `CodecAbcValue.PointerConstructionForbidden`: утверждение
					 * это ВРЕМЕНИ СБОРКИ, и прогоном его не поверить - удавшаяся сборка от
					 * указателя не отказывает, а даёт истину
					 *
					 * \~english
					 * @brief The constructor from a pointer is forbidden
					 * @details The constructor from a boolean is implicit deliberately — `insert("k", true)`
					 * cannot be assembled without it — and every pointer would pass into it by a standard
					 * conversion, silently turning into TRUE
					 *
					 * \~
					 */
					template <typename T>
					Value(T *) = delete;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param value переносимое значение дерева документа
					 *
					 * \~english
					 * @brief Constructor
					 * @param value value of a tree of a document being transferred
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
					 * @brief Оператор присваивания копированием
					 *
					 * @param value копируемое значение
					 * @return      ссылка на присвоенное значение
					 *
					 * \~english
					 * @brief Copy assignment operator
					 * @param value value being copied
					 * @return reference to the assigned value
					 *
					 * \~
					 */
					Value & operator = (const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Оператор присваивания переносом
					 *
					 * @param value переносимое значение
					 * @return      ссылка на присвоенное значение
					 *
					 * \~english
					 * @brief Move assignment operator
					 * @param value value being moved
					 * @return reference to the assigned value
					 *
					 * \~
					 */
					Value & operator = (Value && value) noexcept;
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
					~Value() noexcept;
			} value_t;

			/**
			 * \~russian
			 * @brief Класс потоковой сборки владеющего значения ABC
			 *
			 * @details Сборка ведёт вместилища сама и избавляет потребителя от обхода
			 * собираемого дерева: открытое вместилище помнится путём, а не ссылкою.
			 * Договор тот же, что у сборок прочих кодеков, - разнятся лишь имена
			 * вместилищ, `map` и `array` вместо `table` и `array` у TOML
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 * лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 * равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 * тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 * четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 * ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 * свободно
			 *
			 * \~english
			 * @brief Class of the streaming assembly of an owning value of ABC
			 * @details The assembly leads the containers itself and relieves the consumer of the traversal
			 * of the tree being assembled: an opened container is remembered by a path, not by a reference.
			 * The contract is the same as that of the assemblies of the other containers
			 *
			 * \~
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 * holds a lock, and one must not judge the others by it. A measurement of 25.08.2026, one `Fetcher`
			 * on four threads: thirteen reports of TSan and nineteen records of four hundred read wrongly,
			 * silently. An own object per thread over a SHARED source of the reading: zero reports,
			 * zero divergences
			 *
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
					 * @brief Path to the container opened by the assembly
					 * @details The path is stored by numbers, not by pointers
					 *
					 * \~
					 */
					vector <size_t> _path;
				private:
					// Имя поля отображения, сборкой назначенное
					Value _key;
				private:
					// Объект работы с логами
					const log_t * _log;
				private:
					// Признак назначенного имени поля отображения
					bool _keyed;
				private:
					/**
					 * \~russian
					 * Признак завершённости сборки
					 *
					 * @details Признак нужен затем, что закрытие корневого вместилища путь
					 * опустошает, а опустошённый путь неотличим от несобранного значения
					 *
					 * \~english
					 * @brief Flag of the completeness of the assembly
					 * @details The flag is needed because the closing of the root container empties the path
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
					 * @param index номер занесённого значения во вместилище
					 * @return      признак успешности занесения
					 *
					 * \~english
					 * @brief Method of the depositing of an assembled value into the container
					 * @param value value being deposited
					 * @param index number of the deposited value in the container
					 * @return sign of the successful depositing
					 *
					 * \~
					 */
					[[nodiscard]] bool deposit(Value && value, size_t & index) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия вместилища затребованного вида
					 *
					 * @param value открываемое вместилище
					 * @return      признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a container of the requested kind
					 * @param value container being opened
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool expand(Value && value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия отображения
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a mapping
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool map() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a list of the values
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool array() noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого вместилища
					 *
					 * @return признак успешности закрытия
					 *
					 * \~english
					 * @brief Method of the closing of the opened container
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
					 * @param name назначаемое имя поля отображения
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a field of a mapping
					 * @param name name of the field of the mapping being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения имени поля отображения любого вида
					 *
					 * @details Именем поля запись ABC дозволяет всякое НЕВМЕСТИМОЕ значение, а не
					 * одну лишь строку. Без этой работы потоковая сборка не выражала бы часть
					 * записей, какие сам же кодек и читает, и пишет
					 *
					 * @param name назначаемое имя поля отображения
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a field of a mapping of any kind
					 * @details The record of ABC allows any NON-CONTAINER value as the name of a field rather than
					 * a string alone. Without this work the streaming assembling would not express a part of
					 * the records which the codec itself both reads and writes
					 * @param name name of the field of the mapping being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool key(const Value & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения имени поля отображения строковым литералом
					 *
					 * @details Работа эта заведена РАЗРЕШЕНИЯ РАДИ: вызов с литералом без неё
					 * двусмыслен. Ср. `Value::insert(const char *, const Value &)`
					 *
					 * @param name назначаемое имя поля отображения
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a field of a mapping by a string literal
					 * @details This work is introduced FOR THE SAKE OF THE RESOLUTION: a call with a literal is
					 * ambiguous without it. Compare `Value::insert(const char *, const Value &)`
					 * @param name name of the field of the mapping being assigned
					 * @return sign of the success of the assignment
					 *
					 * \~
					 */
					bool key(const char * name) noexcept;
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
					bool nul() noexcept;
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
					 * @brief Метод записи целого значения со знаком
					 *
					 * @param value записываемое целое значение со знаком
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a whole value with a sign
					 * @param value whole value with a sign being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого значения без знака
					 *
					 * @param value записываемое целое значение без знака
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a whole value without a sign
					 * @param value whole value without a sign being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дробного значения
					 *
					 * @param value записываемое дробное значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a fractional value
					 * @param value fractional value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value записываемое строковое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value string value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value) noexcept;
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
					 * @brief Method of the reset of the assembly to the initial state
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия собранного значения
					 *
					 * @details Сборка после изъятия сбрасывается в исходное состояние и
					 * годна к сборке следующего значения
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the withdrawal of the assembled value
					 * @details The assembly is reset to the initial state after the withdrawal
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
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log object for working with logs
					 *
					 * \~
					 */
					explicit Builder(const log_t * log) noexcept;
			} builder_t;
		};
	};
};

#endif // __AWH_CODEC_ABC_VALUE__
