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
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "document.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
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
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
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
					/**
					 * \~russian
					 * @brief Метод сборки записи из владеющего значения
					 *
					 * @return собранная запись
					 *
					 * \~english
					 * @brief Method of the assembling of a record from an owning value
					 * @return assembled record
					 *
					 * \~
					 */
					vector <uint8_t> dump() const noexcept;
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
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_ABC_VALUE__
