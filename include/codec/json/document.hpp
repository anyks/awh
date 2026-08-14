/**
 * @file document.hpp
 * @date 2026-08-14
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
 * @brief Заголовочный файл документа JSON — дерево, удерживаемое целиком, обход его,
 *        извлечение значений и перезапись в текст
 *
 * \~english
 * @brief Header file of a JSON document — the tree held in full, its traversal,
 *        the extraction of the values and the rewriting into a text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_JSON_DOCUMENT__
#define __AWH_CODEC_JSON_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

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
			 * @brief Документ JSON, удерживаемый целиком
			 *
			 * @details Дерево хранится **сплошным перечнем узлов**, а не россыпью объектов,
			 * связанных указателями. Дети лежат сразу за родителем, указаний на них нет
			 * вовсе, а узел-вместилище хранит размер всего своего поддерева: переход к
			 * соседу и пропуск вложенного вместилища целиком стоят одного сложения.
			 * Знаки всех строк и имён лежат в одном хранилище, оттого имя поля не выделяет
			 * памяти вовсе
			 *
			 * @details **Обращение к значениям.** Обращение по имени и по номеру отдаёт
			 * лёгкую ссылку на узел, а не узел с памятью. Обращение к отсутствующему имени
			 * при чтении **ничего не заводит** и отказом не завершается: отдаётся
			 * недействительная ссылка, у какой `valid()` ложно. Оттого цепочка обращений
			 * вида `json["a"]["b"][3]` безопасна на всякой глубине
			 *
			 * @note Ссылка на узел живёт, пока документ не изменён: правка вправе
			 * перестроить перечень узлов, и ссылки после неё недействительны
			 *
			 * \~english
			 * @brief JSON document held in full
			 * @details The tree is held by a **continuous list of the nodes** rather than by a scattering of the objects
			 * linked by the pointers. The children lie right after the parent, there are no pointers to them
			 * at all, while a container node holds the size of its whole subtree: the transition to
			 * a sibling and the skipping of a nested container as a whole cost one addition.
			 * The characters of all the strings and of the names lie in one storage, whereby the name of a field does not allocate
			 * memory at all
			 * @details **Access to the values.** An access by a name and by an index gives away
			 * a light reference to a node rather than a node with a memory. An access to an absent name
			 * at the reading **creates nothing** and does not end with a refusal: an
			 * invalid reference is given away for which `valid()` is false. Whereby a chain of the accesses
			 * of the kind `json["a"]["b"][3]` is safe at any depth
			 * @note A reference to a node lives while the document is not modified: an editing may
			 * rebuild the list of the nodes, and the references after it are invalid
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				public:
					/**
					 * \~russian
					 * @brief Настройки документа
					 *
					 * \~english
					 * @brief Settings of a document
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Настройки разбора текста
						reader_t::settings_t reader;
						// Настройки записи текста
						writer_t::settings_t writer;
						/**
						 * \~russian
						 * Правило обращения с повторяющимся именем поля объекта
						 *
						 * @note Правило это принадлежит документу, а не разбору: разбор
						 * событий имена не удерживает вовсе, а сличать их можно лишь у
						 * собранного объекта
						 *
						 * \~english
						 * Rule of the handling of a repeating name of a field of an object
						 * @note This rule belongs to the document rather than to the parsing: the parsing
						 * of the events does not hold the names at all, while they can be compared only at
						 * an assembled object
						 *
						 * \~
						 */
						duplicate_t duplicates;
						// Правило преобразования чисел при разборе
						number_t numbers;
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
						Settings() noexcept :
						 duplicates(duplicate_t::ERROR), numbers(number_t::LAZY) {}
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Узел документа
					 *
					 * @details Узел занимает двадцать байтов и указаний на другие узлы не несёт
					 * вовсе: содержимое, имя поля и дети находятся счётом. Имя поля лежит в
					 * хранилище знаков вплотную перед содержимым, оттого одного смещения
					 * хватает обоим
					 *
					 * @details Длина содержимого и количество детей делят одно поле: у
					 * вместилища своего содержимого нет, а у прочих узлов нет детей
					 *
					 * \~english
					 * @brief Node of a document
					 * @details A node occupies twenty bytes and does not carry any pointers to the other nodes
					 * at all: the content, the name of a field and the children are found by a counting. The name of a field lies in
					 * the storage of the characters right before the content, whereby one offset
					 * suffices for both
					 * @details The length of the content and the number of the children share one field: a
					 * container has no content of its own, while the other nodes have no children
					 *
					 * \~
					 */
					typedef struct Node {
						// Вид узла документа
						kind_t kind;
						// Признак того, что содержимое было изменено разбором
						bool modified;
						/**
						 * \~russian
						 * Признак того, что узел является полем объекта
						 *
						 * @note Признак этот необходим: длина имени поля обращается в ноль у
						 * поля с пустым именем, а такое имя стандартом дозволено. Без признака
						 * поле `""` было бы неотличимо от значения массива, и перезапись
						 * документа теряла бы охватывающий его объект
						 *
						 * \~english
						 * Flag that the node is a field of an object
						 * @note This flag is necessary: the length of the name of a field turns into zero for
						 * a field with an empty name, while such a name is allowed by the standard. Without the flag
						 * a field `""` would be indistinguishable from a value of an array, and the rewriting
						 * of the document would lose the object enclosing it
						 *
						 * \~
						 */
						bool keyed;
						/**
						 * \~russian
						 * Количество детей вместилища либо длина содержимого в байтах
						 *
						 * \~english
						 * Number of the children of a container or the length of the content in bytes
						 *
						 * \~
						 */
						uint32_t length;
						/**
						 * \~russian
						 * Количество узлов поддерева, включая сам узел
						 *
						 * @note Служит для пропуска вложенного вместилища целиком за одно
						 * сложение
						 *
						 * \~english
						 * Number of the nodes of the subtree including the node itself
						 * @note Serves for the skipping of a nested container as a whole by one
						 * addition
						 *
						 * \~
						 */
						uint32_t extent;
						// Смещение содержимого либо имени поля в хранилище знаков
						uint32_t offset;
						// Длина имени поля объекта в байтах
						uint32_t named;
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
						Node() noexcept :
						 kind(kind_t::NONE), modified(false), keyed(false),
						 length(0), extent(1), offset(0), named(0) {}
					} node_t;
				public:
					/**
					 * \~russian
					 * @brief Ссылка на узел документа
					 *
					 * @details Ссылка легка: она хранит указатель на документ и номер узла.
					 * Обращение по имени и по номеру отдаёт такую же ссылку, отчего цепочка
					 * обращений памяти не выделяет вовсе
					 *
					 * \~english
					 * @brief Reference to a node of a document
					 * @details The reference is light: it holds a pointer to the document and the index of the node.
					 * An access by a name and by an index gives away the same reference, whereby a chain
					 * of the accesses does not allocate any memory at all
					 *
					 * \~
					 */
					typedef class __AWH_SHARED_EXPORT__ Value {
						private:
							// Документ, которому принадлежит узел
							const Document * _doc;
						private:
							// Номер узла в перечне узлов документа
							uint32_t _index;
						private:
							/**
							 * \~russian
							 * Номер узла за последним узлом вместилища, какому узел принадлежит
							 *
							 * @details Указаний на родителя узел не несёт, а переход к соседу
							 * обязан останавливаться на границе своего вместилища: без границы
							 * обход массива продолжился бы соседом его родителя
							 *
							 * \~english
							 * Index of the node past the last node of the container to which the node belongs
							 * @details A node does not carry any pointer to its parent, while the transition to a sibling
							 * is obliged to stop at the boundary of its container: without the boundary
							 * the traversal of an array would continue with the sibling of its parent
							 *
							 * \~
							 */
							uint32_t _bound;
						public:
							/**
							 * \~russian
							 * @brief Метод проверки действительности ссылки
							 *
							 * @return признак действительности ссылки
							 *
							 * \~english
							 * @brief Method of the check of the validity of the reference
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
							 * @brief Method of the extraction of the kind of the node
							 * @return kind of the node of the document
							 *
							 * \~
							 */
							kind_t kind() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения количества детей вместилища
							 *
							 * @return количество детей вместилища
							 *
							 * \~english
							 * @brief Method of the extraction of the number of the children of a container
							 * @return number of the children of the container
							 *
							 * \~
							 */
							size_t size() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки вместилища на пустоту
							 *
							 * @return признак отсутствия детей у вместилища
							 *
							 * \~english
							 * @brief Method of the check of a container for the emptiness
							 * @return sign of the absence of the children of the container
							 *
							 * \~
							 */
							bool empty() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени поля объекта
							 *
							 * @return имя поля объекта, пусто у прочих узлов
							 *
							 * \~english
							 * @brief Method of the extraction of the name of a field of an object
							 * @return name of the field of the object, empty for the other nodes
							 *
							 * \~
							 */
							string_view name() const noexcept;
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
							 * @brief Метод обращения к полю объекта по имени
							 *
							 * @details Обращение к отсутствующему имени ничего не заводит и
							 * отдаёт недействительную ссылку
							 *
							 * @param name имя поля объекта
							 * @return     ссылка на узел поля объекта
							 *
							 * \~english
							 * @brief Method of the access to a field of an object by a name
							 * @details An access to an absent name creates nothing and
							 * gives away an invalid reference
							 * @param name name of the field of the object
							 * @return reference to the node of the field of the object
							 *
							 * \~
							 */
							Value operator [] (const string & name) const noexcept;
							/**
							 * \~russian
							 * @brief Метод обращения к значению вместилища по номеру
							 *
							 * @param index номер значения во вместилище
							 * @return      ссылка на узел значения
							 *
							 * \~english
							 * @brief Method of the access to a value of a container by an index
							 * @param index index of the value in the container
							 * @return reference to the node of the value
							 *
							 * \~
							 */
							Value operator [] (const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод обращения к значению по указателю JSON Pointer
							 *
							 * @details Указатель записывается по RFC 6901: `/response/users/0/id`
							 *
							 * @param pointer указатель на значение по RFC 6901
							 * @return        ссылка на узел значения
							 *
							 * \~english
							 * @brief Method of the access to a value by a JSON Pointer
							 * @details The pointer is written according to RFC 6901: `/response/users/0/id`
							 * @param pointer pointer to the value according to RFC 6901
							 * @return reference to the node of the value
							 *
							 * \~
							 */
							Value at(const string & pointer) const noexcept;
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
							 * @brief Метод извлечения целого числа
							 *
							 * @details Отказом завершается и тогда, когда число целым не
							 * представимо: `1e400` и `1.5` целым не выдаются
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of an integer
							 * @details It ends with a refusal also when the number is not representable
							 * as an integer: `1e400` and `1.5` are not issued as an integer
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения беззнакового целого числа
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа с плавающей запятой
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a floating-point number
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(double & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения строкового значения
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a string value
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(string & result) const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения записи числа как она есть
							 *
							 * @details Запись выдаётся без преобразования: она годится и для
							 * переноса числа в другой документ без потери точности, и для
							 * разбора видом, какого у контейнера нет
							 *
							 * @return запись числа, пусто у прочих узлов
							 *
							 * \~english
							 * @brief Method of the extraction of the record of a number as it is
							 * @details The record is issued without a conversion: it is suitable both for
							 * the transfer of the number into another document without a loss of the precision and for
							 * a parsing by a kind which the container does not have
							 * @return record of the number, empty for the other nodes
							 *
							 * \~
							 */
							string_view raw() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения строкового значения без копирования
							 *
							 * @return строковое значение, пусто у прочих узлов
							 *
							 * \~english
							 * @brief Method of the extraction of a string value without a copying
							 * @return string value, empty for the other nodes
							 *
							 * \~
							 */
							string_view text() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод перехода к следующему значению вместилища
							 *
							 * @details Переход стоит одного сложения: вложенное вместилище
							 * пропускается целиком по размеру своего поддерева
							 *
							 * @return ссылка на следующее значение вместилища
							 *
							 * \~english
							 * @brief Method of the transition to the next value of a container
							 * @details The transition costs one addition: a nested container
							 * is skipped as a whole by the size of its subtree
							 * @return reference to the next value of the container
							 *
							 * \~
							 */
							Value next() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения первого значения вместилища
							 *
							 * @return ссылка на первое значение вместилища
							 *
							 * \~english
							 * @brief Method of the extraction of the first value of a container
							 * @return reference to the first value of the container
							 *
							 * \~
							 */
							Value begin() const noexcept;
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
							Value() noexcept : _doc(nullptr), _index(NO_INDEX), _bound(0) {}
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
							 * @param doc document to which the node belongs
							 * @param index index of the node in the list of the nodes of the document
							 * @param bound index of the node past the last node of the container
							 *
							 * \~
							 */
							Value(const Document * doc, const uint32_t index, const uint32_t bound) noexcept : _doc(doc), _index(index), _bound(bound) {}
					} value_t;
					/**
					 * \~russian
					 * @brief Обработчик потоковой выдачи значений
					 *
					 * @details Вызывается на всякое разобранное значение верхнего уровня.
					 * Возврат лжи прекращает разбор
					 *
					 * \~english
					 * @brief Handler of the streaming issuance of the values
					 * @details Is called for every parsed value of the top level.
					 * A return of a falsehood stops the parsing
					 *
					 * \~
					 */
					typedef function <bool (const value_t &)> callback_t;
				private:
					// Настройки документа
					settings_t _settings;
				private:
					// Код отказа разбора
					error_t _error;
				private:
					/**
					 * \~russian
					 * Перечень узлов документа
					 *
					 * @details Дети лежат сразу за родителем, оттого обход идёт вперёд по
					 * памяти
					 *
					 * \~english
					 * List of the nodes of the document
					 * @details The children lie right after the parent, whereby the traversal goes forward through
					 * the memory
					 *
					 * \~
					 */
					vector <node_t> _nodes;
				private:
					// Хранилище знаков всех строк и имён документа
					string _storage;
				private:
					/**
					 * \~russian
					 * Отображение имён полей в номера узлов, заводимое по требованию
					 *
					 * @details Заводится при первом обращении по имени к объекту, число
					 * полей какого превышает порог. Мелкие объекты, каких большинство, не
					 * платят за него ничего
					 *
					 * \~english
					 * Mapping of the names of the fields into the indices of the nodes created on demand
					 * @details Is created at the first access by a name to an object the number
					 * of the fields of which exceeds the threshold. The small objects, of which there are the majority, do not
					 * pay anything for it
					 *
					 * \~
					 */
					mutable unordered_map <uint32_t, unordered_map <string_view, uint32_t>> _index;
				private:
					// Положение отказа разбора в исходном тексте
					location_t _position;
				private:
					/**
					 * \~russian
					 * Стек номеров узлов открытых вместилищ
					 *
					 * @note Стек этот принадлежит документу, а не сборке: текст подаётся
					 * кусками, и сборка вызывается на всякий кусок заново. Стек, заведённый
					 * внутри сборки, терялся бы на границе кусков вместе с деревом
					 *
					 * \~english
					 * Stack of the indices of the nodes of the opened containers
					 * @note This stack belongs to the document rather than to the assembly: the text is fed
					 * by chunks, and the assembly is called anew for every chunk. A stack created
					 * inside the assembly would be lost at the boundary of the chunks together with the tree
					 *
					 * \~
					 */
					vector <uint32_t> _nesting;
				private:
					// Длина имени поля объекта, ожидающего своего значения
					uint32_t _named;
				private:
					// Признак того, что имя поля объекта разобрано, а значение его - ещё нет
					bool _keyed;
				private:
					/**
					 * \~russian
					 * @brief Метод сборки дерева по событиям разбора
					 *
					 * @param reader   объект потокового чтения текста
					 * @param callback обработчик потоковой выдачи значений
					 * @return         признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembly of the tree by the events of the parsing
					 * @param reader object of the streaming reading of a text
					 * @param callback handler of the streaming issuance of the values
					 * @return sign of the success of the assembly
					 *
					 * \~
					 */
					bool consume(reader_t & reader, const callback_t & callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора повторяющихся имён полей объекта
					 *
					 * @details Вызывается при закрытии объекта, когда все поля его уже
					 * собраны: разбор событий имена не удерживает вовсе, и сличать их
					 * можно лишь у собранного объекта
					 *
					 * @param parent номер узла разбираемого объекта
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the analysis of the repeating names of the fields of an object
					 * @details Is called at the closing of an object when all of its fields are already
					 * assembled: the parsing of the events does not hold the names at all, and they
					 * can be compared only at an assembled object
					 * @param parent index of the node of the object being analyzed
					 * @return sign of the success of the analysis
					 *
					 * \~
					 */
					bool deduplicate(const uint32_t parent) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод очистки документа
					 *
					 * \~english
					 * @brief Method of the clearing of the document
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста документа
					 *
					 * @param text разбираемый текст документа
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the text of a document
					 * @param text text of the document being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод потоковой выдачи значений разбираемого текста
					 *
					 * @details Документ при этом не заполняется вовсе: обработчику выдаётся
					 * очередное значение верхнего уровня, а память под него переиспользуется.
					 * Пригодно для потока NDJSON и для крупных массивов
					 *
					 * @param text     разбираемый текст документа
					 * @param callback обработчик потоковой выдачи значений
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the streaming issuance of the values of a text being parsed
					 * @details The document is thereby not filled at all: the next value of the top level
					 * is issued to the handler, while the memory for it is reused.
					 * Suitable for an NDJSON stream and for the large arrays
					 * @param text text of the document being parsed
					 * @param callback handler of the streaming issuance of the values
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const callback_t & callback) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста документа из файла
					 *
					 * @param filename адрес разбираемого файла
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the text of a document from a file
					 * @param filename address of the file being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перезаписи документа в текст
					 *
					 * @param format вид оформления собираемого текста
					 * @return       собранный текст документа
					 *
					 * \~english
					 * @brief Method of the rewriting of the document into a text
					 * @param format kind of the formatting of the text being assembled
					 * @return assembled text of the document
					 *
					 * \~
					 */
					string dump(const format_t format = format_t::COMPACT) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи документа в файл
					 *
					 * @param filename адрес записываемого файла
					 * @param format   вид оформления собираемого текста
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the document into a file
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
					 * @brief Метод извлечения корневого значения документа
					 *
					 * @return ссылка на корневое значение документа
					 *
					 * \~english
					 * @brief Method of the extraction of the root value of the document
					 * @return reference to the root value of the document
					 *
					 * \~
					 */
					value_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к полю корневого объекта по имени
					 *
					 * @param name имя поля корневого объекта
					 * @return     ссылка на узел поля объекта
					 *
					 * \~english
					 * @brief Method of the access to a field of the root object by a name
					 * @param name name of the field of the root object
					 * @return reference to the node of the field of the object
					 *
					 * \~
					 */
					value_t operator [] (const string & name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению корневого массива по номеру
					 *
					 * @param index номер значения в корневом массиве
					 * @return      ссылка на узел значения
					 *
					 * \~english
					 * @brief Method of the access to a value of the root array by an index
					 * @param index index of the value in the root array
					 * @return reference to the node of the value
					 *
					 * \~
					 */
					value_t operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к значению по указателю JSON Pointer
					 *
					 * @param pointer указатель на значение по RFC 6901
					 * @return        ссылка на узел значения
					 *
					 * \~english
					 * @brief Method of the access to a value by a JSON Pointer
					 * @param pointer pointer to the value according to RFC 6901
					 * @return reference to the node of the value
					 *
					 * \~
					 */
					value_t at(const string & pointer) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества узлов документа
					 *
					 * @return количество узлов документа
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the nodes of the document
					 * @return number of the nodes of the document
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки документа на пустоту
					 *
					 * @return признак отсутствия узлов в документе
					 *
					 * \~english
					 * @brief Method of the check of the document for the emptiness
					 * @return sign of the absence of the nodes in the document
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа разбора
					 *
					 * @return код отказа разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the parsing
					 * @return error code of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения положения отказа разбора в исходном тексте
					 *
					 * @return положение отказа разбора в исходном тексте
					 *
					 * \~english
					 * @brief Method of the extraction of the position of the refusal of the parsing in the source text
					 * @return position of the refusal of the parsing in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения настроек документа
					 *
					 * @return настройки документа
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the document
					 * @return settings of the document
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек документа
					 *
					 * @param settings устанавливаемые настройки документа
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the document
					 * @param settings settings of the document being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
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
					Document() noexcept;
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
					~Document() noexcept {}
			} document_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_JSON_DOCUMENT__
