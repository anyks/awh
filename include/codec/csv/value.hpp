/**
 * @file value.hpp
 * @date 2026-09-04
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
 * @brief Заголовочный файл владеющего значения CSV — самостоятельный тип данных,
 *        хранящий таблицу собственной памятью, собираемый из значений языка и
 *        пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of CSV — a standalone data type
 *        storing a table by its own memory, assembled from the values of the language and
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
#ifndef __AWH_CODEC_CSV_VALUE__
#define __AWH_CODEC_CSV_VALUE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

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
		 * @brief Пространство имён контейнера CSV
		 *
		 * \~english
		 * @brief CSV container namespace
		 *
		 * \~
		 */
		namespace csv {
			/**
			 * \~russian
			 * @brief Владеющее значение CSV
			 *
			 * @details Тип этот стоит **над** таблицей, а не вместо неё. Таблица разбирает
			 * текст, удерживает его хранилище знаков и отдаёт наружу ВИДЫ в это хранилище,
			 * тогда как владеющее значение держит содержимое собственной памятью и оттого
			 * копируется, передаётся внутрь и отдаётся наружу итогом метода
			 *
			 * @warning Довод к заведению этого типа - не соразмерность прочим кодекам, а
			 * беда, замеренная у самой таблицы: всякое `get`, `row`, `col` и `header`
			 * отдаёт `string_view` в общее хранилище знаков, и хранилище это ПЕРЕЕЗЖАЕТ
			 * при росте. Вид, переживший добавление записи, читает чужое содержимое - молча,
			 * не падая и не подавая знака. Владеющее значение снимает эту заботу целиком:
			 * взятое им живёт своей памятью и таблице более не подвластно
			 *
			 * @note Построение здесь мельче, нежели у прочих кодеков, и это не упущение, а
			 *       свойство наречия: таблица есть перечень записей, запись - перечень
			 *       полей, поле - последовательность знаков. Трёх ступеней довольно, и
			 *       четвёртой у RFC 4180 нет
			 *
			 * @note Своих видов значения у наречия CSV нет вовсе: поле есть последовательность
			 *       знаков, а число ли это либо логическое значение - решает извлечение.
			 *       Оттого простое значение здесь всегда строковое, а извлечение числа
			 *       разбирает запись его при всяком обращении. Уклад этот один с наречием INI
			 *
			 * @note Заголовок таблицы удерживается ОТДЕЛЬНО от записей, а не первой из них:
			 *       иначе счёт записей зависел бы от настройки разбора, и `size()` у одного
			 *       и того же текста выдавал бы то два, то три
			 *
			 * \~english
			 * @brief Owning value of CSV
			 *
			 * @details This type stands **above** the table, and not instead of it. The table parses
			 * the text, holds its storage of the characters and issues outwards the VIEWS into that storage,
			 * whereas the owning value holds the content by its own memory and hence is copied,
			 * passed inwards and issued outwards as a result of a method
			 *
			 * @warning The argument for the introduction of this type is not the proportionality to the other
			 * codecs, but a trouble measured at the table itself: every `get`, `row`, `col` and `header`
			 * issues a `string_view` into the common storage of the characters, and that storage MOVES
			 * at a growth. A view which has survived an addition of a record reads a foreign content — silently,
			 * without falling and without giving a sign. The owning value removes that care entirely:
			 * what is taken by it lives by its own memory and is no longer subject to the table
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				public:
					/**
					 * \~russian
					 * @brief Вид хранимого значения
					 *
					 * @note Видов здесь ТРИ и ни одним более: таблица, запись и поле. Наречие
					 *       глубже не строит, и заводить вид «перечня общего» значило бы
					 *       обещать построение, которое записать текстом нельзя
					 *
					 * \~english
					 * @brief Kind of the stored value
					 *
					 * \~
					 */
					enum class type_t : uint8_t {
						NONE   = 0x00, // Значение не задано
						TABLE  = 0x01, // Таблица, перечень записей
						RECORD = 0x02, // Запись, перечень полей
						FIELD  = 0x03  // Поле, последовательность знаков
					};
				private:
					// Вид хранимого значения
					type_t _type;
					// Содержимое поля, значимое лишь у вида FIELD
					string _text;
					// Имена столбцов, значимые лишь у вида TABLE
					vector <string> _header;
					// Вложенные значения: записи у таблицы, поля у записи
					vector <Value> _items;
					// Объект ведения журнала работы
					const log_t * _log;
					// Код отказа последней работы над значением
					mutable error_t _error;
					// Место отказа последней работы над значением
					mutable location_t _errorLocation;
				private:
					/**
					 * \~russian
					 * @brief Метод получения пустого значения общего пользования
					 *
					 * @details Отдаётся оно обращением к отсутствующему полю: ссылка обязана
					 *          указывать на что-то живое, а заводить пустое значение при
					 *          каждом промахе значило бы плодить их без счёта
					 *
					 * @warning Значение это ОБЩЕЕ, и правка его сквозь снятое постоянство
					 *          отравила бы всякое последующее обращение к отсутствующему
					 *          полю. Оттого наружу оно отдаётся лишь постоянной ссылкой
					 *
					 * @return пустое значение общего пользования
					 *
					 * \~english
					 * @brief Method of getting an empty value of a common use
					 * @return empty value of a common use
					 *
					 * \~
					 */
					static const Value & scrap() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки пригодности значения
					 *
					 * @return признак пригодности значения
					 *
					 * \~english
					 * @brief Method of checking the validity of the value
					 * @return sign of the validity of the value
					 *
					 * \~
					 */
					bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода отказа последней работы над значением
					 *
					 * @details Код этот отвечает на вопрос, отчего работа не сбылась: разбор
					 * текста, чтение файла, выдача текста и запись его в файл ставят его
					 * заново каждая. Без канала отказа пустой текст выдачи неотличим от
					 * значения пустого
					 *
					 * @note Договор этот ОБЩИЙ у кодеков: владеющие значения документа JSON
					 *       и разметки XML отвечают тем же кодом и тем же порядком
					 *
					 * @warning Работы, отказать не могущие, кода НЕ трогают: снятие вида,
					 * размера и содержимого поля оставляют его как есть. Ставь мы код всякою
					 * работой, и донесение об отказе стиралось бы первым же чтением следом
					 *
					 * @return код отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the code of the refusal of the last operation over the value
					 * @return code of the refusal of the last operation
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.ErrorAndItsLocationAreSetAnewByEveryCall
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места отказа последней работы над значением
					 *
					 * @details Место называет строку и столбец текста, на которых разбор
					 *          споткнулся. У работ, места не имеющих - запись в файл, выдача
					 *          текста, - оно остаётся пустым
					 *
					 * @return место отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the location of the refusal of the last operation over the value
					 * @return location of the refusal of the last operation
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки объекта ведения журнала работы
					 *
					 * @details Привязка поздняя нужна там, где значение заведено копией либо
					 *          собрано из значений языка: журнала при заведении ему взять
					 *          неоткуда, а сообщать о бедах оно обязано туда же, куда и
					 *          прочие части кодека
					 *
					 * @note Журнал уходит ВГЛУБЬ по вложенным значениям: значение владеет
					 *       ими целиком, и разойдись журнал у родителя с детьми - беда
					 *       записи поля уходила бы в пустоту, тогда как беда таблицы
					 *       сообщалась бы исправно
					 *
					 * @param log объект ведения журнала работы
					 *
					 * \~english
					 * @brief Method of the setting of the object of the keeping of the work log
					 * @param log the object of the keeping of the work log
					 *
					 * \~
					 */
					void setLogger(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида хранимого значения
					 *
					 * @return вид хранимого значения
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the stored value
					 * @return kind of the stored value
					 *
					 * \~
					 */
					type_t type() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки вида хранимого значения
					 *
					 * @param type сличаемый вид значения
					 * @return     признак совпадения вида
					 *
					 * \~english
					 * @brief Method of checking the kind of the stored value
					 * @param type kind of the value being compared
					 * @return     sign of the coincidence of the kind
					 *
					 * \~
					 */
					bool is(const type_t type) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества вложенных значений
					 *
					 * @details У таблицы это число ЗАПИСЕЙ, у записи - число полей, у поля -
					 *          длина содержимого в знаках
					 *
					 * @warning Заголовок в счёт записей НЕ входит: он удерживается отдельно,
					 *          и вхождение его обращало бы счёт в зависимость от настройки
					 *          разбора
					 *
					 * @return количество вложенных значений
					 *
					 * \~english
					 * @brief Method of the extraction of the quantity of the nested values
					 * @return quantity of the nested values
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
					 * @brief Method of checking the emptiness of the value
					 * @return sign of the emptiness of the value
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
					 * @brief Метод извлечения содержимого поля
					 *
					 * @warning У значений иных видов содержимое пусто: склейки полей записи
					 *          здесь НЕ делается намеренно, ибо всякая склейка требовала бы
					 *          разделителя, а разделитель есть дело настроек записи, не
					 *          значения
					 *
					 * @return содержимое поля
					 *
					 * \~english
					 * @brief Method of the extraction of the content of the field
					 * @return content of the field
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.LongNumberRecord
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения имён столбцов таблицы
					 *
					 * @return имена столбцов таблицы
					 *
					 * \~english
					 * @brief Method of the extraction of the names of the columns of the table
					 * @return names of the columns of the table
					 *
					 * \~
					 */
					const vector <string> & header() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки имён столбцов таблицы
					 *
					 * @warning Повтор имени отвергается ВСЕГДА: столбец, по имени недостижимый,
					 *          молчал бы, а проверка здесь свободна от расхода на разбор.
					 *          Уклад этот один с `Document::header`
					 *
					 * @param names имена столбцов таблицы
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of the names of the columns of the table
					 * @param names names of the columns of the table
					 * @return      sign of the success of the setting
					 *
					 * \~
					 */
					bool header(const vector <string> & names) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия столбца с указанным именем
					 *
					 * @details Имя ходу этому дано общее с прочими кодеками НАМЕРЕННО: у
					 *          владеющих значений документа JSON, разметки XML и трёх
					 *          наречий Николая наличие вложенного значения по имени
					 *          спрашивается словом `contains`, а слово `has` занято у
					 *          разметки под наличие СВОЙСТВА узла - величины иной
					 *
					 * @warning Первая редакция звала этот ход `has`, и расхождение вышло бы
					 * тихим: потребитель, переносящий разбор с документа на таблицу, получил
					 * бы отказ сборки в лучшем случае, а в худшем - попал бы у разметки в
					 * ход об ином предмете
					 *
					 * @param name имя столбца
					 * @return     признак наличия столбца
					 *
					 * \~english
					 * @brief Method of checking the presence of a column with the indicated name
					 * @param name name of the column
					 * @return     sign of the presence of the column
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.PresenceOfANestedValueIsAskedByOneWord
					 */
					bool contains(const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска номера столбца по имени
					 *
					 * @param name имя столбца
					 * @return     номер столбца, `SIZE_MAX` при отсутствии
					 *
					 * \~english
					 * @brief Method of searching for the number of a column by a name
					 * @param name name of the column
					 * @return     number of the column, `SIZE_MAX` in an absence
					 *
					 * \~
					 */
					size_t column(const string & name) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к вложенному значению по номеру
					 *
					 * @param index номер вложенного значения
					 * @return      вложенное значение
					 *
					 * \~english
					 * @brief Method of addressing a nested value by a number
					 * @param index number of the nested value
					 * @return      nested value
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к полю записи по имени столбца
					 *
					 * @warning Имена столбцов принадлежат ТАБЛИЦЕ, а запись их не знает.
					 *          Оттого обращение это работает у записи, взятой из таблицы
					 *          обходом `at`, и молчит у записи самостоятельной: имени негде
					 *          сличиться. Уклад этот вынужден устройством наречия
					 *
					 * @param name имя столбца
					 * @return     поле записи
					 *
					 * \~english
					 * @brief Method of addressing a field of a record by a name of a column
					 * @param name name of the column
					 * @return     field of the record
					 *
					 * \~
					 */
					const Value & operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению по пути
					 *
					 * @details Путь строится звеньями, разделёнными косой чертой: «/2/имя»
					 *          ведёт к полю столбца «имя» третьей записи, «/2/1» - к полю
					 *          вторым по счёту. Пустой путь ведёт к самому значению
					 *
					 * @warning Пустой путь и одиночная косая черта РАЗНЫЕ: по RFC 6901 пустой
					 * путь означает весь документ, а «/» - звено с пустым именем. Столбца с
					 * пустым именем у таблицы не бывает вовсе, оттого «/» ведёт в никуда
					 * всегда - тем же итогом, каким отвечает документ JSON без поля с пустым
					 * именем. Приравняй мы их, и одна и та же запись пути значила бы у
					 * кодеков разное
					 *
					 * @param path путь к искомому значению
					 * @return     найденное значение
					 *
					 * \~english
					 * @brief Method of addressing a value by a path
					 * @param path path to the sought value
					 * @return     value which is found
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.EnumerationAndLookupFormAClosedTraversal
					 */
					const Value & at(const string & path) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод добавления записи в таблицу
					 *
					 * @param value добавляемое значение
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a record into the table
					 * @param value value being added
					 * @return      sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления вложенного значения по номеру
					 *
					 * @param index номер удаляемого значения
					 * @return      признак успешности удаления
					 *
					 * \~english
					 * @brief Method of the removal of a nested value by a number
					 * @param index number of the value being removed
					 * @return      sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const size_t index) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон вида числа, к какому ведётся извлечение
					 *
					 * @tparam T вид числа, к какому ведётся извлечение
					 *
					 * \~english
					 * @brief Template of the type of a number to which the extraction is conducted
					 * @tparam T type of a number to which the extraction is conducted
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод извлечения числового значения поля
					 *
					 * @details Ход этот ОБЩИЙ у всего ряда извлечений, и уклад сужения задан
					 *          им однажды: целое, в затребованный вид не помещающееся,
					 *          заворачивается по кругу приведением языка, а дробное, целая
					 *          часть которого лежит за пределами вида, выдаётся пределом
					 *          этого вида - там у языка поведение неопределено
					 *
					 * @note Уклад этот взят у кодеков JSON и XML СЛОВО В СЛОВО, а приведение
					 *       дробного ведётся общим ходом рамки `awh::codec::convert`. Пиши
					 *       мы своё приведение, и расхождение вышло бы там, где его никто
					 *       не ищет: у краёв вида
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a numeric value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool extract(T & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения логического значения поля
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a boolean value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(bool & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения целого значения поля видом в один байт
					 *
					 * @note Округление дробной записи идёт по правилам математики с половиной,
					 *       отбираемой от нуля. Запись, в затребованный вид не вмещающаяся,
					 *       отказом НЕ отвечает: целое заворачивается по кругу, а дробное
					 *       выдаётся пределом вида - уклад один с кодеками JSON и XML
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an integer value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.Narrowing
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения целого значения поля видом в два байта
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an integer value of a field of a two-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения целого значения поля видом в четыре байта
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an integer value of a field of a four-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения целого значения поля видом в восемь байтов
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an integer value of a field of an eight-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения беззнакового целого значения поля видом в один байт
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an unsigned integer value of a field of a one-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения беззнакового целого значения поля видом в два байта
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an unsigned integer value of a field of a two-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения беззнакового целого значения поля видом в четыре байта
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an unsigned integer value of a field of a four-byte type
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения беззнакового целого значения поля
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of an unsigned integer value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения дробного значения поля
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a floating value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(float & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения дробного значения поля двойной точности
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a floating value of a field of a double precision
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строкового значения поля
					 *
					 * @param result извлекаемое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a string value of a field
					 * @param result value being extracted
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод снятия значения с таблицы
					 *
					 * @details Снятие идёт СОБСТВЕННОЙ памятью: виды в хранилище таблицы
					 *          копируются знак в знак, и снятое значение таблице более не
					 *          подвластно
					 *
					 * @param document таблица, с которой снимается значение
					 * @return         признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the taking of a value from a table
					 * @param document table from which the value is taken
					 * @return         sign of the success of the taking
					 *
					 * \~
					 */
					bool absorb(const Document & document) noexcept;
					/**
					 * \~russian
					 * @brief Метод прививки значения к таблице
					 *
					 * @warning Прививка СНОСИТ прежнее содержимое таблицы целиком: значение
					 *          есть таблица сама по себе, и слияние двух таблиц требовало бы
					 *          правила сведения заголовков, которого у RFC 4180 нет
					 *
					 * @param document таблица, к которой прививается значение
					 * @return         признак успешности прививки
					 *
					 * \~english
					 * @brief Method of the grafting of a value onto a table
					 * @param document table onto which the value is grafted
					 * @return         sign of the success of the grafting
					 *
					 * \~
					 */
					bool graft(Document & document) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста таблицы
					 *
					 * @param text исходный текст таблицы
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a text of a table
					 * @param text source text of the table
					 * @return     sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста таблицы с указанными настройками
					 *
					 * @param text     исходный текст таблицы
					 * @param settings настройки разбора текста таблицы
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a text of a table with the specified settings
					 * @param text     source text of the table
					 * @param settings settings of the parsing of the text of the table
					 * @return         sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const Document::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла
					 *
					 * @param filename адрес файла таблицы
					 * @return         признак успешности чтения
					 *
					 * \~english
					 * @brief Method of the reading of a table from a file
					 * @param filename address of the file of the table
					 * @return         sign of the success of the reading
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения таблицы из файла с указанными настройками
					 *
					 * @param filename адрес файла таблицы
					 * @param settings настройки разбора текста таблицы
					 * @return         признак успешности чтения
					 *
					 * \~english
					 * @brief Method of the reading of a table from a file with the specified settings
					 * @param filename address of the file of the table
					 * @param settings settings of the parsing of the text of the table
					 * @return         sign of the success of the reading
					 *
					 * \~
					 */
					bool load(const string & filename, const Document::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод сохранения таблицы в файл
					 *
					 * @details Запись ведётся во временный файл рядом с целевым, а тот
					 *          подменяется переименованием - действием неделимым
					 *
					 * @warning Уклад этот ОБЯЗАТЕЛЕН, а не желателен: прямая запись в целевой
					 * файл усекает его первым же действием, и отказ на середине - место на
					 * устройстве кончилось, предел размера достигнут, работа прервана -
					 * оставляет потребителя без прежнего содержимого И без нового. Замер
					 * пределом `RLIMIT_FSIZE` показал ровно это до правки: прежний файл
					 * пропадал целиком
					 *
					 * @param filename адрес файла таблицы
					 * @return         признак успешности сохранения
					 *
					 * \~english
					 * @brief Method of the saving of the table into a file
					 * @param filename address of the file of the table
					 * @return         sign of the success of the saving
					 *
					 * \~
					 * @see Договор этот закреплён проверкою CodecContract.SavingKeepsThePreviousFileOnWriteFailure
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи текста таблицы
					 *
					 * @return собранный текст таблицы
					 *
					 * \~english
					 * @brief Method of the issuing of a text of the table
					 * @return assembled text of the table
					 *
					 * \~
					 */
					string dump() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор сличения значений
					 *
					 * @param value сличаемое значение
					 * @return      признак равенства значений
					 *
					 * \~english
					 * @brief Operator of the comparison of the values
					 * @param value value being compared
					 * @return      sign of the equality of the values
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
					 * @return      sign of the inequality of the values
					 *
					 * \~
					 */
					bool operator != (const Value & value) const noexcept;
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
					 * @brief Конструктор вида значения
					 *
					 * @param type вид заводимого значения
					 *
					 * \~english
					 * @brief Constructor of a kind of a value
					 * @param type kind of the value being created
					 *
					 * \~
					 */
					explicit Value(const type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор поля
					 *
					 * @param text содержимое заводимого поля
					 *
					 * \~english
					 * @brief Constructor of a field
					 * @param text content of the field being created
					 *
					 * \~
					 */
					explicit Value(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор снятия значения с таблицы
					 *
					 * @param document таблица, с которой снимается значение
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a table
					 * @param document table from which the value is taken
					 *
					 * \~
					 */
					explicit Value(const Document & document) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор с объектом ведения журнала работы
					 *
					 * @param log объект ведения журнала работы
					 *
					 * \~english
					 * @brief Constructor with the object of the keeping of the work log
					 * @param log the object of the keeping of the work log
					 *
					 * \~
					 */
					explicit Value(const log_t * log) noexcept;
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
			 * @brief Потоковая сборка владеющего значения CSV
			 *
			 * @details Сборщик этот повторяет договор потока записи слово в слово: подать
			 * поле, завершить запись, завершить таблицу. Оттого «пишу текстом» и «строю
			 * таблицу» отличаются у потребителя одной буквой, а не двумя разными договорами
			 *
			 * @details Заведён сборщик ради тех, кто строит таблицу с нуля полем за полем:
			 * сборка записи через перечень требовала бы держать её целиком, а поток
			 * добавляет поле за полем и о собранном не помнит
			 *
			 * @note Числа подаются сборщику РОДНЫМ своим видом, а не записью: подстановка
			 *       числа текстом заставила бы потребителя собирать запись самому. Уклад
			 *       этот один со сборщиками прочих кодеков
			 *
			 * @warning Заголовок объявляется ОТДЕЛЬНЫМ ходом и до первой записи: объяви его
			 * первой поданной записью, и сборщик не отличал бы таблицу без заголовка от
			 * таблицы, у которой первая запись есть данные
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value of CSV
			 *
			 * @details This builder repeats the contract of the writing stream word for word: to pass
			 * a field, to finish a record, to finish the table. Whereby "I write a text" and "I build
			 * a table" differ for a consumer by one letter rather than by two different contracts
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Builder {
				private:
					// Собираемое значение таблицы
					Value _result;
					// Собираемая запись таблицы
					Value _record;
					// Признак того, что запись открыта подачею поля
					bool _opened;
				public:
					/**
					 * \~russian
					 * @brief Метод объявления имён столбцов таблицы
					 *
					 * @warning Объявляется заголовок ДО первой записи: подача его после
					 * отвечает отказом, ибо записи, поданные прежде, столбцов не знали бы
					 *
					 * @param names имена столбцов таблицы
					 * @return      признак успешности объявления
					 *
					 * \~english
					 * @brief Method of the declaration of the names of the columns of the table
					 * @param names names of the columns of the table
					 * @return      sign of the success of the declaration
					 *
					 * \~
					 */
					bool header(const vector <string> & names) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи поля записи строковым значением
					 *
					 * @param text содержимое подаваемого поля
					 * @return     признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the passing of a field of a record by a string value
					 * @param text content of the field being passed
					 * @return     sign of the success of the passing
					 *
					 * \~
					 */
					bool field(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи поля записи целым значением
					 *
					 * @note Число подаётся РОДНЫМ видом, а не записью: собери потребитель
					 *       запись сам, и уклад её разошёлся бы с укладом записи таблицы
					 *
					 * @param number подаваемое число
					 * @return       признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the passing of a field of a record by an integer value
					 * @param number number being passed
					 * @return       sign of the success of the passing
					 *
					 * \~
					 */
					bool field(const int64_t number) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи поля записи дробным значением
					 *
					 * @param number подаваемое число
					 * @return       признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the passing of a field of a record by a floating value
					 * @param number number being passed
					 * @return       sign of the success of the passing
					 *
					 * \~
					 */
					bool field(const double number) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи поля записи логическим значением
					 *
					 * @param flag подаваемое логическое значение
					 * @return     признак успешности подачи
					 *
					 * \~english
					 * @brief Method of the passing of a field of a record by a boolean value
					 * @param flag boolean value being passed
					 * @return     sign of the success of the passing
					 *
					 * \~
					 */
					bool field(const bool flag) noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения записи таблицы
					 *
					 * @details Запись, ни одного поля не получившая, в таблицу НЕ ложится:
					 *          у наречия пустая запись неотличима от записи с одним пустым
					 *          полем, и ложись она в таблицу - круг разошёлся бы
					 *
					 * @return признак успешности завершения записи
					 *
					 * \~english
					 * @brief Method of the finishing of a record of the table
					 * @return sign of the success of the finishing of the record
					 *
					 * \~
					 */
					bool close() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества полей открытой записи
					 *
					 * @return количество полей открытой записи
					 *
					 * \~english
					 * @brief Method of the extraction of the quantity of the fields of an open record
					 * @return quantity of the fields of an open record
					 *
					 * \~
					 */
					size_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения сборки таблицы
					 *
					 * @warning Запись, не закрытая ходом `close`, отвечает ОТКАЗОМ, а не
					 * закрывается сама: закрой мы её молча, и потеря вызова `close` у
					 * потребителя проходила бы незамеченной, а таблица выходила бы верной
					 * по виду с записью, полей недобравшей
					 *
					 * @return собранное значение таблицы
					 *
					 * \~english
					 * @brief Method of the finishing of the assembly of the table
					 * @return assembled value of the table
					 *
					 * \~
					 */
					Value finish() noexcept;
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

#endif // __AWH_CODEC_CSV_VALUE__
