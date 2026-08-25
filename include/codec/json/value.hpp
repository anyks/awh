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
 * @brief Заголовочный файл владеющего значения JSON — самостоятельный тип данных,
 *        хранящий дерево значений собственной памятью, собираемый из значений языка и
 *        пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of JSON — a standalone data type
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
#ifndef __AWH_CODEC_JSON_VALUE__
#define __AWH_CODEC_JSON_VALUE__

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
			 * @brief Владеющее значение JSON
			 *
			 * @details Тип этот стоит **над** документом, а не вместо него. Документ
			 * разбирает текст и хранит дерево плоским массивом узлов, отчего ссылка на
			 * узел `Document::Value` владельцем не является и документ пережить не может
			 *
			 * @details Владеющее значение решает ровно ту задачу, какую ссылка решать не
			 * вправе: оно держит своё поддерево собственной памятью и потому складывается
			 * из значений языка, копируется, кладётся во вместилища языка, передаётся
			 * вглубь и **отдаётся наружу итогом метода**
			 *
			 * @details Оформления исходного текста значение не удерживает: расстановка
			 * пробелов и вид записи чисел принадлежат документу, и правка чужого текста с
			 * сохранением его неприкосновенности остаётся за документом. Это два разных
			 * употребления, и смешивать их не следует
			 *
			 * @details Частность наречия JSON, сохранённая значением, - вид числа: узкий
			 * вид опознаётся при заведении значения и переживает передачу наружу, отчего
			 * число, заведённое `uint64_t`, числом со знаком не станет
			 *
			 * @warning **Снимать надлежит поддерево, а не документ целиком.** Владение
			 * стоит памяти: документ хранится сплошным перечнем узлов и одним хранилищем
			 * знаков, обходясь одним выделением памяти, тогда как значение держит у
			 * всякого узла свою строку и свои вместилища. Замер: снятие ответа службы в
			 * две сотни октетов берёт пять выделений и полтора килобайта, а снятие
			 * выгрузки в шестнадцать мегабайтов - полмиллиона выделений и сто тридцать
			 * мегабайтов, вчетверо больше самого текста. Плата эта берётся за право
			 * пережить документ, и брать её за поддерево, какое наружу не уходит, незачем
			 *
			 * @note Облик этого типа общий у всех кодеков рамки: имена действий и правила
			 *       их поведения одинаковы у JSON, XML, YAML, TOML и INI, а частности
			 *       наречий прибавляются полями сверху
			 *
			 * \~english
			 * @brief Owning value of JSON
			 * @details This type stands **above** the document rather than instead of it. The document
			 * parses a text and stores the tree as a flat array of the nodes, whereby a reference to
			 * a node `Document::Value` is not an owner and cannot outlive the document
			 * @details The owning value solves exactly the task which a reference has no
			 * right to solve: it holds its subtree by its own memory and therefore is assembled
			 * from the values of the language, is copied, is placed into the containers of the language, is passed
			 * inwards and **is given away outwards as the result of a method**
			 * @details The value does not retain the formatting of the source text: the placement
			 * of the spaces and the kind of the record of the numbers belong to the document, and the editing of a foreign text with
			 * the preservation of its intactness remains with the document. These are two different
			 * usages and they should not be mixed
			 * @details The particularity of the JSON dialect preserved by the value is the kind of a number: the narrow
			 * kind is recognized at the creation of the value and outlives the passing outwards, whereby
			 * a number created as `uint64_t` will not become a signed number
			 * @warning **A subtree should be taken rather than a whole document.** The ownership
			 * costs memory: a document is stored by a continuous list of the nodes and one storage
			 * of the characters, managing with one allocation of the memory, whereas a value holds
			 * its own string and its own containers for every node. A measurement: the taking of a service
			 * response of two hundred octets costs five allocations and a kilobyte and a half, while the taking
			 * of a dump of sixteen megabytes — half a million allocations and one hundred and thirty
			 * megabytes, four times more than the text itself. This payment is taken for the right
			 * to outlive the document, and there is no point in taking it for a subtree which does not go outwards
			 * @note The shape of this type is common for all the codecs of the framework: the names of the actions and the rules
			 *       of their behaviour are identical for JSON, XML, YAML, TOML and INI, while the particularities
			 *       of the dialects are added by the fields on top
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				private:
					/**
					 * \~russian
					 * @brief Хранилище числа родного вида
					 *
					 * @details Число хранится родным видом, а не записью: разбор записи на
					 * всякое извлечение стоил бы дороже самого извлечения
					 *
					 * \~english
					 * @brief Storage of a number of a native kind
					 * @details A number is stored by a native kind rather than by a record: the parsing of the record
					 * at every extraction would cost more than the extraction itself
					 *
					 * \~
					 */
					typedef union Numeric {
						// Логическое значение
						bool flag;
						// Целое число со знаком
						int64_t integer;
						// Целое число без знака
						uint64_t natural;
						// Дробное число
						double real;
					} numeric_t;
				private:
					// Вид хранимого значения
					kind_t _kind;
				private:
					/**
					 * \~russian
					 * Вид числа, опознанный при заведении значения
					 *
					 * @details Вид этот хранится значением, а не берётся у документа:
					 * значение владеющее документ пережить обязано, и вид числа его
					 * переопределяться настройками чужого разбора не должен
					 *
					 * \~english
					 * Kind of the number recognized at the creation of the value
					 * @details This kind is stored by the value rather than taken from the document:
					 * an owning value must outlive the document, and the kind of its number
					 * must not be redefined by the settings of a foreign parsing
					 *
					 * \~
					 */
					type_t _type;
				private:
					// Число либо логическое значение родного вида
					numeric_t _number;
				private:
					/**
					 * \~russian
					 * Содержимое значения строкового и запись числа, в родной вид не вместимого
					 *
					 * @details Поле одно на два употребления намеренно: у строки число не
					 * хранится, у числа расширенного строки не бывает, и разводить их
					 * двумя полями означало бы держать по пустой строке на всякое значение
					 *
					 * \~english
					 * Content of a string value and the record of a number not fitting into a native kind
					 * @details This field is one for the two usages deliberately: a string does not store a number,
					 * an extended number does not have a string, and to separate them
					 * by two fields would mean to keep an empty string for every value
					 *
					 * \~
					 */
					string _text;
				private:
					/**
					 * \~russian
					 * Имена полей объекта
					 *
					 * @details Перечень этот наполняется лишь у объекта: у массива имён нет
					 * вовсе, и хранить пустые строки ему незачем
					 *
					 * \~english
					 * Names of the fields of an object
					 * @details This list is filled only for an object: an array has no names
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
					 * Отображение имён полей объекта на их номера, заводимое по требованию
					 *
					 * @details Мелкие объекты разыскиваются перебором имён, а крупные -
					 * отображением: заведение его обходится дороже сличения немногих имён,
					 * а объектов о немногих полях в обиходе подавляющее большинство. Порог
					 * тот же, каким заводит отображение документ, - речь об одном выборе
					 *
					 * @note Ключом служит САМО ИМЯ, а не вид на него: у документа
					 *       отображение опирается на неподвижное хранилище знаков, здесь же
					 *       имена лежат перечнем, и перевыделение его сдвинуло бы знаки
					 *       коротких имён вместе с самими строками, обратив всякий вид
					 *       висячим
					 *
					 * @note Повтор имени отображение хранит ПЕРВЫМ вхождением: добавление
					 *       поля повтор допускает намеренно, а перебор находит первое, и
					 *       отображению надлежит отвечать тем же
					 *
					 * \~english
					 * Mapping of the names of the fields of an object onto their numbers, built on demand
					 *
					 * \~
					 */
					mutable unordered_map <string, size_t> _index;
				private:
					/**
					 * \~russian
					 * Признак заведённости отображения имён полей объекта
					 *
					 * @note Признак отдельный от пустоты отображения обязателен: у объекта
					 *       об одних лишь повторяющихся именах отображение вышло бы короче
					 *       перечня, и сличение размеров велело бы заводить его снова и
					 *       снова на всяком обращении
					 *
					 * \~english
					 * Flag of the readiness of the mapping of the names of the fields of an object
					 *
					 * \~
					 */
					mutable bool _indexed;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Умолчание стоит прямо в объявлении намеренно: конструкторы копии и
					 *       переноса логгера не принимают, и без умолчания поле у них осталось
					 *       бы неопределённым. Сами они логгер СНИМАЮТ С ИСТОЧНИКА, но лишь
					 *       когда своего у цели ещё нет: настроенная цель своего не отдаёт
					 *
					 * \~english
					 * Object of the keeping of the work log
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * @brief Метод розыска номера поля объекта по имени
					 *
					 * @details Мелкий объект разыскивается перебором имён, крупный -
					 * отображением, заводимым при первом же обращении
					 *
					 * @param  name разыскиваемое имя поля объекта
					 * @return      номер разысканного поля объекта либо признак отсутствия
					 *
					 * \~english
					 * @brief Method of the search of the number of a field of an object by name
					 *
					 * @param  name the searched name of a field of an object
					 * @return      the number of the found field of an object or the flag of absence
					 *
					 * \~
					 */
					size_t lookup(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта заведённого поля объекта в отображении имён
					 *
					 * @note Метод зовётся ПОСЛЕ добавления имени в перечень: номером служит
					 *       место, какое имя в перечне заняло
					 *
					 * @param name имя заведённого поля объекта
					 *
					 * \~english
					 * @brief Method of the accounting of an added field of an object in the mapping of names
					 *
					 * @param name the name of the added field of an object
					 *
					 * \~
					 */
					void indexed(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса отображения имён полей объекта
					 *
					 * @note Сброс обязателен при всяком сдвиге номеров: снятие поля сдвигает
					 *       все следующие за ним, и отображение, пережившее сдвиг, отвечало
					 *       бы номером соседа
					 *
					 * \~english
					 * @brief Method of the reset of the mapping of the names of the fields of an object
					 *
					 * \~
					 */
					void reindex() const noexcept;
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
					 * @details Отказ потока записи разбирается немедля и наверх передаётся
					 * отказом: поток отвергает значение, ничего не записав, а продолжение
					 * записи после отказа оставило бы в тексте имя поля без значения его
					 *
					 * @param writer поток записи, куда ложится значение
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a writing stream
					 * @details A refusal of the writing stream is handled at once and is passed upwards
					 * as a refusal: the stream rejects a value having written nothing, while a continuation
					 * of the writing after a refusal would leave in the text the name of a field without its value
					 * @param writer writing stream where the value is placed
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool compose(writer_t & writer) const noexcept;
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
				private:
					/**
					 * \~russian
					 * @brief Метод извлечения значения неопределённого
					 *
					 * @details Значение это одно на все отказы обращения только для чтения:
					 * выдать ссылку метод обязан, а ссылки на несуществующее не бывает
					 *
					 * @return значение неопределённое
					 *
					 * \~english
					 * @brief Method of the extraction of an undefined value
					 * @details This value is one for all the refusals of a read-only access:
					 * a method is obliged to give away a reference, while a reference to a non-existent thing does not exist
					 * @return undefined value
					 *
					 * \~
					 */
					static const Value & undefined() noexcept;
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
					 * @details Вид этот точен: число выдаёт тот самый вид, каким оно
					 * хранится, - от `INT8` до `DOUBLE`
					 *
					 * @return вид хранения значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the storage of the value
					 * @details This kind is exact: a number issues that very kind by which it
					 * is stored — from `INT8` to `DOUBLE`
					 * @return kind of the storage of the value
					 *
					 * \~
					 */
					type_t type() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки значения на принадлежность к виду
					 *
					 * @details Проверка идёт наложением разрядов, ровно как у ссылки на узел
					 * документа, оттого точный вопрос и сборный стоят одинаково
					 *
					 * @param type вид либо набор видов, на принадлежность к какому
					 *             проверяется значение
					 * @return     признак принадлежности значения к виду
					 *
					 * \~english
					 * @brief Method of the checking of the value for the belonging to a kind
					 * @details The checking goes by an overlaying of the bits, exactly as for a reference to a node
					 * of a document, whereby an exact question and a composite one cost the same
					 * @param type kind or set of the kinds for the belonging to which
					 *             the value is checked
					 * @return flag of the belonging of the value to the kind
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
					 * @brief Метод извлечения строкового содержимого значения
					 *
					 * @return строковое содержимое значения, пусто у прочих значений
					 *
					 * \~english
					 * @brief Method of the extraction of the string content of the value
					 * @return string content of the value, empty for the other values
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения записи числа
					 *
					 * @details Запись собирается тем же потоком записи, каким собирается
					 * текст всего значения: две отдельные записи одного и того же числа
					 * неминуемо разошлись бы видом
					 *
					 * @return запись числа, пусто у прочих значений
					 *
					 * \~english
					 * @brief Method of the extraction of the record of a number
					 * @details The record is assembled by the same writing stream by which the text
					 * of the whole value is assembled: two separate records of one and the same number
					 * would inevitably diverge in their kind
					 * @return record of the number, empty for the other values
					 *
					 * \~
					 */
					string raw() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имени поля объекта по номеру
					 *
					 * @param index номер поля объекта
					 * @return      имя поля объекта, пустое - поля с таким номером нет
					 *
					 * \~english
					 * @brief Method of the extraction of the name of a field of an object by an index
					 * @param index index of the field of the object
					 * @return name of the field of the object, an empty one — there is no field with such index
					 *
					 * \~
					 */
					const string & key(const size_t index) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия поля объекта с указанным именем
					 *
					 * @param name разыскиваемое имя поля объекта
					 * @return     признак наличия поля объекта
					 *
					 * \~english
					 * @brief Method of the check of the presence of a field of an object with the indicated name
					 * @param name name of the field of the object being searched for
					 * @return sign of the presence of the field of the object
					 *
					 * \~
					 */
					bool contains(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению по пути
					 *
					 * @details Путь записывается указателем JSON Pointer, частями,
					 * разделёнными косой чертой: `/response/users/0/id`. Обращение к
					 * отсутствующему пути ничего не заводит и отдаёт значение неопределённое
					 *
					 * @param path путь к разыскиваемому значению
					 * @return     ссылка на разысканное значение
					 *
					 * \~english
					 * @brief Method of the access to a value by a path
					 * @details The path is written as a JSON Pointer, by the parts
					 * separated by a slash: `/response/users/0/id`. An access to an
					 * absent path creates nothing and gives away an undefined value
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
					 * @details Недостающие вместилища пути заводятся объектами, а часть пути
					 * числовая заводит массив. Значение простое, встреченное на пути вместо
					 * вместилища, перерождается вместилищем
					 *
					 * @param path путь к разыскиваемому значению
					 * @return     ссылка на разысканное либо заведённое значение
					 *
					 * \~english
					 * @brief Method of the access to a value by a path with the creation of the missing ones
					 * @details The missing containers of the path are created as objects, while a numeric part
					 * of the path creates an array. A simple value met on the path instead of
					 * a container is reborn as a container
					 * @param path path to the value being searched for
					 * @return reference to the found or created value
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к полю объекта по имени
					 *
					 * @param name имя поля объекта
					 * @return     ссылка на значение поля объекта
					 *
					 * \~english
					 * @brief Method of the access to a field of an object by a name
					 * @param name name of the field of the object
					 * @return reference to the value of the field of the object
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к полю объекта по имени с заведением недостающего
					 *
					 * @details Обращение к отсутствующему имени заводит поле значением
					 * неопределённым, ровно как это делает `nlohmann::json`
					 *
					 * @param name имя поля объекта
					 * @return     ссылка на значение поля объекта
					 *
					 * \~english
					 * @brief Method of the access to a field of an object by a name with the creation of a missing one
					 * @details An access to an absent name creates the field with an undefined
					 * value, exactly as `nlohmann::json` does it
					 * @param name name of the field of the object
					 * @return reference to the value of the field of the object
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
					 * @details Обращение за границу массива растит его до затребованного
					 * номера значениями неопределёнными
					 *
					 * @param index номер значения во вместилище
					 * @return      ссылка на значение вместилища
					 *
					 * \~english
					 * @brief Method of the access to a value of a container by an index with the creation of a missing one
					 * @details An access beyond the boundary of an array grows it up to the demanded
					 * index by the undefined values
					 * @param index index of the value in the container
					 * @return reference to the value of the container
					 *
					 * \~
					 */
					Value & operator [] (const size_t index) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод добавления значения в конец массива
					 *
					 * @details Значение неопределённое перерождается массивом, а значение
					 * простое перерождению не подлежит и добавление отвечает отказом
					 *
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a value to the end of an array
					 * @details An undefined value is reborn as an array, while a simple value
					 * is not subject to the rebirth and the addition responds with a refusal
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки поля объекта
					 *
					 * @details Поле с уже занятым именем перезаписывается на своём месте, а
					 * порядок полей объекта при том сохраняется
					 *
					 * @note Правило это взято у JavaScript, стандарт породившего: присвоение
					 *       уже заведённому полю меняет значение его, а места не меняет.
					 *       Стандарт сам порядка полей не предписывает вовсе, оттого разбор
					 *       текста правилом `duplicate_t::LAST` вправе оставлять поле на
					 *       месте последнего его появления - расхождением это не является
					 *
					 * @param name  имя поля объекта
					 * @param value устанавливаемое значение поля
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a field of an object
					 * @details A field with an already occupied name is overwritten in its place, while
					 * the order of the fields of the object is thereby preserved
					 * @note This rule is taken from JavaScript which gave birth to the standard: an assignment
					 *       to an already created field changes its value but does not change its place.
					 *       The standard itself does not prescribe the order of the fields at all, whereby the parsing
					 *       of a text by the rule `duplicate_t::LAST` is entitled to leave a field at
					 *       the place of the last of its appearances — this is not a divergence
					 * @param name name of the field of the object
					 * @param value value of the field being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления поля объекта с удержанием повтора
					 *
					 * @details Поле кладётся **рядом**, а не поверх: имя, уже заведённое,
					 * поиском не отыскивается вовсе, и объект получает два поля с одним
					 * именем. В том и вся разница с методом `insert`
					 *
					 * @details Метод этот отвечает настройке разбора `duplicate_t::KEEP`:
					 * ею дозволено удержание повторяющихся имён, и без добавления снятое с
					 * такого дерева значение обратно не собиралось бы вовсе - повторы
					 * схлопывались бы в одно поле
					 *
					 * @note Запись такого объекта в текст даёт повторяющиеся имена, а разбор
					 *       записи вернёт их лишь при той же настройке `duplicate_t::KEEP`:
					 *       прочие правила повтор отвергнут либо сведут
					 *
					 * @param name  имя добавляемого поля объекта
					 * @param value добавляемое значение поля
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a field of an object with the retention of a repetition
					 * @details A field is placed **alongside** rather than on top: a name already created
					 * is not searched for at all, and the object gets two fields with one
					 * name. Therein lies the whole difference from the method `insert`
					 * @details This method answers the setting of the parsing `duplicate_t::KEEP`:
					 * by it the retention of the repeating names is allowed, and without the addition a value taken
					 * from such a tree would not be assembled back at all — the repetitions
					 * would collapse into a single field
					 * @note The writing of such an object into a text gives repeating names, while the parsing
					 *       of the record will return them only with the same setting `duplicate_t::KEEP`:
					 *       the other rules will reject or reduce a repetition
					 * @param name name of the field of the object being added
					 * @param value value of the field being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool append(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия поля объекта по имени
					 *
					 * @param name имя снимаемого поля объекта
					 * @return     признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a field of an object by a name
					 * @param name name of the field of the object being removed
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
					 * @details Извлечение сличает само значение с пределами затребованного
					 * вида, а не вид хранения с видом затребованным: значение, хранящее
					 * `INT8`, извлекается и как `double`, и как `uint64_t`. Отказом
					 * извлечение завершается лишь тогда, когда значение числом не является
					 * вовсе
					 *
					 * @note Дробное ОКРУГЛЯЕТСЯ до целого по правилам математики с уводом
					 *       половины от нуля: `1.5` даёт `2`, `-1.5` даёт `-2`, `1.4` даёт `1`.
					 *       Усечение к нулю, стоявшее здесь прежде, расходилось с обиходным счётом
					 *
					 * @note Дробное, чья целая часть лежит за пределами затребованного
					 *       целого вида, выдаётся пределом этого вида, а `NaN` - нулём:
					 *       стандарт зовёт такое приведение неопределённым поведением, а
					 *       неопределённого поведения в кодеке не будет
					 *
					 * @note Целое, не помещающееся в затребованный целый вид, ЗАВОРАЧИВАЕТСЯ
					 *       по правилам самого языка, а не огранивается пределом: приведение
					 *       это языком определено, и выдумывать поверх него нечего. Огранение
					 *       заведено лишь у дробного, и заведено по необходимости - там
					 *       приведение языком не определено вовсе. Правило это одно на все три
					 *       поверхности извлечения: дерево, узел дерева и владеющее значение
					 *
					 * @warning Ни то, ни другое отказом не является: извлечение непомещающегося
					 *          числа выдаёт признак успеха. Потребителю, которому важна
					 *          сохранность величины, надлежит брать вид пошире либо запись
					 *          числа целиком
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a number
					 * @details The extraction compares the value itself with the limits of the demanded
					 * kind rather than the kind of the storage with the demanded kind: a value storing
					 * `INT8` is extracted both as `double` and as `uint64_t`. The extraction ends with a refusal
					 * only when the value is not a number at all
					 * @note A fractional number whose integer part lies beyond the limits of the demanded
					 *       integer kind is issued as the limit of that kind, while `NaN` — as a zero:
					 *       the standard calls such a casting an undefined behaviour, and
					 *       there will be no undefined behaviour in the codec
					 * @note An integer not fitting into the demanded integer kind is WRAPPED by the rules
					 *       of the language itself rather than clamped to the limit: that casting is defined by
					 *       the language, and there is nothing to invent on top of it. The clamping is arranged
					 *       only for a fractional number, and arranged out of necessity — there the casting is
					 *       not defined by the language at all. This rule is one and the same for all the three
					 *       surfaces of the extraction: the tree, a node of the tree and an owning value
					 * @warning Neither of the two is a refusal: the extraction of a number that does not fit
					 *          issues a sign of success. A consumer to whom the preservation of the magnitude
					 *          matters ought to take a wider kind or the record of the number as a whole
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(float & result) const noexcept;
					/**
					 * @copydoc awh::codec::json::Value::value(int8_t &) const
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строкового значения
					 *
					 * @details Извлечение это есть ПРОВЕРКА ВИДА, ровно как извлечение числом:
					 * значение, строкою не являющееся, отвечает отказом, а не записью своей.
					 * Запись значения добывается перезаписью его - `dump()`, - а у числа ещё и
					 * отдельным `raw()`; разведены они намеренно: «дай мне строку» и «дай мне
					 * запись текстом» суть разные вопросы, и видовому наречию отвечать на
					 * первый вторым нечестно
					 *
					 * @note Кодеки XML и INI на том же месте выдают запись, и это НЕ расхождение
					 *       недосмотром: видов у них нет вовсе, всякое значение там и есть текст,
					 *       и отвечать отказом им попросту нечему. Правило это следует из
					 *       устройства наречия, а не из вкуса кодека
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a string value
					 * @details This extraction is a CHECK OF THE KIND, exactly as the extraction by a number:
					 * a value that is not a string answers with a refusal rather than with its own record.
					 * The record of a value is obtained by rewriting it — `dump()` — and for a number also
					 * by the separate `raw()`; they are separated deliberately: "give me a string" and
					 * "give me the record as a text" are different questions, and for a typed notation
					 * to answer the first with the second is dishonest
					 * @note The codecs XML and INI issue the record at the same place, and this is NOT a divergence
					 *       by an oversight: they have no kinds at all, every value there is a text,
					 *       and there is simply nothing for them to refuse. This rule follows from
					 *       the structure of the notation rather than from the taste of the codec
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста JSON во владеющее значение
					 *
					 * @details Разбор ведётся тем же документом, каким он ведётся всегда:
					 * обёртка лишь снимает с него дерево собственной памятью. Оформление
					 * исходного текста при снятии не удерживается
					 *
					 * @note Настройки разбора принимаются доводом: без него ни одна из них - ни
					 * обращение с повторяющимися именами полей, ни пределы вложенности - у
					 * владеющего значения недостижима вовсе, и разбор обрывка вёлся бы одними
					 * умолчаниями
					 *
					 * @param text     разбираемый текст JSON
					 * @param settings настройки разбора текста JSON
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a JSON text into an owning value
					 * @details The parsing is conducted by the same document by which it is always conducted:
					 * the wrapper only takes the tree off it by its own memory. The formatting
					 * of the source text is not retained at the taking
					 * @note The settings of the parsing are taken as an argument: without it not a single one
					 * of them — neither the handling of the repeating names of the fields of an object, nor the
					 * limits of the nesting — would be reachable at all for an owning value, and the parsing
					 * of a fragment would be conducted by the defaults alone
					 * @param text JSON text being parsed
					 * @param settings settings of the parsing of a JSON text
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const document_t::settings_t & settings = document_t::settings_t()) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста JSON из файла
					 *
					 * @param filename адрес разбираемого файла
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a JSON text from a file
					 * @param filename address of the file being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст JSON
					 *
					 * @param format вид оформления собираемого текста
					 * @return       текст JSON
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a JSON text
					 * @param format kind of the formatting of the text being assembled
					 * @return JSON text
					 *
					 * \~
					 */
					string dump(const format_t format = format_t::COMPACT) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст JSON с указанными настройками
					 *
					 * @details Отказ записи хотя бы одного значения отдаёт текст пустым:
					 * текст усечённый, с именем поля без значения его, негоден вовсе, а
					 * выдавать негодное молча кодек не вправе. Случай этот один - число,
					 * стандарту неведомое, при запрете записи таких чисел настройками
					 *
					 * @param settings настройки записи текста
					 * @return         текст JSON, пустой - записать значение не удалось
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a JSON text with the indicated settings
					 * @details A refusal of the writing of at least one value gives away an empty text: a truncated text
					 * with the name of a field without its value is unusable at all, while the codec has no right
					 * to give away an unusable thing silently. There is one such case — a number unknown
					 * to the standard while the writing of such numbers is forbidden by the settings
					 * @param settings settings of the writing of the text
					 * @return JSON text, an empty one — the value could not be written
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения в файл
					 *
					 * @param filename адрес записываемого файла
					 * @param format   вид оформления собираемого текста
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a file
					 * @param filename address of the file being written
					 * @param format kind of the formatting of the text being assembled
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename, const format_t format = format_t::COMPACT) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сличения значений
					 *
					 * @details Сличаются вид и содержимое дерева, а вид хранения числа
					 * сличению не подлежит: число, заведённое `INT8`, и то же число,
					 * заведённое `UINT64`, одинаковы по сути своей
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Method of the comparison of the values
					 * @details The kind and the content of the tree are compared, while the kind of the storage of a number
					 * is not subject to the comparison: a number created as `INT8` and the same number
					 * created as `UINT64` are identical in their essence
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
					 * @brief Конструктор значения указанного вида
					 *
					 * @param kind вид заводимого значения
					 *
					 * \~english
					 * @brief Constructor of a value of the indicated kind
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
					 *
					 * \~english
					 * @brief Constructor of a string value
					 * @param value value being created
					 *
					 * \~
					 */
					Value(const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор строкового значения, поданного строкой языка Си
					 *
					 * @note Конструктор этот заведён отдельным намеренно: без него подача
					 *       строкового литерала ушла бы в заведение значения ЛОГИЧЕСКОГО,
					 *       ибо приведение указателя к `bool` языку ближе приведения к строке
					 *
					 * @param value заводимое значение, ноль - значение пустое
					 *
					 * \~english
					 * @brief Constructor of a string value passed as a C string
					 * @note This constructor is made separate deliberately: without it the passing
					 *       of a string literal would go into the creation of a LOGICAL value,
					 *       for the casting of a pointer to `bool` is closer to the language than the casting to a string
					 * @param value value being created, a zero — an empty value
					 *
					 * \~
					 */
					Value(const char * value) noexcept;
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
					/**
					 * \~russian
					 * @brief Метод установки объекта ведения журнала работы
					 *
					 * @details Привязка поздняя нужна там, где значение заведено копией либо
					 * переносом: логгера они не принимают, и снять его с источника выходит лишь
					 * когда у источника он есть
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
					 * @brief Конструктор копии
					 *
					 * @param value копируемое значение
					 *
					 * \~english
					 * @brief Constructor of the copy
					 *
					 * @param value the copied value
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
			 * @brief Потоковая сборка владеющего значения JSON
			 *
			 * @details Сборщик этот повторяет договор потока записи слово в слово: открыть
			 * объект, открыть массив, записать имя поля, записать значение, закрыть
			 * вместилище, завершить. Оттого «пишу текстом» и «строю дерево» отличаются у
			 * потребителя одной буквой, а не двумя разными договорами
			 *
			 * @details Заведён сборщик ради тех, кто строит документ с нуля полем за полем:
			 * заведение всякого поля через путь `place("/a/b/c")` стоило бы разбора пути на
			 * всякое поле, тогда как сборка потоком стоит одного добавления
			 *
			 * @note Числа подаются сборщику родным своим видом, а не записью: подстановка
			 *       числа текстом заставила бы потребителя собирать запись самому и
			 *       разбирать её обратно
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value of JSON
			 * @details This builder repeats the contract of the writing stream word for word: to open
			 * an object, to open an array, to write the name of a field, to write a value, to close
			 * a container, to finish. Whereby "I write a text" and "I build a tree" differ for
			 * a consumer by one letter rather than by two different contracts
			 * @details The builder is made for those who build a document from nothing field by field:
			 * the creation of every field through the path `place("/a/b/c")` would cost a parsing of the path for
			 * every field, whereas the assembly by a stream costs one addition
			 * @note The numbers are passed to the builder by their native kind rather than by a record: the substitution
			 *       of a number by a text would force a consumer to assemble the record itself and
			 *       to parse it back
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Builder {
				private:
					// Собираемое значение
					value_t _result;
				private:
					/**
					 * \~russian
					 * Стек указаний на открытые вместилища
					 *
					 * @details Стек хранит указания, а не номера: вместилище, уже открытое,
					 * своих детей не переселяет, оттого указание на него живёт до самого
					 * его закрытия
					 *
					 * \~english
					 * Stack of the pointers to the opened containers
					 * @details The stack stores the pointers rather than the indexes: a container already opened
					 * does not relocate its children, whereby a pointer to it lives until
					 * its very closing
					 *
					 * \~
					 */
					vector <value_t *> _nesting;
				private:
					// Имя поля объекта, записанное перед значением
					string _name;
				private:
					// Признак того, что имя поля объекта записано, а значение ещё нет
					bool _keyed;
				private:
					/**
					 * \~russian
					 * Признак того, что поле объекта кладётся рядом, а не поверх
					 *
					 * @note Признак этот принадлежит имени, а не сборщику целиком: он ставится
					 * методом `append` и снимается всяким последующим `key`, - иначе одно
					 * добавление сделало бы добавлениями все поля до конца сборки
					 *
					 * \~english
					 * Sign that a field of an object is placed alongside rather than on top
					 * @note This sign belongs to the name rather than to the builder as a whole: it is set
					 * by the method `append` and is removed by every subsequent `key` — otherwise a single
					 * addition would make additions of all the fields until the end of the assembly
					 *
					 * \~
					 */
					bool _appended;
				private:
					/**
					 * \~russian
					 * @brief Метод помещения собранного значения на своё место
					 *
					 * @param value помещаемое значение
					 * @return      указание на помещённое значение, ноль - помещение не удалось
					 *
					 * \~english
					 * @brief Method of the placement of an assembled value at its place
					 * @param value value being placed
					 * @return pointer to the placed value, a zero — the placement has failed
					 *
					 * \~
					 */
					value_t * attach(const value_t & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия объекта
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of an object
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool object() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия массива
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of an array
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
					 * @brief Method of the closing of an opened container
					 * @return sign of the success of the closing
					 *
					 * \~
					 */
					bool close() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени поля объекта
					 *
					 * @param name записываемое имя поля объекта
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the name of a field of an object
					 * @param name name of the field of the object being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени поля объекта с удержанием повтора
					 *
					 * @details Действует ровно как `key`, с одною разницей: значение,
					 * следом записанное, кладётся **рядом** с полем того же имени, а не
					 * поверх него. Отвечает настройке разбора `duplicate_t::KEEP`, ею
					 * дозволенной
					 *
					 * @note Без этого метода значение, снятое с дерева, разобранного
					 *       правилом `duplicate_t::KEEP`, сборщиком обратно не собиралось
					 *       бы вовсе: повторы схлопывались бы в одно поле
					 *
					 * @param name записываемое имя поля объекта
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a name of a field of an object with the retention of a repetition
					 * @details It acts exactly as `key`, with one difference: the value
					 * written next is placed **alongside** a field of the same name rather than
					 * on top of it. It answers the setting of the parsing `duplicate_t::KEEP`
					 * allowed by it
					 * @note Without this method a value taken from a tree parsed
					 *       by the rule `duplicate_t::KEEP` would not be assembled back by the builder
					 *       at all: the repetitions would collapse into a single field
					 * @param name name of the field of the object being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool append(const string & name) noexcept;
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
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a signed integer number
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа без знака
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of an unsigned integer number
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дробного числа
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a fractional number
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения, поданного строкой языка Си
					 *
					 * @note Метод этот заведён отдельным намеренно: без него подача
					 *       строкового литерала ушла бы в запись значения ЛОГИЧЕСКОГО
					 *
					 * @param value записываемое значение, ноль - значение пустое
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a string value passed as a C string
					 * @note This method is made separate deliberately: without it the passing
					 *       of a string literal would go into the writing of a LOGICAL value
					 * @param value value being written, a zero — an empty value
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const char * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи готового значения
					 *
					 * @details Метод этот вставляет в собираемое дерево целое поддерево,
					 * собранное где-то ещё: сборка сборкою, а готовые куски дерева
					 * переписывать полем за полем незачем
					 *
					 * @param value записываемое значение
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a ready value
					 * @details This method inserts into the tree being assembled a whole subtree
					 * assembled somewhere else: an assembly is an assembly, while there is no point
					 * in rewriting the ready pieces of a tree field by field
					 * @param value value being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const value_t & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения текущей глубины вложенности
					 *
					 * @return количество открытых и ещё не закрытых вместилищ
					 *
					 * \~english
					 * @brief Method of the extraction of the current depth of the nesting
					 * @return number of the opened and not yet closed containers
					 *
					 * \~
					 */
					size_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния сборки
					 *
					 * \~english
					 * @brief Method of the reset of the state of the assembly
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения сборки и изъятия собранного значения
					 *
					 * @details Незакрытые вместилища закрываются сами: сборка потоковая
					 * прервана быть не может, а отказ здесь оставил бы потребителя вовсе
					 * без итога. Сборщик после изъятия готов к новой сборке
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the finishing of the assembly and of the taking away of the assembled value
					 * @details The unclosed containers are closed by themselves: a streaming assembly
					 * cannot be interrupted, while a refusal here would leave a consumer entirely
					 * without a result. After the taking away the builder is ready for a new assembly
					 * @return assembled value
					 *
					 * \~
					 */
					value_t finish() noexcept;
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
					explicit Builder(const log_t * log) noexcept : _keyed(false), _appended(false) {
						// Выполняем установку объекта ведения журнала собираемому значению
						this->_result.setLogger(log);
					}
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

#endif // __AWH_CODEC_JSON_VALUE__
