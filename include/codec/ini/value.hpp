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
 * @brief Заголовочный файл владеющего значения INI — самостоятельный тип данных,
 *        хранящий дерево значений собственной памятью, собираемый из значений языка и
 *        пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of INI — a standalone data type
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
#ifndef __AWH_CODEC_INI_VALUE__
#define __AWH_CODEC_INI_VALUE__

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
		 * @brief Пространство имён контейнера INI
		 *
		 * \~english
		 * @brief INI container namespace
		 *
		 * \~
		 */
		namespace ini {
			/**
			 * \~russian
			 * @brief Владеющее значение INI
			 *
			 * @details Тип этот стоит **над** деревом настроек, а не вместо него. Дерево
			 * разбирает текст, удерживает оформление его - расстановку строк, замечания и
			 * записи имён - и правится по именам, тогда как владеющее значение держит своё
			 * поддерево собственной памятью и оттого собирается из значений языка,
			 * копируется, передаётся внутрь и **отдаётся наружу итогом метода**
			 *
			 * @details Оформления исходного текста тип этот не удерживает: удержание есть
			 * дело дерева, и правка чужого настроечного файла через дерево остаётся
			 * единственным способом сохранить чужой текст целым
			 *
			 * @note Своих видов значения у наречия INI нет вовсе: значение свойства есть
			 *       последовательность знаков, а число ли это либо логическое значение -
			 *       решает извлечение. Оттого простое значение здесь всегда строковое, а
			 *       извлечение числа разбирает запись его при всяком обращении
			 *
			 * @note Глубина наречием ограничена: раздел, подраздел и свойство - вот и всё
			 *       построение. Владеющее значение глубже этого собрать даёт, ибо оно есть
			 *       дерево общего вида, но записать такое дерево текстом настроек нельзя, и
			 *       запись отвечает на него пустым текстом
			 *
			 * @note Выделяется ли подраздел вовсе - решает настройка разбора, и снятое
			 *       значение ей следует. При умолчании `subsection_t::NONE` имя раздела
			 *       берётся целиком, и «[server.tls]» ложится одной парой с именем
			 *       «server.tls»: обращаться к ней надлежит путём «/server.tls/enabled», а
			 *       не «/server/tls/enabled». При `subsection_t::DELIMITED` тот же текст
			 *       ложится двумя уровнями, и верен второй путь. Расхождения тут нет -
			 *       владеющее значение показывает то, что разобрало дерево
			 *
			 * @note Повтор имени свойства ложится перечнем, а свойство, объявленное
			 *       однажды, - простым значением. Обёртка перечнем всегда заставляла бы
			 *       потребителя различать эти два случая при всяком обращении
			 *
			 * \~english
			 * @brief Owning value of INI
			 * @details This type stands **above** the settings tree rather than instead of it. The tree
			 * parses a text, retains its formatting — the arrangement of the lines, the remarks and
			 * the records of the names — and is edited by names, whereas the owning value holds its
			 * subtree by its own memory and therefore is assembled from the values of the language,
			 * is copied, is passed inwards and **is given away outwards as the result of a method**
			 * @details This type does not retain the formatting of the source text: the retention is the business
			 * of the tree, and the editing of a foreign settings file through the tree remains the only way
			 * to keep a foreign text intact
			 * @note The INI dialect has no kinds of a value of its own at all: the value of a property is
			 *       a sequence of characters, while whether this is a number or a logical value — is decided
			 *       by the extraction. Therefore a simple value here is always a string one, while the extraction
			 *       of a number parses its record at every addressing
			 * @note The depth is limited by the dialect: a section, a subsection and a property — that is
			 *       the whole construction. The owning value allows assembling deeper than that, for it is
			 *       a tree of a general kind, but such a tree cannot be written as a settings text, and
			 *       the writing answers to it with an empty text
			 * @note Whether a subsection is singled out at all is decided by the settings of the parsing, and
			 *       the taken value follows them. Under the default `subsection_t::NONE` the name of a section
			 *       is taken entirely, and «[server.tls]» is laid as one pair with the name «server.tls»:
			 *       it ought to be addressed by the path «/server.tls/enabled» rather than «/server/tls/enabled».
			 *       Under `subsection_t::DELIMITED` the same text is laid by two levels, and the second path is
			 *       the correct one. There is no divergence here — the owning value shows what the tree has parsed
			 * @note A repetition of the name of a property is laid as an array, while a property declared
			 *       once — as a simple value. A wrapping into an array always would force the consumer to
			 *       distinguish these two cases at every addressing
			 *
			 * \~
			 * @warning ССЫЛКА, ЭТИМИ ТЕЛАМИ ОТДАВАЕМАЯ, ЖИВЁТ ДО ПЕРВОГО ЗАВЕДЕНИЯ СОСЕДА.
			 * Дети лежат в перемещаемом вместилище, и рост его перевыделяет память: ссылка,
			 * взятая прежде, указывает на память освобождённую. Надзор за памятью валит это
			 * настоящим обращением к освобождённому - `heap-use-after-free`, замерено щупом
			 * на шестидесяти четырёх соседях
			 *
			 * @warning Клейма поколения, каким лечится ссылка на узел у дерева YAML, здесь
			 * поставить НЕЧЕМ: ссылка языка признака нести не может, и поверить её нельзя
			 * ничем. Оттого правило записано договором: держать надлежит ПУТЬ, а не ссылку -
			 * `значение["раздел"]["ключ"]` заново, а не `Value & ключ` про запас
			 *
			 * @note Правило это общее у INI и TOML, а найдено оно Василием у кодеков JSON и
			 * XML: устройство там то же - дети в перемещаемом вместилище
			 *
			 *
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
					/**
					 * \~russian
					 * Код отказа последней работы над владеющим значением
					 *
					 * @details Держится он ради того, чтобы отказ НАЗЫВАЛ ПРИЧИНУ: запись
					 * отвечает текстом, и пустой текст у неё означал разом и дерево пустое, и
					 * отказ записи, - различить их потребителю было нечем вовсе. Причину
					 * писатель знал и выбрасывал: объект записи живёт внутри одного вызова
					 *
					 * @note Немота эта найдена разбором, а не отказом: пустой текст ни к
					 *       падению, ни к расхождению не ведёт - потребитель просто сохраняет
					 *       пустой файл вместо своих настроек
					 *
					 * \~english
					 * Code of the failure of the last operation over the owning value
					 *
					 * \~
					 */
					mutable error_t _error = error_t::NONE;
					const log_t * _log = nullptr;
				private:
					// Тип хранимого значения
					type_t _type;
				private:
					/**
					 * \~russian
					 * Признак значения, записанного в кавычках
					 *
					 * @details Держится он ради записи в текст: кавычки выбраны потребителем,
					 * и запись обязана их соблюдать. Содержимого признак не касается вовсе -
					 * кавычки обрамляющие в него не входят
					 *
					 * \~english
					 * Flag of a value written in quotes
					 * @details It is kept for the sake of the writing into a text: the quotes have been chosen
					 * by the consumer, and the writing is obliged to observe them. The flag does not concern
					 * the content at all — the enclosing quotes are not included into it
					 *
					 * \~
					 */
					bool _quoted;
				private:
					// Содержимое простого значения, хранимое собственной памятью
					string _text;
				private:
					/**
					 * \~russian
					 * Имена пар вместилища
					 *
					 * @details Перечень этот наполняется лишь у вместилища пар: у перечня
					 * значений имён нет вовсе, и хранить пустые строки ему незачем
					 *
					 * \~english
					 * Names of the pairs of a container
					 * @details This list is filled only for a container of the pairs: an array of the values
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
				private:
					/**
					 * \~russian
					 * @brief Метод снятия значения с дерева настроек
					 *
					 * @param document дерево настроек, откуда снимается значение
					 *
					 * \~english
					 * @brief Method of the taking of a value from a settings tree
					 * @param document settings tree wherefrom the value is taken
					 *
					 * \~
					 */
					void absorb(const Document & document) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения предела роста вместилища по номеру
					 *
					 * @return предел роста вместилища по номеру
					 *
					 * \~english
					 * @brief Method of the extraction of the limit of the growth of a container by an index
					 * @return limit of the growth of a container by an index
					 *
					 * \~
					 */
					static size_t limit() noexcept;
					/**
					 * \~russian
					 * @brief Метод установки предела роста вместилища по номеру
					 *
					 * @param value устанавливаемый предел роста
					 *
					 * \~english
					 * @brief Method of the setting of the limit of the growth of a container by an index
					 * @param value limit of the growth being set
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
					 * @brief Метод получения кода отказа последней работы
					 *
					 * @details Ход этот отвечает на вопрос «отчего запись пуста»: пустой текст
					 * означает разом и дерево пустое, и отказ записи, и различить их иначе
					 * потребителю нечем. Код держится от ПОСЛЕДНЕЙ работы, а не от последней
					 * неудачной - правило это одно у дерева и у значения
					 *
					 * @note Прежде отказ записи молчал целиком: ни кода, ни сообщения в журнал.
					 *       Потребитель сохранял бы пустой файл вместо своих настроек и узнал бы
					 *       о том лишь при следующем чтении, а то и никогда
					 *
					 * @return код отказа последней работы
					 *
					 * \~english
					 * @brief Method of getting the code of the failure of the last operation
					 * @return code of the failure of the last operation
					 *
					 * \~
					 */
					error_t error() const noexcept;
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
					/**
					 * \~russian
					 * @brief Метод извлечения количества значений вместилища
					 *
					 * @return количество значений вместилища
					 *
					 * \~english
					 * @brief Method of the extraction of the quantity of the values of a container
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
					 * @brief Метод извлечения содержимого простого значения
					 *
					 * @return содержимое простого значения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a simple value
					 * @return content of the simple value
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имени пары вместилища по номеру
					 *
					 * @param index порядковый номер пары вместилища
					 * @return      имя пары вместилища
					 *
					 * \~english
					 * @brief Method of the extraction of the name of a pair of a container by an index
					 * @param index ordinal index of the pair of the container
					 * @return name of the pair of the container
					 *
					 * \~
					 */
					const string & key(const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения признака значения, записанного в кавычках
					 *
					 * @return признак значения, записанного в кавычках
					 *
					 * \~english
					 * @brief Method of the extraction of the flag of a value written in quotes
					 * @return flag of the value written in quotes
					 *
					 * \~
					 */
					bool quoted() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки признака значения, записанного в кавычках
					 *
					 * @param quoted устанавливаемый признак
					 *
					 * \~english
					 * @brief Method of the setting of the flag of a value written in quotes
					 * @param quoted flag being set
					 *
					 * \~
					 */
					void quoted(const bool quoted) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия пары вместилища по имени
					 *
					 * @param name имя искомой пары вместилища
					 * @return     признак наличия пары вместилища
					 *
					 * \~english
					 * @brief Method of the check of the presence of a pair of a container by a name
					 * @param name name of the pair of the container being sought
					 * @return sign of the presence of the pair of the container
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
					 * @details Путь задаётся частями, отделёнными косой чертой: «/раздел/свойство».
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
					 * @details The path is given by parts separated by a slash: «/section/property».
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
					 * @param path путь до заводимого значения
					 * @return     заведённое значение
					 *
					 * \~english
					 * @brief Method of the creation of a nested value by a path
					 * @param path path to the value being created
					 * @return created value
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор обращения к паре вместилища по имени
					 *
					 * @param name имя искомой пары вместилища
					 * @return     найденное значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a pair of a container by a name
					 * @param name name of the pair of the container being sought
					 * @return found value
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к паре вместилища по имени с заведением
					 *
					 * @param name имя искомой пары вместилища
					 * @return     найденное либо заведённое значение
					 *
					 * \~english
					 * @brief Operator of the addressing to a pair of a container by a name with a creation
					 * @param name name of the pair of the container being sought
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
					 * @brief Метод установки пары вместилища
					 *
					 * @details Имя занятое перезаписывается на прежнем месте: порядок пар
					 * задан потребителем, и перестановка их при перезаписи меняла бы вид
					 * записанного текста без его на то воли
					 *
					 * @note Имя пустое отвергается отказом, а не подставляется: свойство без
					 *       имени записано быть не может, и молчаливая подстановка скрыла бы
					 *       ошибку у потребителя
					 *
					 * @param name  имя устанавливаемой пары
					 * @param value устанавливаемое значение
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a pair of a container
					 * @details An occupied name is overwritten in its former place: the order of the pairs is
					 * given by the consumer, and their rearrangement at an overwriting would change the appearance
					 * of the written text without its will for that
					 * @note An empty name is rejected with a refusal rather than substituted: a property without
					 *       a name cannot be written, and a silent substitution would conceal a mistake
					 *       of the consumer
					 * @param name  name of the pair being set
					 * @param value value being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления пары вместилища без перезаписи
					 *
					 * @note Отличие от установки в том, что имя занятое отвергается отказом:
					 *       добавление есть заявление о новизне имени
					 *
					 * @param name  имя добавляемой пары
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a pair of a container without an overwriting
					 * @note The difference from the setting is that an occupied name is rejected with a refusal:
					 *       an addition is a declaration of the novelty of the name
					 * @param name  name of the pair being added
					 * @param value value being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool append(const string & name, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления пары вместилища по имени
					 *
					 * @param name имя удаляемой пары
					 * @return     признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a pair of a container by a name
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
					 * @details Разбор ведётся расширенным видом записи: «true», «yes», «on» и
					 * единица суть истина, а «false», «no», «off» и ноль - ложь. Регистр
					 * записи не учитывается
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a logical value
					 * @details The parsing is conducted by the extended notation: «true», «yes», «on» and
					 * a one are the truth, while «false», «no», «off» and a zero are the falsehood. The case
					 * of the record is not taken into account
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
					 * @details Отказом извлечение завершается лишь тогда, когда запись значения
					 * числом не является вовсе. Запись извлечению не указ: дробная запись
					 * извлекается и целым видом, а целая - и дробным
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
					 * @details The extraction ends with a refusal only when the record of the value is not
					 * a number at all. The record is not a directive to the extraction: a fractional record is
					 * extracted also as an integer kind, and an integer one — also as a fractional
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
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(float & result) const noexcept;
					/**
					 * @copydoc awh::codec::ini::Value::value(int8_t &) const
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого простого значения
					 *
					 * @param result переменная, куда помещается извлечённое содержимое
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of the content of a simple value
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
					 * @details Построение записываемого дерева ограничено наречием: корень
					 * несёт свойства верхнего уровня и разделы, раздел несёт свойства и
					 * подразделы, подраздел несёт одни свойства. Свойство простое либо
					 * перечнем одноимённых значений
					 *
					 * @note Дерево, построение это нарушающее, отвечает пустым текстом:
					 *       записать его текстом настроек нельзя, а записать частью значило
					 *       бы потерять остаток молча
					 *
					 * @note Оформление исходного текста при этом не восстанавливается:
					 *       удержание его есть дело дерева настроек
					 *
					 * @return записанный текст настроек
					 *
					 * \~english
					 * @brief Method of the writing of an owning value as a settings text
					 * @details The construction of the tree being written is limited by the dialect: the root
					 * carries the properties of the top level and the sections, a section carries the properties
					 * and the subsections, a subsection carries the properties alone. A property is a simple one
					 * or an array of the values of the same name
					 * @note A tree violating this construction answers with an empty text: it cannot be written
					 *       as a settings text, while to write it partially would mean to lose the remainder silently
					 * @note The formatting of the source text is thereby not restored: its retention is
					 *       the business of the settings tree
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
					 * @details Перенос ведётся тем же построением, каким ведётся запись:
					 * корень несёт свойства и разделы, раздел - свойства и подразделы
					 *
					 * @note Дерево, построение это нарушающее, отвечает отказом: молчаливый
					 *       пропуск оставил бы потребителя с деревом без части его значения
					 *
					 * @note Перечень значений переносится повтором имени свойства - записью,
					 *       какой перечень записи INI и является. Объявления, в разделе уже
					 *       имеющиеся, при этом сносятся: перенос обязан выдать перечень тот,
					 *       какой в значении, а не сросшийся с прежним. Своё место в разделе
					 *       перечень тем самым теряет и уходит в конец его
					 *
					 * @param document дерево настроек, куда переносится значение
					 * @return         признак успешности переноса
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into a settings tree
					 * @details The grafting is conducted by the same construction by which the writing is
					 * conducted: the root carries the properties and the sections, a section — the properties
					 * and the subsections
					 * @note A tree violating this construction answers with a refusal: a silent skipping would
					 *       leave the consumer with a tree without a part of its value
					 * @note A list of the values is grafted by a repetition of the name of the property — the record
					 *       which a list of the INI notation is. The declarations already present in the section
					 *       are thereby removed: the grafting must issue the list which is in the value
					 *       rather than one grown together with the previous one. The list thereby loses its place
					 *       in the section and goes to the end of it
					 * @param document settings tree whereinto the value is grafted
					 * @return sign of the success of the grafting
					 *
					 * \~
					 */
					bool graft(Document & document) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор сличения значений
					 *
					 * @note Сличение вместилищ пар порядка НЕ учитывает, а сличение перечней
					 *       учитывает: вместилище есть отображение имён на значения, и
					 *       порядок записи его значением не является
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values
					 * @note The comparison of the containers of the pairs does NOT take the order into account,
					 *       while the comparison of the arrays does: a container is a mapping of the names onto
					 *       the values, and the order of its writing is not a value
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
					 * @param type тип заводимого значения
					 *
					 * \~english
					 * @brief Constructor of a container of the demanded type
					 * @param type type of the value being created
					 *
					 * \~
					 */
					explicit Value(const type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор простого значения
					 *
					 * @param value  устанавливаемое содержимое
					 * @param quoted признак значения, записанного в кавычках
					 *
					 * \~english
					 * @brief Constructor of a simple value
					 * @param value  content being set
					 * @param quoted flag of a value written in quotes
					 *
					 * \~
					 */
					Value(const string & value, const bool quoted = false) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор простого значения из строки языка
					 *
					 * @note Конструктор этот заведён рядом с принимающим `string`
					 *       намеренно: без него запись `Value("текст")` уходила бы к
					 *       конструктору вида через приведение указателя
					 *
					 * @param value  устанавливаемое содержимое
					 * @param quoted признак значения, записанного в кавычках
					 *
					 * \~english
					 * @brief Constructor of a simple value from a string of the language
					 * @note This constructor is created next to the one accepting a `string` deliberately:
					 *       without it the record `Value("текст")` would go to the constructor of a kind
					 *       through a conversion of the pointer
					 * @param value  content being set
					 * @param quoted flag of a value written in quotes
					 *
					 * \~
					 */
					Value(const char * value, const bool quoted = false) noexcept;
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
			 * путями, а не вместо них. Потребитель, свойства подряд заносящий,
			 * вложенностей не порождает и путями не пользуется вовсе
			 *
			 * @details Договор её слово в слово повторяет договор потока записи: открыть
			 * вместилище, назвать свойство, записать значение, закрыть
			 *
			 * @note Вместилища зовутся по наречию: у INI это `section` и `list`. Раздел и
			 *       подраздел зовутся одним именем оттого, что различает их не устройство,
			 *       а глубина: подраздел есть раздел, открытый внутри раздела
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value
			 * @details This assembly is the second way to build a tree, standing alongside the paths
			 * rather than instead of them. A consumer entering the properties in a row does not generate
			 * the nestings and does not use the paths at all
			 * @details Its contract repeats the contract of the writing stream word for word: open
			 * a container, name a property, write a value, close
			 * @note The containers are called by the dialect: with INI these are `section` and `list`.
			 *       A section and a subsection are called by one name because they are distinguished not by
			 *       the arrangement but by the depth: a subsection is a section opened inside a section
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
					 * лежат в перечне, и всякое добавление переселяет его в памяти
					 *
					 * \~english
					 * Path to the container opened by the assembly
					 * @details The path is stored by the indices rather than by the pointers: the values
					 * of a container lie in a list, and every addition relocates it in the memory
					 *
					 * \~
					 */
					vector <size_t> _path;
				private:
					// Имя свойства, значения ожидающее
					string _key;
				private:
					// Признак того, что имя свойства назначено
					bool _keyed;
				private:
					// Признак того, что сборка завершена
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
					 * @brief Метод открытия раздела
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a section
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool section() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений одноимённого свойства
					 *
					 * @return признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of an array of the values of a property of the same name
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool list() noexcept;
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
					 * @brief Метод назначения имени свойства
					 *
					 * @note Имя пустое отвергается отказом, а имя, назначенное дважды
					 *       подряд, - тоже: и то и другое есть ошибка у потребителя
					 *
					 * @param name назначаемое имя свойства
					 * @return     признак успешности назначения
					 *
					 * \~english
					 * @brief Method of the assignment of the name of a property
					 * @note An empty name is rejected with a refusal, and a name assigned twice in a row —
					 *       likewise: both the one and the other are a mistake of the consumer
					 * @param name name of the property being assigned
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
					 * @brief Метод записи простого значения
					 *
					 * @param value  записываемое содержимое
					 * @param quoted признак значения, записанного в кавычках
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a simple value
					 * @param value  content being written
					 * @param quoted flag of a value written in quotes
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const string & value, const bool quoted = false) noexcept;
					/**
					 * @copydoc awh::codec::ini::Builder::value(const string &, const bool)
					 */
					bool value(const char * value, const bool quoted = false) noexcept;
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
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the withdrawal of the assembled value
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

#endif // __AWH_CODEC_INI_VALUE__
