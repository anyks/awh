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
#include <cstring>
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
			 * @brief Предварительное объявление владеющего значения
			 *
			 * @details Объявление это заведено ради прививки: документ прививаемое значение
			 * принимает, а определение его лежит в заголовке, какой сам подключает документ.
			 * Полное объявление есть в `value.hpp`
			 *
			 * \~english
			 * @brief Forward declaration of the owning value
			 * @details This declaration is made for the sake of the grafting: a document accepts a value
			 * being grafted, while its definition lies in a header which itself includes the document.
			 * The full declaration is in `value.hpp`
			 *
			 * \~
			 */
			class Value;

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
			 * @note **Разбирающему много документов подряд** надлежит держать один объект
			 * документа, а не заводить его на всякий текст: вместилища дерева и хранилище
			 * знаков переживают разбор и памяти своей не отдают. На мелких документах, каких
			 * у служб большинство, переиспользование объекта даёт до полутора раз скорости -
			 * заведение вместилищ с нуля на документ длиною в сотню байтов стоит дороже
			 * самого разбора его
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
						 duplicates(duplicate_t::ERROR), numbers(number_t::NATIVE) {}
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
						// Вид значения узла документа
						type_t type;
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
						// Смещение содержимого либо имени поля в хранилище знаков
						uint32_t offset;
						// Длина имени поля объекта в байтах
						uint32_t named;
						/**
						 * \~russian
						 * Содержимое узла шириною в восемь байтов
						 *
						 * @details Восемь этих байтов служат узлу по-разному, смотря по виду
						 * значения его. У вместилища это пара чисел: количество детей и размах
						 * поддерева. У строки - длина содержимого. У числа родного вида - само
						 * число, готовое к выдаче. У числа вида `EXTENDED` - длина записи его
						 *
						 * @note Число кладётся сюда переносом байтов, а не отдельным полем на
						 * восемь байтов: поле такое потребовало бы выравнивания по восьми
						 * байтам, а с ним узел вырос бы с двадцати байтов до двадцати четырёх.
						 * На дереве в два миллиона узлов это семь мегабайтов впустую
						 *
						 * \~english
						 * Content of the node eight bytes wide
						 * @details These eight bytes serve the node differently, depending on the kind
						 * of its value. For a container it is a pair of numbers: the number of the children and the extent
						 * of the subtree. For a string it is the length of the content. For a number of a native kind it is
						 * the number itself ready for the issuance. For a number of the kind `EXTENDED` it is the length of its record
						 * @note A number is placed here by a transfer of the bytes rather than by a separate field of
						 * eight bytes: such a field would demand an alignment by eight
						 * bytes, and with it the node would grow from twenty bytes to twenty four.
						 * On a tree of two million nodes that is seven megabytes wasted
						 *
						 * \~
						 */
						uint32_t content[2];
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместилища либо длины содержимого
						 *
						 * @return количество детей вместилища либо длина содержимого в байтах
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the content
						 * @return number of the children of a container or the length of the content in bytes
						 *
						 * \~
						 */
						AWH_JSON_INLINE uint32_t length() const noexcept {
							// Выводим количество детей вместилища либо длину содержимого
							return this->content[0];
						}
						/**
						 * \~russian
						 * @brief Метод получения количества узлов поддерева, включая сам узел
						 *
						 * @details Размах хранится лишь у вместилища: у всякого прочего узла он
						 * равен единице всегда, и место под него занято самим значением
						 *
						 * @return количество узлов поддерева, включая сам узел
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the nodes of the subtree including the node itself
						 * @details The extent is stored only for a container: for every other node it is
						 * always equal to one, and the place for it is occupied by the value itself
						 * @return number of the nodes of the subtree including the node itself
						 *
						 * \~
						 */
						AWH_JSON_INLINE uint32_t extent() const noexcept {
							// Выводим размах поддерева вместилища либо единицу у прочих узлов
							return (this->nested() ? this->content[1] : 1);
						}
						/**
						 * \~russian
						 * @brief Метод установки количества детей вместилища либо длины содержимого
						 *
						 * @param value устанавливаемое количество детей либо длина содержимого
						 *
						 * \~english
						 * @brief Method of the setting of the number of the children of a container or of the length of the content
						 * @param value number of the children or length of the content being set
						 *
						 * \~
						 */
						AWH_JSON_INLINE void length(const uint32_t value) noexcept {
							// Устанавливаем количество детей вместилища либо длину содержимого
							this->content[0] = value;
						}
						/**
						 * \~russian
						 * @brief Метод установки количества узлов поддерева, включая сам узел
						 *
						 * @note Размах устанавливается лишь вместилищу: у прочих узлов место
						 *       это занято значением, и запись в него значение бы и погубила
						 *
						 * @param value устанавливаемое количество узлов поддерева
						 *
						 * \~english
						 * @brief Method of the setting of the number of the nodes of the subtree including the node itself
						 * @note The extent is set only for a container: for the other nodes this place
						 *       is occupied by the value, and a writing into it would ruin the value itself
						 * @param value number of the nodes of the subtree being set
						 *
						 * \~
						 */
						AWH_JSON_INLINE void extent(const uint32_t value) noexcept {
							/**
							 * Если узел является вместилищем
							 */
							if(this->nested())
								// Устанавливаем количество узлов поддерева вместилища
								this->content[1] = value;
						}
						/**
						 * \~russian
						 * @brief Метод проверки числа на хранение самим узлом
						 *
						 * @details Число родного вида лежит в самом узле готовым, а число вида
						 * `EXTENDED` - записью своей в хранилище знаков. Место в узле у них
						 * занято разным, и путать эти два случая нельзя
						 *
						 * @return признак того, что число хранится самим узлом
						 *
						 * \~english
						 * @brief Method of the checking of a number for being stored by the node itself
						 * @details A number of a native kind lies in the node itself ready, while a number of the kind
						 * `EXTENDED` lies as its record in the storage of the characters. The place in the node is
						 * occupied by different things for them, and these two cases must not be confused
						 * @return flag that the number is stored by the node itself
						 *
						 * \~
						 */
						AWH_JSON_INLINE bool native() const noexcept {
							// Получаем разряды вида значения узла
							const uint16_t mask = static_cast <uint16_t> (this->type);
							// Выводим признак того, что число хранится самим узлом
							return (((mask & static_cast <uint16_t> (type_t::NUMBER)) != 0) && (this->type != type_t::EXTENDED));
						}
						/**
						 * \~russian
						 * @brief Метод проверки узла на принадлежность к вместилищам
						 *
						 * @return признак того, что узел является массивом либо объектом
						 *
						 * \~english
						 * @brief Method of the checking of a node for the belonging to the containers
						 * @return flag that the node is an array or an object
						 *
						 * \~
						 */
						AWH_JSON_INLINE bool nested() const noexcept {
							// Выводим признак принадлежности узла к вместилищам
							return ((static_cast <uint16_t> (this->type) & (static_cast <uint16_t> (type_t::ARRAY) | static_cast <uint16_t> (type_t::OBJECT))) != 0);
						}
						/**
						 * \~russian
						 * @brief Шаблонный метод получения числа, хранимого узлом
						 *
						 * @tparam T вид хранимого узлом числа
						 * @return   число, хранимое узлом
						 *
						 * \~english
						 * @brief Template method of the obtaining of the number stored by the node
						 * @tparam T kind of the number stored by the node
						 * @return number stored by the node
						 *
						 * \~
						 */
						template <typename T>
						AWH_JSON_INLINE T number() const noexcept {
							// Извлекаемое из узла число
							T result;
							// Выполняем перенос байтов числа из содержимого узла
							::memcpy(&result, this->content, sizeof(T));
							// Выводим извлечённое из узла число
							return result;
						}
						/**
						 * \~russian
						 * @brief Шаблонный метод установки числа, хранимого узлом
						 *
						 * @tparam T     вид хранимого узлом числа
						 * @param  value устанавливаемое число
						 *
						 * \~english
						 * @brief Template method of the setting of the number stored by the node
						 * @tparam T kind of the number stored by the node
						 * @param value number being set
						 *
						 * \~
						 */
						template <typename T>
						AWH_JSON_INLINE void number(const T value) noexcept {
							// Выполняем обнуление содержимого узла
							this->content[0] = 0;
							// Выполняем обнуление старшей половины содержимого узла
							this->content[1] = 0;
							// Выполняем перенос байтов числа в содержимое узла
							::memcpy(this->content, &value, sizeof(T));
						}
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
						 type(type_t::UNDEFINED), modified(false), keyed(false),
						 offset(0), named(0), content{0, 0} {}
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
							AWH_JSON_INLINE bool valid() const noexcept {
								// Выводим признак действительности ссылки
								return ((this->_doc != nullptr) && (this->_index < this->_doc->_nodes.size()));
							}
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
							AWH_JSON_INLINE kind_t kind() const noexcept {
								// Выводим вид узла документа, если ссылка действительна
								return (this->valid() ? json::kind(this->_doc->_nodes[this->_index].type) : kind_t::NONE);
							}
							/**
							 * \~russian
							 * @brief Метод извлечения вида значения
							 *
							 * @details Вид этот точен: число выдаёт тот самый вид, каким оно
							 * хранится, - от `INT8` до `DOUBLE`
							 *
							 * @return вид значения документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of the value
							 * @details This kind is exact: a number issues that very kind by which it
							 * is stored — from `INT8` to `DOUBLE`
							 * @return kind of the value of the document
							 *
							 * \~
							 */
							AWH_JSON_INLINE type_t type() const noexcept {
								// Выводим вид значения документа, если ссылка действительна
								return (this->valid() ? this->_doc->_nodes[this->_index].type : type_t::UNDEFINED);
							}
							/**
							 * \~russian
							 * @brief Метод проверки значения на принадлежность к виду
							 *
							 * @details Проверка идёт наложением разрядов, оттого точный вопрос
							 * `is(type_t::INT32)` и сборный `is(type_t::NUMBER)` стоят одинаково
							 *
							 * @note Вопрос о нескольких видах разом задаётся сборным видом, а не
							 *       несколькими вызовами: `is(type_t::REAL)` истинен и у `FLOAT`,
							 *       и у `DOUBLE`
							 *
							 * @param type вид либо набор видов, на принадлежность к какому
							 *             проверяется значение
							 * @return     признак принадлежности значения к виду
							 *
							 * \~english
							 * @brief Method of the checking of a value for the belonging to a kind
							 * @details The checking goes by an overlaying of the bits, whereby an exact question
							 * `is(type_t::INT32)` and a composite one `is(type_t::NUMBER)` cost the same
							 * @note A question about several kinds at once is asked by a composite kind rather than by
							 *       several calls: `is(type_t::REAL)` is true both for `FLOAT`
							 *       and for `DOUBLE`
							 * @param type kind or set of the kinds for the belonging to which
							 *             the value is checked
							 * @return flag of the belonging of the value to the kind
							 *
							 * \~
							 */
							AWH_JSON_INLINE bool is(const type_t type) const noexcept {
								/**
								 * Если спрошено об отсутствии значения
								 *
								 * @note Отсутствие значения разряда своего не имеет вовсе - оно есть
								 *       пустота разрядов, и наложением проверено быть не может
								 */
								if(type == type_t::UNDEFINED)
									// Выводим признак отсутствия значения
									return (this->type() == type_t::UNDEFINED);
								// Выводим признак принадлежности значения к виду
								return ((static_cast <uint16_t> (this->type()) & static_cast <uint16_t> (type)) != 0);
							}
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
							AWH_JSON_INLINE size_t size() const noexcept {
								/**
								 * Если ссылка недействительна
								 */
								if(!this->valid())
									// Выводим отсутствие детей у вместилища
									return 0;
								// Получаем узел, на какой указывает ссылка
								const Node & node = this->_doc->_nodes[this->_index];
								/**
								 * Выводим количество детей вместилища, а у прочих узлов - отсутствие детей
								 *
								 * @note Поле длины у вместилища и у прочих узлов занято разным, и выдавать
								 *       длину строки количеством детей означало бы завести обход по числу
								 *       байтов её содержимого
								 */
								return (node.nested() ? static_cast <size_t> (node.length()) : 0);
							}
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
							AWH_JSON_INLINE bool empty() const noexcept {
								// Выводим признак отсутствия детей у вместилища
								return (this->size() == 0);
							}
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
							AWH_JSON_INLINE string_view name() const noexcept {
								/**
								 * Если ссылка недействительна
								 */
								if(!this->valid())
									// Выводим отсутствие имени поля объекта
									return string_view();
								// Получаем узел, на какой указывает ссылка
								const Node & node = this->_doc->_nodes[this->_index];
								/**
								 * Если узел полем объекта не является
								 */
								if(!node.keyed)
									// Выводим отсутствие имени поля объекта
									return string_view();
								// Выводим имя поля объекта, лежащее вплотную перед содержимым
								return string_view(this->_doc->_storage.data() + (node.offset - node.named), node.named);
							}
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
						private:
							/**
							 * \~russian
							 * @brief Шаблонный метод извлечения числа затребованным видом
							 *
							 * @details Извлечение сличает само значение с пределами затребованного
							 * вида, а не вид хранения с видом затребованным
							 *
							 * @tparam T      затребованный вид числа
							 * @param  result переменная, куда помещается извлечённое значение
							 * @return        признак успешности извлечения
							 *
							 * \~english
							 * @brief Template method of the extraction of a number by the demanded kind
							 * @details The extraction compares the value itself with the limits of the demanded
							 * kind rather than the kind of the storage with the demanded kind
							 * @tparam T demanded kind of the number
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							template <typename T>
							bool extract(T & result) const noexcept;
						public:
							bool value(bool & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа
							 *
							 * @details Отказом извлечение завершается лишь тогда, когда узел
							 * числом не является вовсе. Вид хранения извлечению не указ: узел,
							 * хранящий `INT8`, извлекается и как `double`, и как `uint64_t`
							 *
							 * @details Целое, не вмещающееся в затребованный вид, переносится
							 * младшими разрядами - ровно так, как это делает `static_cast`. Дробное
							 * же ОКРУГЛЯЕТСЯ по правилам математики с уводом половины от нуля:
							 * `1.5` даёт `2`, `-1.5` даёт `-2`, `1.4` даёт `1`
							 *
							 * @note Единственное отступление от `static_cast` касается дробного,
							 *       чья целая часть лежит за пределами затребованного вида, - вроде
							 *       `1e300` в `int32_t`. Стандарт зовёт это неопределённым
							 *       поведением, а мы выдаём предел затребованного вида: неопределённого
							 *       поведения в кодеке не будет
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number
							 * @details The extraction ends with a refusal only when the node
							 * is not a number at all. The kind of the storage is not a directive to the extraction: a node
							 * storing an `INT8` is extracted both as a `double` and as a `uint64_t`
							 * @details An integer not fitting into the demanded kind is transferred by the lower
							 * bits — exactly as `static_cast` does it. A fractional number, however, is ROUNDED
							 * by the rules of mathematics with the half taken away from zero:
							 * `1.5` gives `2`, `-1.5` gives `-2`, `1.4` gives `1`
							 * @note The only deviation from `static_cast` concerns a fractional number
							 *       whose integer part lies beyond the limits of the demanded kind — like
							 *       `1e300` into an `int32_t`. The standard calls this an undefined
							 *       behaviour, while we issue the limit of the demanded kind: there will be no undefined
							 *       behaviour in the codec
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `int16_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `int16_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `int32_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `int32_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `int64_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `int64_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `uint8_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `uint8_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `uint16_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `uint16_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `uint32_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `uint32_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `uint64_t`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `uint64_t`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `float`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `float`
							 * @param result variable where the extracted value is placed
							 * @return sign of the success of the extraction
							 *
							 * \~
							 */
							bool value(float & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения числа видом `double`
							 *
							 * @param result переменная, куда помещается извлечённое значение
							 * @return       признак успешности извлечения
							 *
							 * \~english
							 * @brief Method of the extraction of a number by the kind `double`
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
							 * @brief Метод извлечения записи числа
							 *
							 * @details Запись собирается из хранимого узлом числа кратчайшей
							 * записью, читающейся обратно тем же самым числом. Число вида
							 * `EXTENDED` выдаёт запись свою дословно, как она стояла в тексте
							 *
							 * @note Дословного совпадения с исходным текстом запись не обещает:
							 *       разбор хранит число, а не знаки его, и `1.50` выдаётся как
							 *       `1.5`, а `1e2` - как `100`. Значение при этом то же самое
							 *
							 * @return запись числа, пусто у прочих узлов
							 *
							 * \~english
							 * @brief Method of the extraction of the record of a number
							 * @details The record is assembled from the number stored by the node by the shortest
							 * record which is read back as the very same number. A number of the kind
							 * `EXTENDED` issues its record verbatim, as it stood in the text
							 * @note The record does not promise a verbatim coincidence with the source text:
							 *       the parsing stores the number rather than its characters, and `1.50` is issued as
							 *       `1.5`, while `1e2` as `100`. The value at that is the very same
							 * @return record of the number, empty for the other nodes
							 *
							 * \~
							 */
							string raw() const noexcept;
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
							AWH_JSON_INLINE Value next() const noexcept {
								/**
								 * Если ссылка недействительна
								 */
								if(!this->valid())
									// Выводим недействительную ссылку
									return Value();
								// Получаем номер следующего значения вместилища
								const uint32_t index = (this->_index + this->_doc->_nodes[this->_index].extent());
								// Выводим ссылку на следующее значение вместилища, если оно не вышло за границу
								return ((index < this->_bound) ? Value(this->_doc, index, this->_bound) : Value());
							}
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
							AWH_JSON_INLINE Value begin() const noexcept {
								/**
								 * Если вместилище пусто
								 */
								if(this->empty())
									// Выводим недействительную ссылку
									return Value();
								// Получаем узел, на какой указывает ссылка
								const Node & node = this->_doc->_nodes[this->_index];
								// Выводим ссылку на первое значение вместилища
								return Value(this->_doc, (this->_index + 1), (this->_index + node.extent()));
							}
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
					/**
					 * \~russian
					 * Чтение текста документа
					 *
					 * @note Чтение принадлежит документу, а не разбору: заведение его на всякий
					 * разбор стоило бы выделения памяти под очередь событий, хранилище знаков и
					 * стек вложенности - а на малых документах, каких у служб большинство,
					 * стоимость эта и составляет почти всё время разбора
					 *
					 * \~english
					 * Reading of the text of the document
					 * @note The reading belongs to the document rather than to the parsing: its creation
					 * for every parsing would cost an allocation of the memory for the queue of the events, the storage of the characters and
					 * the stack of the nesting — and for the small documents, of which there are the majority at the services,
					 * this cost constitutes almost all of the time of the parsing
					 *
					 * \~
					 */
					reader_t _reader;
				private:
					// Код отказа разбора
					error_t _error;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Логгер уходит и в чтение, деревом хранимое: разбор сообщает о бедах
					 *       своих сам, и дерево обязано ему это дать
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
					/**
					 * \~russian
					 * Перечень имён полей разбираемого объекта вместе с номерами их узлов
					 *
					 * @note Перечень принадлежит документу, а не разбору повторов: заведение
					 * его на всяком закрываемом объекте стоило бы выделения памяти на всякий
					 * объект документа
					 *
					 * \~english
					 * List of the names of the fields of the object being analyzed together with the indices of their nodes
					 * @note The list belongs to the document rather than to the analysis of the repetitions: its creation
					 * at every object being closed would cost an allocation of the memory for every
					 * object of the document
					 *
					 * \~
					 */
					vector <pair <string_view, uint32_t>> _naming;
				private:
					// Отображение имён полей крупного объекта в места их в перечне имён
					unordered_map <string_view, size_t> _lookup;
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
					/**
					 * \~russian
					 * Признак того, что документ уже собран целиком
					 *
					 * @details Поток несёт документы один за другим, а дерево вмещает один. Без
					 * обработчика потоковой выдачи второй документ потока уходил бы в никуда
					 * молча: узлы его ложились бы за корнем недостижимыми, а выдача текста
					 * отдавала бы первый документ признаком успеха
					 *
					 * \~english
					 * Sign of the document having already been assembled as a whole
					 * @details A stream carries the documents one after another, while a tree holds one.
					 * Without a handler of the streaming issuance the second document of a stream would
					 * go nowhere silently: its nodes would lie behind the root unreachable, while the
					 * issuance of the text would give away the first document with a sign of success
					 *
					 * \~
					 */
					bool _completed;
				private:
					/**
					 * \~russian
					 * Сквозное положение конца имени поля объекта, ожидающего своего значения
					 *
					 * @details Узел несёт одно смещение на имя и на содержимое разом: имя лежит
					 * вплотную перед содержимым. У вместилища же своего содержимого нет, и
					 * смещению его взяться неоткуда - оттого конец имени запоминается отдельно
					 *
					 * \~english
					 * Through-going position of the end of the name of the field of an object awaiting its value
					 * @details A node carries one offset for the name and for the content at once: the name lies
					 * right before the content. A container has no content of its own, and
					 * its offset has nowhere to come from — whereby the end of the name is remembered separately
					 *
					 * \~
					 */
					uint64_t _pointer;
				private:
					/**
					 * \~russian
					 * Сквозное положение первого знака хранилища документа в потоке разобранных знаков
					 *
					 * @details Разбор ведёт смещения сквозными по всему потоку, а хранилище
					 * документа при потоковой выдаче значений очищается на всяком документе.
					 * Вычитание этого положения переводит сквозное положение в смещение
					 * внутри хранилища документа
					 *
					 * \~english
					 * Through-going position of the first character of the storage of the document in the stream of the parsed characters
					 * @details The parsing keeps the offsets through-going over the whole stream, while the storage
					 * of the document at the streaming issuance of the values is cleared at every document.
					 * The subtraction of this position converts a through-going position into an offset
					 * inside the storage of the document
					 *
					 * \~
					 */
					uint64_t _base;
				private:
					/**
					 * Обработчик потоковой выдачи значений, действующий на время разбора
					 *
					 * @note Обработчик хранится указанием, а не копией: событие приходит из
					 *       чтения прямо в сборку дерева, а передать его туда доводом неоткуда
					 */
					const callback_t * _callback;
				private:
					/**
					 * \~russian
					 * @brief Метод сборки дерева по очередному событию разбора
					 *
					 * @param reader   объект потокового чтения текста
					 * @param event    вид очередного события разбора
					 * @param content  указание на содержимое события в хранилище знаков разбора
					 * @param modified признак изменения содержимого разбором
					 * @return         признак успешности сборки
					 *
					 * \~english
					 * @brief Method of the assembly of the tree by the next event of the parsing
					 * @param reader   object of the streaming reading of a text
					 * @param event    kind of the next event of the parsing
					 * @param content  pointer at the content of the event in the storage of the characters of the parsing
					 * @param modified flag of the modification of the content by the parsing
					 * @return sign of the success of the assembly
					 *
					 * \~
					 */
					bool digest(reader_t & reader, const event_t event, const span_t content, const bool modified) noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса знаков разбора в хранилище документа
					 *
					 * @details Знаки переносятся целым куском по исчерпании поданного текста, а
					 * не по одному значению: смещения узлов сквозные, и содержимое их приходит
					 * на своё место само
					 *
					 * @note Перенос по одному значению стоил половины всего времени сборки дерева
					 *
					 * @param reader объект потокового чтения текста
					 *
					 * \~english
					 * @brief Method of the transfer of the characters of the parsing into the storage of the document
					 * @details The characters are transferred as a whole piece upon the exhaustion of the fed text, and
					 * not one value at a time: the offsets of the nodes are through-going, and their content arrives
					 * at its place by itself
					 * @note The transfer one value at a time cost half of the whole time of the assembly of the tree
					 * @param reader object of the streaming reading of a text
					 *
					 * \~
					 */
					void transfer(const reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод приёма события разбора, выданного прямо из чтения
					 *
					 * @details Стоит посредником между обработчиком чтения, какому возвращать
					 * нечего, и сборкой дерева: отказ сборки прекращает разбор вызовом `abort()`
					 *
					 * @param context  указание на документ, собирающий дерево
					 * @param reader   объект потокового чтения текста
					 * @param event    вид очередного события разбора
					 * @param content  указание на содержимое события в хранилище знаков разбора
					 * @param modified признак изменения содержимого разбором
					 *
					 * \~english
					 * @brief Method of the reception of a parsing event issued straight from the reading
					 * @details It stands as an intermediary between the handler of the reading, which has nothing
					 * to return, and the assembly of the tree: a failure of the assembly terminates the parsing by a call of `abort()`
					 * @param context  pointer at the document assembling the tree
					 * @param reader   object of the streaming reading of a text
					 * @param event    kind of the next event of the parsing
					 * @param content  pointer at the content of the event in the storage of the characters of the parsing
					 * @param modified flag of the modification of the content by the parsing
					 *
					 * \~
					 */
					static void handler(void * context, reader_t & reader, const event_t event, const span_t content, const bool modified) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора повторяющихся имён полей объекта
					 *
					 * @details Вызывается при закрытии объекта, когда все поля его уже
					 * собраны: разбор событий имена не удерживает вовсе, и сличать их
					 * можно лишь у собранного объекта
					 *
					 * @param parent номер узла разбираемого объекта
					 * @param reader объект потокового чтения текста
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the analysis of the repeating names of the fields of an object
					 * @details Is called at the closing of an object when all of its fields are already
					 * assembled: the parsing of the events does not hold the names at all, and they
					 * can be compared only at an assembled object
					 * @param parent index of the node of the object being analyzed
					 * @param reader object of the streaming reading of a text
					 * @return sign of the success of the analysis
					 *
					 * \~
					 */
					bool deduplicate(const uint32_t parent, const reader_t & reader) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод переноса владеющего значения в перечень узлов дерева
					 *
					 * @details Узлы ложатся тем же порядком, каким их кладёт разбор: сперва
					 * сам узел, за ним дети его подряд. Содержимое дописывается в конец
					 * хранилища знаков документа, а имя поля ложится вплотную перед
					 * содержимым - одного смещения хватает обоим
					 *
					 * @param value переносимое владеющее значение
					 * @param name  имя поля объекта, ноль - узел полем объекта не является
					 * @param nodes перечень узлов, куда ложится перенесённое значение
					 * @return      количество узлов перенесённого поддерева
					 *
					 * \~english
					 * @brief Method of the transfer of an owning value into the list of the nodes of the tree
					 * @details The nodes are placed in the same order in which the parsing places them: first
					 * the node itself, then its children in a row. The content is appended to the end of
					 * the storage of the characters of the document, while the name of a field is placed right before
					 * the content — one offset suffices for both
					 * @param value owning value being transferred
					 * @param name name of the field of an object, a zero — the node is not a field of an object
					 * @param nodes list of the nodes where the transferred value is placed
					 * @return number of the nodes of the transferred subtree
					 *
					 * \~
					 */
					uint32_t transplant(const json::Value & value, const string * name, vector <node_t> & nodes) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод определения вида числа вместе с преобразованием его
					 *
					 * @details Вид выбирается самый узкий из вмещающих: число `1` получает вид
					 * `UINT8`, а `-1` - вид `INT8`. Знаковость решается знаком записи, а не
					 * величиной: запись без минуса есть число без знака
					 *
					 * @param text разбираемая запись числа
					 * @param node узел документа, куда помещается разобранное число
					 * @return     признак того, что число вместилось в родной вид
					 *
					 * \~english
					 * @brief Method of the determination of the kind of a number together with its conversion
					 * @details The kind is chosen as the narrowest of the containing ones: the number `1` receives the kind
					 * `UINT8`, while `-1` the kind `INT8`. The signedness is decided by the sign of the record rather than by
					 * the magnitude: a record without a minus is a number without a sign
					 * @param text record of the number being parsed
					 * @param node node of the document where the parsed number is placed
					 * @return flag that the number fitted into a native kind
					 *
					 * \~
					 */
					static bool classify(const string_view text, node_t & node) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи числа, хранимого узлом
					 *
					 * @details Метод этот один на перезапись документа и на выдачу записи
					 * числа: две отдельные записи одного и того же числа неминуемо разошлись
					 * бы видом
					 *
					 * @param writer объект записи текста документа
					 * @param node   узел, число какого записывается
					 *
					 * \~english
					 * @brief Method of the writing of the number stored by a node
					 * @details This method is one for the rewriting of a document and for the issuance of the record
					 * of a number: two separate writings of one and the same number would inevitably diverge
					 * in their appearance
					 * @param writer object of the writing of a text of a document
					 * @param node node the number of which is being written
					 *
					 * \~
					 */
					void compose(writer_t & writer, const node_t & node) const noexcept;
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
					bool parse(const string_view text) noexcept;
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
					bool parse(const string_view text, const callback_t & callback) noexcept;
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
					 * @brief Метод прививки владеющего значения в дерево документа
					 *
					 * @details Метод этот - обратный мост к тому, каким владеющее значение
					 * снимается с документа: снятое, изменённое и собранное заново значение
					 * возвращается на своё место в дереве, а прочее дерево остаётся
					 * нетронутым. Имя поля объекта при том сохраняется: прививается значение,
					 * а не поле
					 *
					 * @details Указатель записывается по RFC 6901, ровно как у метода `at`.
					 * Пустой указатель прививает значение корнем документа, а указатель,
					 * ведущий к отсутствующему пути, отвечает отказом: заведение
					 * недостающего принадлежит владеющему значению, а не документу
					 *
					 * @note Дерево документа лежит плоским перечнем узлов, и прививка вправе
					 *       стоить перемещения хвоста его: содержимое привитого значения
					 *       дописывается в конец хранилища знаков, а прежнее содержимое
					 *       заменённого поддерева остаётся в нём мёртвым грузом. Прививка -
					 *       действие нечастое, и платить за неё сжатием хранилища на всякий
					 *       раз было бы дороже
					 *
					 * @param pointer указатель на прививаемое место по RFC 6901
					 * @param value   прививаемое владеющее значение
					 * @return        признак успешности прививки
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into the tree of the document
					 * @details This method is the bridge reverse to the one by which an owning value
					 * is taken off a document: a value taken, changed and assembled anew
					 * is returned to its place in the tree, while the rest of the tree remains
					 * untouched. The name of a field of an object is thereby preserved: a value is grafted
					 * rather than a field
					 * @details The pointer is written by RFC 6901, exactly as for the method `at`.
					 * An empty pointer grafts the value as the root of the document, while a pointer
					 * leading to an absent path responds with a refusal: the creation
					 * of the missing belongs to an owning value rather than to a document
					 * @note The tree of a document lies as a flat list of the nodes, and the grafting is entitled
					 *       to cost a relocation of its tail: the content of the grafted value
					 *       is appended to the end of the storage of the characters, while the former content
					 *       of the replaced subtree remains in it as a dead weight. The grafting is
					 *       an infrequent action, and to pay for it by a compaction of the storage every
					 *       time would be more expensive
					 * @param pointer pointer to the place being grafted by RFC 6901
					 * @param value owning value being grafted
					 * @return sign of the success of the grafting
					 *
					 * \~
					 */
					bool graft(const string & pointer, const json::Value & value) noexcept;
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
					 * @note Имя названо явным нарочно: у чтения `location()` означает место
					 *       ТЕКУЩЕГО события, а не отказа, и одно имя о двух значениях
					 *       заводило потребителя в западню при переходе с уровня на уровень.
					 *       Кодеки XML, TOML и INI зовут его так же
					 *
					 * \~english
					 * @brief Method of the extraction of the position of the refusal of the parsing in the source text
					 * @return position of the refusal of the parsing in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
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
					explicit Document(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки объекта ведения журнала работы
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
