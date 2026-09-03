/**
 * @file document.hpp
 * @date 2026-08-01
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
 * @brief Заголовочный файл дерева разметки XML — класс Document, размещающий узлы разобранного текста
 *        в арене и связывающий их индексами, и класс Node, дающий доступ к отдельному узлу дерева
 *
 * \~english
 * @brief Header file of the XML markup tree — the Document class, which places the nodes of the parsed text
 *        in an arena and links them by the indexes, and the Node class, which gives access to a separate node of the tree
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_DOCUMENT__
#define __AWH_CODEC_XML_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "reader.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён контейнеров данных
	 *
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера XML
		 *
		 *
		 * \~english
		 * @brief XML container namespace
		 *
		 * \~
		 */
		namespace xml {
			/**
			 * \~russian
			 * @brief Индекс узла дерева разметки в арене
			 *
			 * \~english
			 * @brief Index of a node of a markup tree in the arena
			 *
			 * \~
			 */
			using node_id_t = uint32_t;

			/**
			 * \~russian
			 * @brief Значение индекса отсутствующего узла дерева разметки
			 *
			 * \~english
			 * @brief Value of the index of an absent node of a markup tree
			 *
			 * \~
			 */
			constexpr node_id_t INVALID_NODE = static_cast <node_id_t> (~0u);

			/**
			 * \~russian
			 * @brief Виды узлов дерева разметки
			 *
			 * @details Вид SPACE появляется в дереве лишь при включённой настройке
			 * разбора separateSpaces; без неё пробельное содержимое неотличимо от
			 * текстового и кладётся узлом вида TEXT
			 *
			 * @note Содержимое узла разметки и его запись от этого разделения не
			 * зависят: пробельное содержимое собирается и записывается наравне с
			 * текстовым, а отбирать его следует видом узла
			 *
			 * \~english
			 * @brief Kinds of the nodes of a markup tree
			 * @details The SPACE kind appears in a tree only when the separateSpaces setting of the
			 * parsing is enabled; without it a whitespace content is indistinguishable from a
			 * text one and is put as a node of the TEXT kind
			 * @note The content of a markup node and its writing do not depend on this
			 * separation: a whitespace content is assembled and written on a par with a
			 * text one, while it should be selected by the kind of the node
			 *
			 * \~
			 */
			enum class kind_t : uint8_t {
				NONE       = 0x00, // Вид узла не определён
				DOCUMENT   = 0x01, // Корень дерева, вмещающий содержимое текста целиком
				ELEMENT    = 0x02, // Узел разметки с именем, атрибутами и содержимым
				TEXT       = 0x03, // Текстовое содержимое узла
				CDATA      = 0x04, // Раздел дословного текста
				COMMENT    = 0x05, // Примечание
				PROCESSING = 0x06, // Указание обработчику
				DOCTYPE    = 0x07, // Описание типа документа
				SPACE      = 0x08  // Пробельное содержимое, не значимое для строения
			};

			/**
			 * \~russian
			 * @brief Предварительное объявление класса дерева разметки
			 *
			 * \~english
			 * @brief Forward declaration of the class of the markup tree
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ Document;

			/**
			 * \~russian
			 * @brief Предварительное объявление владеющего значения
			 *
			 * @details Объявление это заведено ради прививки: дерево прививаемое значение
			 * принимает, но устройства его не знает вовсе - оно лежит отдельным заголовком
			 *
			 * \~english
			 * @brief Forward declaration of the owning value
			 * @details This declaration is made for the sake of the grafting: a tree accepts a value
			 * being grafted, yet knows nothing of its arrangement — it lies in a separate header
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ Value;

			/**
			 * \~russian
			 * @brief Класс узла дерева разметки
			 *
			 * @details Узел не владеет содержимым, а лишь указывает на запись в арене
			 * дерева. Обходить дерево следует узлами, а не индексами: узел проверяет
			 * собственную пригодность и не требует держать дерево под рукой
			 *
			 * @warning Узел остаётся пригодным, пока живо дерево и пока его строение не
			 * изменено. Изменение строения дерева обесценивает все ранее полученные узлы
			 *
			 * \~english
			 * @brief Class of a node of a markup tree
			 * @details A node does not own the content but only points to a record in the arena
			 * of the tree. The tree should be traversed by the nodes rather than by the indexes: a node checks
			 * its own validity and does not require keeping the tree at hand
			 * @warning A node remains valid while the tree is alive and while its construction has not been
			 * changed. A change of the construction of the tree invalidates all the previously obtained nodes
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Node {
				private:
					// Дерево разметки, которому принадлежит узел
					const Document * _document;
				private:
					// Индекс узла в арене дерева разметки
					node_id_t _id;
				private:
					/**
					 * \~russian
					 * Клеймо поколения дерева, при котором узел снят
					 *
					 * @details Узел хранит опознаватель, а перестроение дерева - разбор, чтение,
					 * очистка, прививка - нумерацию меняет. Узел, перестроение переживший,
					 * указывал бы на СОВСЕМ ДРУГОЙ узел, отвечая при этом пригодностью и отдавая
					 * правдоподобное имя: отличить подмену эту потребителю было НЕЧЕМ. Замер: узел
					 * `<r>`, снятый до повторного разбора, отвечал пригодным и звался `<z>` -
					 * корнем нового дерева
					 *
					 * @note Клеймо сличается ПОСЛЕДНИМ в цепочке проверок. Замер соседнего кодека
					 *       JSON, устроенного тем же порядком и тем же вложенным деревом: сличение
					 *       перед проверкой номера стоило 12 %, после неё - 2.5 %
					 *
					 * @warning Порядок этот НЕ переносим между кодеками. У кодека YAML замер дал
					 * обратное - 1.5 % прежде номера против 9.4 % после, - и дерево там плоское.
					 * Выгодный порядок есть свойство своего кодека; здесь он взят от JSON по
					 * сходству устройства дерева, а не по общему правилу
					 *
					 * \~english
					 * Stamp of the generation of the tree at which the node was taken
					 * @details The node holds an identifier, while a rebuilding of the tree — the parsing, the reading,
					 * the clearing, the grafting — changes the numbering. A node that has survived a rebuilding
					 * would point at a COMPLETELY DIFFERENT node, answering at that with fitness and giving away
					 * a plausible name: there was NOTHING for the consumer to tell that substitution by. Measurement: the node
					 * `<r>`, taken before a repeated parsing, answered as fit and was called `<z>` —
					 * the root of the new tree
					 * @note The stamp is compared LAST in the chain of the checks. The measurement of the neighbouring codec
					 *       JSON, arranged in the same order: the comparison before the check of the index
					 *       cost 13 %, after it — 3 %
					 *
					 * \~
					 */
					uint32_t _stamp;
				public:
					/**
					 * \~russian
					 * @brief Метод получения вида узла
					 *
					 * @return вид узла дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the kind of a node
					 * @return kind of the node of the markup tree
					 *
					 * \~
					 */
					kind_t kind() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения имени узла
					 *
					 * @return имя узла с учётом пространства имён
					 *
					 * \~english
					 * @brief Method of getting the name of a node
					 * @return name of the node with regard to the namespace
					 *
					 * \~
					 */
					name_t name() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого узла
					 *
					 * @details Для текстового содержимого, дословного раздела, примечания и
					 * указания обработчику - их собственное содержимое. Для узла разметки -
					 * содержимое всех вложенных в него текстовых узлов подряд
					 *
					 * @return содержимое узла
					 *
					 * \~english
					 * @brief Method of getting the content of a node
					 * @details For a text content, a literal section, a comment and a
					 * processing instruction — their own content. For a markup node —
					 * the content of all the text nodes nested into it in a row
					 * @return content of the node
					 *
					 * \~
					 */
					string text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества вложенных узлов
					 *
					 * @details Считаются дети ПЕРВОГО УРОВНЯ вложенности, а глубже лежащие в счёт
					 * не идут. Считаются при том вложенные узлы ВСЯКОГО вида, а не одни лишь узлы
					 * разметки: содержимое текстовое, разделы дословные, примечания и указания
					 * обработчику суть такие же дети, и обход `first()`/`next()` отдаёт их наравне
					 *
					 * @note Счёт этот годен для обхода `for(i = 0; i < size(); i++)` вместе со
					 *       звеном пути по номеру, каковое тоже берёт вложенный узел всякого вида
					 *
					 * @return количество вложенных узлов первого уровня
					 *
					 * \~english
					 * @brief Method of getting the number of the nested nodes
					 * @details The children of THE FIRST LEVEL of the nesting are counted, and those
					 * lying deeper do not go into the count. The nested nodes of EVERY kind are counted
					 * rather than the markup nodes alone
					 * @return number of the nested nodes of the first level
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места узла в исходном тексте
					 *
					 * @return положение начала узла в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of a node in the source text
					 * @return position of the beginning of the node in the source text
					 *
					 * \~
					 */
					location_t location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения родительского узла
					 *
					 * @return родительский узел дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the parent node
					 * @return parent node of the markup tree
					 *
					 * \~
					 */
					Node parent() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения первого вложенного узла
					 *
					 * @return первый вложенный узел дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the first nested node
					 * @return first nested node of the markup tree
					 *
					 * \~
					 */
					Node first() const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод заведения отображения имён вложенных узлов родителя
					 *
					 * @note Отображение, уже заведённое, не перестраивается
					 *
					 * \~english
					 * @brief Method of the creation of the mapping of the names of the nested nodes of a parent
					 *
					 * \~
					 */
					void reindex() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения последнего вложенного узла
					 *
					 * @return последний вложенный узел дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the last nested node
					 * @return last nested node of the markup tree
					 *
					 * \~
					 */
					Node last() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения следующего узла того же уровня
					 *
					 * @return следующий узел того же уровня вложенности
					 *
					 * \~english
					 * @brief Method of getting the next node of the same level
					 * @return next node of the same level of the nesting
					 *
					 * \~
					 */
					Node next() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения предыдущего узла того же уровня
					 *
					 * @return предыдущий узел того же уровня вложенности
					 *
					 * \~english
					 * @brief Method of getting the previous node of the same level
					 * @return previous node of the same level of the nesting
					 *
					 * \~
					 */
					Node prev() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод поиска вложенного узла разметки по имени
					 *
					 * @details Поиск ведётся среди непосредственно вложенных узлов, вглубь
					 * не спускаясь. Сличение имён ведётся по паре из обозначения
					 * пространства имён и местного имени
					 *
					 * @warning Пустое обозначение пространства имён означает узел, пространству
					 * имён не принадлежащий, а не «любое пространство имён». Разбор с учётом
					 * пространств имён связывает узлы объявленного документа с их обозначениями,
					 * и поиск по одному местному имени не найдёт там ничего
					 *
					 * @param local местное имя искомого узла
					 * @param uri   обозначение пространства имён искомого узла
					 * @return      найденный узел дерева разметки
					 *
					 * \~english
					 * @brief Method of searching for a nested markup node by a name
					 * @details The search is conducted among the directly nested nodes without descending
					 * into the depth. The comparison of the names is conducted by the pair of the designation of the
					 * namespace and the local name
					 * @warning An empty designation of a namespace means a node not belonging to a
					 * namespace rather than «any namespace». A parsing with regard to the
					 * namespaces binds the nodes of a declared document to their designations,
					 * and a search by the local name alone will find nothing there
					 * @param local local name of the node being sought
					 * @param uri   designation of the namespace of the node being sought
					 * @return      found node of the markup tree
					 *
					 * \~
					 */
					Node child(const string_view local, const string_view uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня вложенных узлов разметки по имени
					 *
					 * @warning Пустое обозначение пространства имён означает узел, пространству
					 * имён не принадлежащий, а не «любое пространство имён». Разбор с учётом
					 * пространств имён связывает узлы объявленного документа с их обозначениями,
					 * и поиск по одному местному имени не найдёт там ничего
					 *
					 * @param local местное имя искомых узлов
					 * @param uri   обозначение пространства имён искомых узлов
					 * @return      перечень найденных узлов дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the list of the nested markup nodes by a name
					 * @warning An empty designation of a namespace means a node not belonging to a
					 * namespace rather than «any namespace». A parsing with regard to the
					 * namespaces binds the nodes of a declared document to their designations,
					 * and a search by the local name alone will find nothing there
					 * @param local local name of the nodes being sought
					 * @param uri   designation of the namespace of the nodes being sought
					 * @return      list of the found nodes of the markup tree
					 *
					 * \~
					 */
					vector <Node> children(const string_view local, const string_view uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска узла разметки вглубь дерева по имени
					 *
					 * @details Поиск обходит всё поддерево узла в порядке следования в
					 * исходном тексте и выдаёт первое совпадение
					 *
					 * @warning Пустое обозначение пространства имён означает узел, пространству
					 * имён не принадлежащий, а не «любое пространство имён». Разбор с учётом
					 * пространств имён связывает узлы объявленного документа с их обозначениями,
					 * и поиск по одному местному имени не найдёт там ничего
					 *
					 * @param local местное имя искомого узла
					 * @param uri   обозначение пространства имён искомого узла
					 * @return      найденный узел дерева разметки
					 *
					 * \~english
					 * @brief Method of searching for a markup node into the depth of the tree by a name
					 * @details The search traverses the whole subtree of the node in the order of the succession in the
					 * source text and issues the first coincidence
					 * @warning An empty designation of a namespace means a node not belonging to a
					 * namespace rather than «any namespace». A parsing with regard to the
					 * namespaces binds the nodes of a declared document to their designations,
					 * and a search by the local name alone will find nothing there
					 * @param local local name of the node being sought
					 * @param uri   designation of the namespace of the node being sought
					 * @return      found node of the markup tree
					 *
					 * \~
					 */
					Node find(const string_view local, const string_view uri = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения перечня атрибутов узла
					 *
					 * @return перечень атрибутов узла разметки
					 *
					 * \~english
					 * @brief Method of getting the list of the attributes of a node
					 * @return list of the attributes of the markup node
					 *
					 * \~
					 */
					vector <attribute_t> attributes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня объявлений пространств имён узла
					 *
					 * @details Выдаются объявления, записанные при самом узле, а не действующие в нём:
					 * объявленное родителем действует и здесь, но принадлежит родителю. Смысл имён от
					 * объявлений не зависит - он уже разрешён и выдаётся обозначением пространства имён,
					 * - а нужны они там, где требуется вернуть текст в исходной записи: договор о подписи
					 * XML сличает документы знак в знак, и переназначенный при записи префикс подпись ломает
					 *
					 * @return перечень связываний префиксов, объявленных узлом
					 *
					 * \~english
					 * @brief Method of getting the list of the declarations of the namespaces of a node
					 * @details The declarations written at the node itself are issued rather than the ones effective in it:
					 * what has been declared by the parent acts here as well but belongs to the parent. The meaning of the names does not depend on the
					 * declarations — it has already been resolved and is issued as the designation of the namespace —
					 * while they are needed there where it is required to return the text in the source notation: the XML signature
					 * protocol compares the documents character by character, and a prefix reassigned at the writing breaks the signature
					 * @return list of the bindings of the prefixes declared by the node
					 *
					 * \~
					 */
					vector <binding_t> bindings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения атрибута узла
					 *
					 * @warning Вид этот живёт лишь до ближайшего перестроения дерева: он указывает
					 * в общее хранилище знаков, а разбор, чтение, очистка и прививка его переписывают.
					 * Сам узел клеймом поколения защищён и по устаревании отвечает пустым, а вот вид,
					 * СНЯТЫЙ ПРЕЖДЕ и удержанный, защищён быть не может. Нужное дольше следует копировать
					 *
					 * @param local местное имя искомого атрибута
					 * @param uri   обозначение пространства имён искомого атрибута
					 * @return      значение найденного атрибута либо пустая последовательность
					 *
					 * \~english
					 * @brief Method of getting the value of an attribute of a node
					 * @warning This view lives only until the nearest rebuilding of the tree: it points
					 * into the common storage of the characters, while the parsing, the reading, the clearing and the grafting rewrite it.
					 * The node itself is protected by the stamp of the generation and upon becoming stale answers with an empty result, whereas a view
					 * TAKEN BEFOREHAND and held cannot be protected. What is needed for longer should be copied
					 * @param local local name of the attribute being sought
					 * @param uri   designation of the namespace of the attribute being sought
					 * @return      value of the found attribute or an empty sequence
					 *
					 * \~
					 */
					string_view attribute(const string_view local, const string_view uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия атрибута у узла
					 *
					 * @param local местное имя искомого атрибута
					 * @param uri   обозначение пространства имён искомого атрибута
					 * @return      результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of an attribute at a node
					 * @param local local name of the attribute being sought
					 * @param uri   designation of the namespace of the attribute being sought
					 * @return      result of the check
					 *
					 * \~
					 */
					bool has(const string_view local, const string_view uri = "") const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения содержимого узла числом
					 *
					 * @details Разбор ведётся по правилам местности «C» с отбрасыванием
					 * пробельной обвязки и с проверкой выхода за пределы запрошенного типа.
					 * Содержимое, числом не являющееся целиком, отвергается
					 *
					 * @note Признаком успеха служит выданное значение, а не разобранное
					 * число: без этого содержимое «abc» не отличалось бы от «0»
					 *
					 * @param result ссылка на результат разбора
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the content of a node as a number
					 * @details The parsing is conducted by the rules of the «C» locale with the discarding of the
					 * whitespace padding and with a check of going beyond the limits of the requested type.
					 * A content that is not a number in full is rejected
					 * @note The issued value serves as the sign of the success rather than the parsed
					 * number: without this the content «abc» would not differ from «0»
					 * @param result reference to the result of the parsing
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result) const noexcept {
						// Выполняем разбор содержимого узла числом
						return numeric(this->text(), result);
					}
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения значения атрибута узла числом
					 *
					 * @param result ссылка на результат разбора
					 * @param local  местное имя искомого атрибута
					 * @param uri    обозначение пространства имён искомого атрибута
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the value of an attribute of a node as a number
					 * @param result reference to the result of the parsing
					 * @param local  local name of the attribute being sought
					 * @param uri    designation of the namespace of the attribute being sought
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result, const string_view local, const string_view uri = "") const noexcept {
						// Выполняем разбор значения атрибута узла числом
						return numeric(this->attribute(local, uri), result);
					}
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения содержимого узла числом со значением по умолчанию
					 *
					 * @details Значение по умолчанию выдаётся и тогда, когда узел непригоден,
					 * и тогда, когда его содержимое числом не является. Способ предназначен
					 * для необязательных полей, где отсутствие значения и есть умолчание
					 *
					 * @param fallback значение, выдаваемое при неудачном разборе
					 * @return         разобранное число либо значение по умолчанию
					 *
					 * \~english
					 * @brief Method of getting the content of a node as a number with a default value
					 * @details The default value is issued both when the node is invalid
					 * and when its content is not a number. The method is intended
					 * for the optional fields where the absence of a value is the default
					 * @param fallback value issued at an unsuccessful parsing
					 * @return         parsed number or the default value
					 *
					 * \~
					 */
					T number(const T fallback) const noexcept {
						// Результат разбора содержимого узла
						T result = fallback;
						/**
						 * Выводим разобранное число, если разбор удался
						 */
						return (this->value(result) ? result : fallback);
					}
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения значения атрибута узла числом со значением по умолчанию
					 *
					 * @param local    местное имя искомого атрибута
					 * @param fallback значение, выдаваемое при неудачном разборе
					 * @param uri      обозначение пространства имён искомого атрибута
					 * @return         разобранное число либо значение по умолчанию
					 *
					 * \~english
					 * @brief Method of getting the value of an attribute of a node as a number with a default value
					 * @param local    local name of the attribute being sought
					 * @param fallback value issued at an unsuccessful parsing
					 * @param uri      designation of the namespace of the attribute being sought
					 * @return         parsed number or the default value
					 *
					 * \~
					 */
					T number(const string_view local, const T fallback, const string_view uri = "") const noexcept {
						// Результат разбора значения атрибута узла
						T result = fallback;
						/**
						 * Выводим разобранное число, если разбор удался
						 */
						return (this->value(result, local, uri) ? result : fallback);
					}
				public:
					/**
					 * \~russian
					 * @brief Метод проверки узла на пригодность
					 *
					 * @warning Проверка эта предполагает документ ЖИВЫМ и обнаружить его уничтожение
					 *          НЕ МОЖЕТ: клеймо поколения лежит внутри самого документа, и спросить его
					 *          у снесённого объекта значит читать освобождённую память. Замер 31.08.2026
					 *          на кодеке JSON, устроенном так же: узел, снятый внутри области видимости
					 *          и опрошенный за её пределами, отвергается надзором как
					 *          `stack-use-after-scope`. Узел жизнь документа не продлевает и продлевать
					 *          не должен - иначе всякая снятая ручка держала бы дерево целиком
					 *
					 * @note Сказано это здесь нарочно: способ СОБОЮ приглашает думать, будто спрашивать
					 *       его безопасно всегда, и молчание об этом условии опаснее отсутствия способа
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking a node for its validity
					 * @return result of the check
					 *
					 * \~
					 */
					bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Оператор приведения к логическому типу
					 *
					 * @return результат проверки узла на пригодность
					 *
					 * \~english
					 * @brief Conversion operator to the logical type
					 * @return result of the check of the node for its validity
					 *
					 * \~
					 */
					explicit operator bool () const noexcept;
					/**
					 * \~russian
					 * @brief Оператор сравнения
					 *
					 * @param node узел для сравнения
					 * @return     результат сравнения
					 *
					 * \~english
					 * @brief Comparison operator
					 * @param node node for the comparison
					 * @return     result of the comparison
					 *
					 * \~
					 */
					bool operator == (const Node & node) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор сравнения
					 *
					 * @param node узел для сравнения
					 * @return     результат сравнения
					 *
					 * \~english
					 * @brief Comparison operator
					 * @param node node for the comparison
					 * @return     result of the comparison
					 *
					 * \~
					 */
					bool operator != (const Node & node) const noexcept;
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
					Node() noexcept : _document(nullptr), _id(INVALID_NODE), _stamp(0) {}
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param document дерево разметки, которому принадлежит узел
					 * @param id       индекс узла в арене дерева разметки
					 *
					 * \~english
					 * @brief Constructor
					 * @param document markup tree to which the node belongs
					 * @param id       index of the node in the arena of the markup tree
					 *
					 * \~
					 */
					Node(const Document * document, const node_id_t id) noexcept;
			} node_t;

			/**
			 * \~russian
			 * @brief Класс дерева разметки
			 *
			 * @details Дерево собирается из событий потокового чтения и размещает узлы в
			 * общей арене, связывая их индексами, а не указателями. Содержимое узлов
			 * хранится в общем хранилище знаков: одно выделение памяти на многие узлы
			 * вместо выделения на каждый
			 *
			 * @note Дерево удерживает разобранный текст целиком и оправдано там, где обход
			 * содержимого важнее расхода памяти. Для разбора текста по мере его поступления
			 * из сети предназначено потоковое чтение
			 *
			 * \~english
			 * @brief Class of a markup tree
			 * @details The tree is assembled from the events of the streaming reading and places the nodes in
			 * a common arena, linking them by the indexes rather than by the pointers. The content of the nodes
			 * is stored in a common storage of the characters: one allocation of the memory for many nodes
			 * instead of an allocation for each one
			 * @note The tree holds the parsed text in full and is justified there where a traversal of the
			 * content is more important than the expenditure of the memory. For the parsing of a text as it arrives
			 * from the network the streaming reading is intended
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Document {
				/**
				 * Узел дерева обращается к арене и хранилищу знаков напрямую
				 */
				friend class Node;
				private:
					/**
					 * \~russian
					 * @brief Запись имени в арене дерева разметки
					 *
					 * \~english
					 * @brief Record of a name in the arena of a markup tree
					 *
					 * \~
					 */
					typedef struct Title {
						// Префикс пространства имён
						span_t prefix;
						// Местное имя без префикса
						span_t local;
						// Обозначение связанного пространства имён
						span_t uri;
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
						Title() noexcept {}
					} title_t;
					/**
					 * \~russian
					 * @brief Запись атрибута в арене дерева разметки
					 *
					 * \~english
					 * @brief Record of an attribute in the arena of a markup tree
					 *
					 * \~
					 */
					typedef struct Property {
						// Имя атрибута с учётом пространства имён
						title_t name;
						// Значение атрибута, приведённое к окончательному виду
						span_t value;
						// Положение атрибута в исходном тексте
						location_t location;
						// Признак того, что значение взято из объявления по умолчанию
						bool defaulted;
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
						Property() noexcept : defaulted(false) {}
					} property_t;
					/**
					 * \~russian
					 * @brief Запись связывания префикса в арене дерева разметки
					 *
					 * @details Объявления пространств имён хранятся отдельно от узлов и привязываются
					 * к ним отдельным перечнем: объявляет их считанное число узлов, а место в записи
					 * узла заняли бы все
					 *
					 * \~english
					 * @brief Record of a binding of a prefix in the arena of a markup tree
					 * @details The declarations of the namespaces are stored separately from the nodes and are bound
					 * to them by a separate list: a mere few nodes declare them, while the place in the record
					 * of a node would be occupied by all of them
					 *
					 * \~
					 */
					typedef struct Scope {
						// Префикс без разделителя, пустой для объявления по умолчанию
						span_t prefix;
						// Обозначение объявляемого пространства имён
						span_t uri;
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
						Scope() noexcept {}
					} scope_t;
					/**
					 * \~russian
					 * @brief Запись узла в арене дерева разметки
					 *
					 * \~english
					 * @brief Record of a node in the arena of a markup tree
					 *
					 * \~
					 */
					typedef struct Record {
						// Вид узла дерева разметки
						kind_t kind;
						// Имя узла с учётом пространства имён
						title_t name;
						// Содержимое узла в хранилище знаков
						span_t value;
						// Индекс первого атрибута узла в хранилище атрибутов
						uint32_t attribute;
						// Количество атрибутов узла
						uint32_t attributes;
						// Индекс родительского узла
						node_id_t parent;
						// Индекс первого вложенного узла
						node_id_t first;
						// Индекс последнего вложенного узла
						node_id_t last;
						// Индекс следующего узла того же уровня
						node_id_t next;
						// Индекс предыдущего узла того же уровня
						node_id_t prev;
						// Положение начала узла в исходном тексте
						location_t location;
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
						Record() noexcept;
					} record_t;
					/**
					 * \~russian
					 * @brief Запись отображения имён: первый узел с именем и число одноимённых
					 *
					 * @details Числа одноимённых довольно, чтобы выдать ВСЕХ совпавших без
					 * перебора там, где он всего дороже: у широкого родителя разных имён
					 * совпавший один, и перебор всей цепочки ради него был квадратичен
					 *
					 * @note Хранить положения ВСЕХ одноимённых незачем: там, где их много, и
					 * выдача велика, и перебор цепочки платится не зря - у родителя о двадцати
					 * тысячах детей одного имени он стоит семи наносекунд на выданный узел
					 *
					 * \~english
					 * @brief Record of the mapping of the names: the first node with the name and the count of the namesakes
					 *
					 * \~
					 */
					typedef struct Entry {
						// Номер первого узла разметки с этим именем
						node_id_t first;
						// Количество узлов разметки с этим именем
						uint32_t count;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param first номер первого узла разметки с этим именем
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * @param first the number of the first markup node with this name
						 *
						 * \~
						 */
						Entry(const node_id_t first = 0) noexcept : first(first), count(1) {}
					} entry_t;
				public:
					/**
					 * \~russian
					 * @brief Настройки дерева разметки
					 *
					 * @details Настройки разбора и записи держатся раздельно: прочитать разметку
					 * с одними пределами и записать её иным видом - обыкновенное дело, и
					 * связывать их незачем
					 *
					 * @note Построение это заведено ради согласия кодеков рамки: у всех у них
					 *       настройки дерева спрашиваются и ставятся одинаково, а вид их свой
					 *       у каждого кодека
					 *
					 * @warning Настройки разбора, поданные ходу `parse()` доводом, действуют
					 *          лишь на этот заход и хранимых настроек НЕ меняют
					 *
					 * \~english
					 * @brief Settings of the markup tree
					 * @details The settings of the parsing and of the writing are kept separately
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Настройки разбора текста разметки
						reader_t::settings_t reader;
						// Настройки записи текста разметки
						writer_settings_t writer;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Settings() noexcept {}
					} settings_t;
				private:
					// Настройки дерева разметки
					settings_t _settings;
				private:
					/**
					 * \~russian
					 * @brief Метод розыска узла дерева по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой, ровно как
					 * у метода `at` владеющего значения: `/Envelope/Body/0`. Звено пути
					 * обращается к вложенному узлу по местному имени либо по номеру
					 *
					 * @note Работа эта общая у прививки, опроса наличия и извлечения узла:
					 *       розыск по пути обязан идти у них по одним правилам, а прежде он
					 *       жил внутри одной лишь прививки
					 *
					 * @details Пустой путь ведёт к корневому узлу разметки - тому самому, какой
					 * заводит и заменяет прививка пустым путём. Корень же арены узлом разметки не
					 * является вовсе и содержимое текста лишь вмещает: путём к нему не ведут
					 *
					 * @param path разыскиваемый путь
					 * @return     индекс разысканного узла, `INVALID_NODE` - узел не разыскан
					 *
					 * \~english
					 * @brief Method of the search of a node of the tree by a path
					 * @param path path being searched for
					 * @return     index of the found node, `INVALID_NODE` — the node is not found
					 *
					 * \~
					 */
					node_id_t locate(const string & path) const noexcept;
				private:
					/**
					 * \~russian
					 * Код ошибки последней операции разбора
					 *
					 * @note Изменяемым он объявлен ради записи: `dump()` есть ход постоянный -
					 *       дерева он не меняет, - однако отказ записи обязан лечь в дерево,
					 *       и спрашивается он тем же ходом `error()`, что и отказ разбора
					 *
					 * \~english
					 * Code of the error of the last operation of the parsing
					 *
					 * \~
					 */
					mutable error_t _error;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Логгер уходит и в заводимое деревом чтение: разбор сообщает о бедах
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
					 * Положение обнаруженной ошибки в исходном тексте
					 *
					 * @note Изменчиво оно намеренно: запись дерева константна, а сброс положения
					 *       прежней работы обязателен и ей - код от новой работы складывался бы
					 *       со старым местом в донесение стройное, но ложное
					 */
					mutable location_t _errorLocation;
				private:
					/**
					 * \~russian
					 * Кодировка, какою исходный текст разметки прочитан
					 *
					 * @note Хранится полем оттого, что чтение живёт лишь внутри разбора: спросить
					 *       его после выхода нельзя, а потребителю кодировка нужна и после
					 *
					 * @warning Поле это обязано сбрасываться очисткой дерева наравне с кодом
					 *          отказа: пережившая очистку кодировка отвечала бы о прежнем тексте
					 *
					 * \~english
					 * Encoding by which the source text of the markup has been read
					 *
					 * \~
					 */
					encoding_t _encoding = encoding_t::NONE;
				private:
					// Арена узлов дерева разметки
				private:
					/**
					 * \~russian
					 * Клеймо поколения дерева разметки
					 *
					 * @details Растёт всякий раз, как нумерация узлов перестраивается: разбором,
					 * чтением, очисткой и прививкой. Узлы хранят клеймо своего поколения и по
					 * несовпадению отвечают непригодностью - тем молчаливая подмена узла и
					 * обращается в честный отказ
					 *
					 * \~english
					 * Stamp of the generation of the markup tree
					 * @details Grows every time the numbering of the nodes is rebuilt: by the parsing,
					 * the reading, the clearing and the grafting. The nodes hold the stamp of their generation and
					 * answer with unfitness upon a mismatch — whereby the silent substitution of a node
					 * turns into an honest refusal
					 *
					 * \~
					 */
					uint32_t _stamp;
				private:
					vector <record_t> _nodes;
				private:
					// Хранилище атрибутов всех узлов дерева
					vector <property_t> _attributes;
				private:
					// Хранилище связываний префиксов, объявленных узлами дерева
					vector <scope_t> _scopes;
				private:
					/**
					 * \~russian
					 * Отрезки хранилища связываний, объявленных узлами дерева
					 *
					 * @note Объявляет пространства имён считанное число узлов - обыкновенно один
					 * корневой, - и отводить место под отрезок в записи каждого узла незачем:
					 * отрезок отыскивается по узлу лишь тогда, когда объявления запрошены
					 *
					 * \~english
					 * Segments of the storage of the bindings declared by the nodes of the tree
					 * @note A mere few nodes declare the namespaces — ordinarily one
					 * root node — and there is no point in allotting a place for a segment in the record of every node:
					 * the segment is found by a node only when the declarations have been requested
					 *
					 * \~
					 */
					unordered_map <node_id_t, span_t> _scoped;
				private:
					// Общее хранилище знаков имён и содержимого узлов
					string _storage;
				private:
					/**
					 * \~russian
					 * Таблица размещённых имён для сведения повторов
					 *
					 * @details Обозначение пространства имён повторяется у каждого узла области
					 * видимости, а хранилище знаков размещало бы каждое его вхождение отдельно.
					 * В ответе по договору SOAP обозначение длиннее и самого имени узла, и его
					 * содержимого, и на тысяче узлов занимает больше места, чем всё прочее вместе.
					 * Таблица сводит повторы к единственному размещению
					 *
					 * @note Сводятся обозначения пространств имён, а не имена узлов и содержимое:
					 * обозначение собственной записью узла не является и повторяется у всех узлов
					 * области видимости по устройству разметки, а имя узла и содержимое повторяются
					 * лишь по случаю. Замер на ответе по договору SOAP показал, что сведение имён
					 * узлов сверх того берёт шестую часть скорости разбора за двадцатую часть памяти
					 *
					 * \~english
					 * Table of the placed names for the reduction of the repetitions
					 * @details The designation of a namespace is repeated at every node of the scope,
					 * while the storage of the characters would place every occurrence of it separately.
					 * In an answer over the SOAP protocol the designation is longer than the name of a node itself and than its
					 * content, and on a thousand nodes it takes up more space than everything else together.
					 * The table reduces the repetitions to a single placement
					 * @note The designations of the namespaces are reduced rather than the names of the nodes and the content:
					 * a designation is not a record of a node of its own and is repeated at all the nodes
					 * of the scope by the arrangement of the markup, while the name of a node and the content are repeated
					 * only by chance. A measurement on an answer over the SOAP protocol showed that a reduction of the names
					 * of the nodes on top of that takes a sixth of the speed of the parsing for a twentieth of the memory
					 *
					 * \~
					 */
					unordered_map <string, span_t> _interned;
				private:
					/**
					 * \~russian
					 * Отображения местных имён вложенных узлов на их номера по родительским узлам
					 *
					 * @details Заводится при первом же розыске у родителя, детей у какого больше
					 * `INDEX_THRESHOLD`. Узкие родители, а таковых подавляющее большинство, не
					 * платят за него ничего: розыск у них так и остаётся перебором цепочки
					 *
					 * @note Ключом служит ОДНО местное имя, без пространства имён. Розыск ведётся
					 * по паре, и совпадение по отображению сличается с пространством имён на
					 * месте: разойдись они - розыск откатывается к перебору, а тот отвечает по
					 * договору в точности. Случай этот - несколько узлов об одном местном имени в
					 * разных пространствах - редок, и платить за него ключом из пары значило бы
					 * платить на всяком розыске
					 *
					 * @warning Ключи суть виды в общее хранилище знаков, и всякий его рост
					 * обращает их висячими: прививка значения хранилище пополняет, и отображение
					 * ей надлежит сбрасывать целиком
					 *
					 * @note Номера узлов постоянны: арена лишь доливается, узлы из неё не
					 * изымаются и не сдвигаются вовсе. Оттого сброса требуют только очистка
					 * дерева да прививка, перевязывающая цепочку детей
					 *
					 * \~english
					 * Mappings of the local names of the nested nodes onto their numbers by the parent nodes
					 *
					 * \~
					 */
					mutable unordered_map <node_id_t, unordered_map <string_view, entry_t>> _index;
				private:
					/**
					 * \~russian
					 * @brief Метод получения последовательности знаков по отрезку хранилища
					 *
					 * @param span отрезок общего хранилища знаков
					 * @return     последовательность знаков указанного отрезка
					 *
					 * \~english
					 * @brief Method of getting a sequence of characters by a segment of the storage
					 * @param span segment of the common storage of the characters
					 * @return     sequence of characters of the specified segment
					 *
					 * \~
					 */
					string_view get(const span_t & span) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки имени по записи арены
					 *
					 * @param title запись имени в арене дерева разметки
					 * @return      имя с учётом пространства имён
					 *
					 * \~english
					 * @brief Method of assembling a name by a record of the arena
					 * @param title record of the name in the arena of the markup tree
					 * @return      name with regard to the namespace
					 *
					 * \~
					 */
					name_t get(const title_t & title) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод переноса владеющего значения в арену дерева
					 *
					 * @details Значение переносится узел за узлом: запись узла дописывается к
					 * арене, знаки имени и содержимого - к хранилищу знаков, атрибуты и
					 * связывания префиксов - к своим хранилищам отрезком подряд
					 *
					 * @note Атрибуты и связывания размещаются прежде обхода вложенного
					 * содержимого намеренно: отрезок их задан началом и количеством, и
					 * содержимое, размещённое посреди, разорвало бы отрезок надвое
					 *
					 * @param value  переносимое владеющее значение
					 * @param parent индекс родительского узла переносимого значения
					 * @return       индекс заведённого узла либо признак недействительности
					 *
					 * \~english
					 * @brief Method of the transfer of an owning value into the arena of the tree
					 * @details The value is transferred node by node: the record of a node is appended to
					 * the arena, the characters of the name and of the content — to the storage of the characters,
					 * the attributes and the bindings of the prefixes — to their storages as a contiguous segment
					 * @note The attributes and the bindings are placed before the traversal of the nested
					 * content deliberately: their segment is given by a beginning and a count, and
					 * a content placed in the middle would tear the segment in two
					 * @param value  owning value being transferred
					 * @param parent index of the parent node of the value being transferred
					 * @return       index of the created node or the sign of the invalidity
					 *
					 * \~
					 */
					node_id_t transplant(const xml::Value & value, const node_id_t parent) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки
					 *
					 * @details Собирает дерево из переданного текста целиком, отбрасывая
					 * ранее разобранное. Отрицательный итог оставляет дерево пустым, а
					 * причину отказа сообщает код ошибки
					 *
					 * @param text     исходный текст разметки целиком
					 * @param settings настройки разбора текста разметки
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a markup text
					 * @details Assembles the tree from the passed text in full, discarding
					 * what has been parsed before. A negative result leaves the tree empty, while
					 * the reason of the refusal is reported by the error code
					 * @param text     source markup text in full
					 * @param settings settings of the parsing of a markup text
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool parse(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки с указанными настройками
					 *
					 * @note Настройки эти действуют лишь на этот заход и хранимых настроек
					 *       дерева НЕ меняют
					 *
					 * @param text     исходный текст разметки целиком
					 * @param settings настройки разбора текста разметки
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of parsing a markup text with the given settings
					 * @param text     source text of the markup in its entirety
					 * @param settings settings of the parsing of the text of the markup
					 * @return         flag of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, const reader_t::settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения кодировки исходного текста разметки
					 *
					 * @details Выдаётся кодировка, какою текст ПРОЧИТАН - та, из которой шёл перевод
					 * в UTF-8, распознанная по метке порядка байтов либо по объявлению разметки
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки: потребитель, читающий
					 *       несколько кодеков, спрашивает кодировку одинаково
					 *
					 * @warning До первого разбора выдаётся кодировка неопределённая
					 *
					 * @return кодировка исходного текста разметки
					 *
					 * \~english
					 * @brief Method of the extraction of the encoding of the source text of the markup
					 * @return encoding of the source text of the markup
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи дерева разметки текстом
					 *
					 * @details Записывается дерево ЦЕЛИКОМ, от корня, вместе с объявлением разметки,
					 * указаниями обработчику и примечаниями. Настройки записи берутся принятые
					 * по умолчанию; иные подаются перегрузкой ниже
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки: потребитель, пишущий
					 *       обобщённо, зовёт `dump()` без довода, не зная кодека вовсе
					 *
					 * @warning Дословного совпадения с исходным текстом запись не обещает: связывания
					 *          пространств имён назначаются заново, а объявление типа документа
					 *          не записывается вовсе - модуль его писать не умеет
					 *
					 * @warning При отказе записи выдаётся ПУСТОЙ текст, а код отказа ложится в
					 *          дерево и спрашивается ходом `error()`. Пустое дерево от отказа
					 *          отличается только этим кодом
					 *
					 * @return текст разметки собранного дерева
					 *
					 * \~english
					 * @brief Method of writing the markup tree as a text
					 * @details The tree is written WHOLE, from the root, together with the markup declaration,
					 * the processing instructions and the comments
					 * @return text of the markup of the assembled tree
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева разметки текстом с указанными настройками
					 *
					 * @details Настройки записи подаются свои: вид записи, отступ, схлопывание
					 * пустых узлов и прочее
					 *
					 * @note Вид настроек тут свой у каждого кодека рамки, а общею сделана лишь
					 *       ПОДПИСЬ: потребитель, знающий кодек, подаёт его настройки
					 *
					 * @param settings настройки записи текста разметки
					 * @return         текст разметки собранного дерева
					 *
					 * \~english
					 * @brief Method of writing the markup tree as a text with the given settings
					 * @param settings settings of the writing of the text of the markup
					 * @return         text of the markup of the assembled tree
					 *
					 * \~
					 */
					string dump(const writer_settings_t & settings) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки из файла
					 *
					 * @details Файл читается целиком, после чего текст его разбирается ходом
					 * `parse()`. Кусками разметка не разбирается: дерево собирается по всему
					 * тексту сразу
					 *
					 * @note Отказ доступа к файлу отвечает своим кодом - `FILE_NOT_OPENED` либо
					 *       `FILE_NOT_READ`, - а не кодом отказа разбора: путь подан извне, и
					 *       код внутренней беды отправлял бы потребителя искать изъян у нас
					 *
					 * @warning Каталог, поданный вместо файла, ОТКРЫВАЕТСЯ успешно, а читается
					 *          признаками конца и отказа - теми же, какими отзывается файл
					 *          пустой. Распознаётся он по самому адресу и отвечает `FILE_NOT_READ`
					 *
					 * @param filename адрес файла разметки
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of parsing a markup text from a file
					 * @param filename address of the file of the markup
					 * @return         flag of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения настроек дерева разметки
					 *
					 * @return настройки дерева разметки
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the markup tree
					 * @return settings of the markup tree
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек дерева разметки
					 *
					 * @note Настройки эти действуют на ходы, довода настроек не принимающие:
					 *       `parse(text)`, `load()`, `dump()` и `save(filename)`. Поданные же
					 *       доводом действуют лишь на свой заход
					 *
					 * @param settings устанавливаемые настройки дерева разметки
					 *
					 * \~english
					 * @brief Method of setting the settings of the markup tree
					 * @param settings settings of the markup tree being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева разметки в файл
					 *
					 * @details Записывается то же самое, что выдаёт ход `dump()`, с настройками,
					 * принятыми по умолчанию
					 *
					 * @warning Прежнее содержимое файла затирается целиком
					 *
					 * @param filename адрес файла разметки
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of writing the markup tree to a file
					 * @param filename address of the file of the markup
					 * @return         flag of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дерева разметки в файл с указанными настройками
					 *
					 * @param filename адрес файла разметки
					 * @param settings настройки записи текста разметки
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of writing the markup tree to a file with the given settings
					 * @param filename address of the file of the markup
					 * @param settings settings of the writing of the text of the markup
					 * @return         flag of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename, const writer_settings_t & settings) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения корня дерева
					 *
					 * @details Корень дерева вмещает содержимое текста целиком, включая
					 * примечания и указания обработчику вне корневого узла разметки
					 *
					 * @return корневой узел дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the root of the tree
					 * @details The root of the tree accommodates the content of the text in full, including
					 * the comments and the processing instructions outside the root markup node
					 * @return root node of the markup tree
					 *
					 * \~
					 */
					node_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения корневого узла разметки
					 *
					 * @return единственный узел разметки верхнего уровня
					 *
					 * \~english
					 * @brief Method of getting the root markup node
					 * @return single markup node of the top level
					 *
					 * \~
					 */
					node_t element() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод прививки владеющего значения в дерево разметки
					 *
					 * @details Метод этот - обратный мост к тому, каким владеющее значение
					 * снимается с узла дерева: значение переносится в арену и становится на
					 * место указанного узла со всем его содержимым
					 *
					 * @details Путь записывается частями, разделёнными косой чертой, ровно как
					 * у метода `at` владеющего значения: `/Envelope/Body/0`. Прививаемое место
					 * обязано существовать - прививка заменяет поддерево, а не заводит его:
					 * заведение отсутствующего принадлежит владеющему значению, а не дереву
					 *
					 * @details Пустой путь прививает значение корневым узлом разметки, дерево
					 * притом заводя с нуля, буде его ещё нет. Ходом этим дерево и строится,
					 * минуя разбор текста: собирается владеющее значение со всем содержимым
					 * своим, а прививка ставит его корнем. Документ JSON тем же пустым
					 * указателем заводит корень своего дерева
					 *
					 * @note Корнем дерева разметки становится ОДИН ЛИШЬ УЗЕЛ РАЗМЕТКИ: XML 1.0
					 * требует у документа ровно один корневой элемент, и текст, примечание либо
					 * указание обработчику корнем ему не бывать - прививка их отвергает кодом
					 * `MISSING_ROOT`. Расхождение это с документом JSON, где корнем становится
					 * значение любого вида, идёт от стандарта, а не от произвола
					 *
					 * @note Арена дерева лишь дописывается: узлы заменённого поддерева остаются
					 * в ней недостижимыми и место своё возвращают только с очисткою дерева либо
					 * с новым разбором. Устройство это намеренное - перенумерование узлов
					 * обесценило бы всякую ссылку на дерево, выданную наружу прежде
					 *
					 * @note Значение, несущее корень дерева с единственным узлом, прививается
					 * узлом этим: разбор текста во владеющее значение корень заводит всегда, и
					 * правило это то же самое, каким сборка отдаёт собранный узел без корня над
					 * ним. Корень с иным числом узлов отвергается - одним узлом ему не стать, а
					 * прививка заменяет узел узлом
					 *
					 * @note Перенос значения в арену идёт возвратно, глубина за глубиной, и
					 * дерево небывалой вложенности сорвало бы стек. Значение, снятое с разбора,
					 * тем ограждено пределом вложенности чтения (1024 по умолчанию), а
					 * собранному вручную предела нет вовсе. Замер по стендам: при стеке 4 МБ
					 * (NetBSD, OpenBSD) срыв наступает около 9 000 уровней у самой скупой из
					 * систем, при стеке 8 МБ (macOS) - около 25 000. Предел чтения держится
					 * оттого с девятикратным запасом даже там, где стека меньше всего
					 *
					 * @param path  путь к прививаемому месту
					 * @param value прививаемое владеющее значение
					 * @return      признак успешности прививки
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into the markup tree
					 * @details This method is the bridge reverse to the one by which an owning value
					 * is taken from a node of a tree: the value is transferred into the arena and takes
					 * the place of the specified node with all of its content
					 * @details The path is written by the parts separated by a slash, exactly as
					 * for the method `at` of an owning value: `/Envelope/Body/0`. The place being grafted
					 * must exist — the grafting replaces a subtree rather than creates it: the creation
					 * of the missing belongs to an owning value rather than to the tree
					 * @details An empty path grafts the value as the root node of the markup, creating
					 * the tree from scratch if there is none yet. It is by this that a tree is built
					 * apart from the parsing of a text: an owning value is assembled with all of its
					 * content, and the grafting sets it as the root. A JSON document creates the root
					 * of its own tree by that same empty pointer
					 * @note ONLY A MARKUP NODE becomes the root of a markup tree: XML 1.0 requires
					 * exactly one root element of a document, and a text, a comment or a processing
					 * instruction is never to be its root — the grafting refuses them with the code
					 * `MISSING_ROOT`. This divergence from a JSON document, where a value of any kind
					 * becomes the root, comes from the standard rather than from an arbitrary choice
					 * @note The arena of the tree is only appended to: the nodes of the replaced subtree remain
					 * in it unreachable and give their place back only with the clearing of the tree or
					 * with a new parsing. This arrangement is deliberate — a renumbering of the nodes
					 * would invalidate every reference to the tree given away outside before
					 * @note A value carrying the root of a tree with a single node is grafted by that
					 * node: the parsing of a text into an owning value always creates a root, and this rule
					 * is the very same by which the building gives away a built node without a root above it.
					 * A root with any other number of nodes is refused — it is never to become a single node,
					 * while the grafting replaces a node by a node
					 * @note The transfer of a value into the arena goes recursively, depth by depth, and
					 * a tree of an unheard-of nesting would overflow the stack. A value taken from a parsing
					 * is guarded by the limit of the nesting of the reading (1024 by default), while for one
					 * built by hand there is no limit at all. A measurement across the stands: with a stack of 4 MB
					 * (NetBSD, OpenBSD) the overflow comes at about 9 000 levels at the scarcest of the systems,
					 * with a stack of 8 MB (macOS) — at about 25 000. The limit of the reading thus holds
					 * with a ninefold reserve even where there is the least stack
					 * @param path  path to the place being grafted
					 * @param value owning value being grafted
					 * @return      sign of the success of the grafting
					 *
					 * \~
					 */
					bool set(const string & path, const xml::Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод сноса узла дерева разметки по пути
					 *
					 * @details Узел, разысканный путём, отвязывается от дерева со всем содержимым
					 * своим: родитель о нём забывает, а соседи связываются друг с другом
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки, а вид пути остаётся
					 *       своим у каждого: у разметки это путь по именам узлов и номерам
					 *
					 * @warning Снос корня отвергается: дерево без корня разметкой не является
					 *          вовсе, и опустошается оно ходом `clear()`. Довод этот от СТАНДАРТА,
					 *          а не от единообразия: XML 1.0 §2.1 велит документу нести ровно один
					 *          корневой элемент. У кодеков, чей стандарт пустой документ допускает,
					 *          снос корня вправе быть дозволен, и приводить их к нашему укладу
					 *          НЕ НАДО: при столкновении согласия API со стандартом старше
					 *          стандарт (решение владельца от 03.09.2026)
					 *
					 * @warning Арена дерева лишь дописывается: узлы снесённого поддерева остаются
					 *          в ней недостижимыми и место своё возвращают только с очисткою
					 *          дерева либо с новым разбором - ровно как при правке
					 *
					 * @details Пустой путь ведёт к корневому узлу разметки, а снос его отвергается:
					 * XML 1.0 требует у документа ровно один корневой элемент, и дерево, корня
					 * лишённое, записи не подлежит вовсе. Заменить корень целиком дозволено
					 * прививкой пустым путём, а снести его - нет. Документ JSON пустой указатель
					 * сносом отвергает тем же порядком
					 *
					 * @param path путь к сносимому узлу
					 * @return     признак успешности сноса
					 *
					 * \~english
					 * @brief Method of the removal of a node of the markup tree by a path
					 * @param path path to the node being removed
					 * @return     flag of the success of the removal
					 *
					 * \~
					 */
					bool erase(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса содержимого узла дерева разметки по пути
					 *
					 * @details Узел, разысканный путём, содержимого своего лишается: вложенные
					 * узлы отвязываются, а сам узел остаётся на месте с именем своим и
					 * свойствами своими
					 *
					 * @note Сброс от сноса тем и отличается, что узел сохраняется: пустой узел
					 *       разметки есть узел законный, и отсутствию его он не равен
					 *
					 * @details Пустой путь ведёт к корневому узлу разметки: сброс им опустошает
					 * содержимое корня, сам корень оставляя на месте
					 *
					 * @param path путь к сбрасываемому узлу
					 * @return     признак успешности сброса
					 *
					 * \~english
					 * @brief Method of the resetting of the content of a node of the markup tree by a path
					 * @param path path to the node being reset
					 * @return     flag of the success of the resetting
					 *
					 * \~
					 */
					bool reset(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод прививки владеющего значения в дерево разметки
					 *
					 * @deprecated Имя это УСТАРЕЛО и оставлено посредником ради потребителей,
					 * написанных прежде согласования кодеков рамки между собой. Зови `set()`:
					 * им правка дерева по пути зовётся у всех семи кодеков, а вид пути остаётся
					 * своим у каждого - у разметки это путь по именам узлов и номерам вложенных
					 *
					 * @param path  путь к прививаемому месту
					 * @param value прививаемое владеющее значение
					 * @return      признак успешности прививки
					 *
					 * \~english
					 * @brief Method of the grafting of an owning value into the markup tree
					 * @deprecated This name is DEPRECATED and is left as an intermediary. Call `set()`
					 * @param path  path to the place being grafted
					 * @param value owning value being grafted
					 * @return      flag of the success of the grafting
					 *
					 * \~
					 */
					bool graft(const string & path, const xml::Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия узла дерева по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой, ровно как у
					 * прививки: `/Envelope/Body/0`
					 *
					 * @note Ход этот заведён общим у всех кодеков рамки, а вид пути остаётся
					 *       своим у каждого: у разметки это путь по именам узлов и номерам
					 *
					 * @param path проверяемый путь
					 * @return     признак наличия узла по указанному пути
					 *
					 * \~english
					 * @brief Method of checking the presence of a node of the tree by a path
					 * @param path path being checked
					 * @return     flag of the presence of a node at the specified path
					 *
					 * \~
					 */
					bool has(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения узла дерева по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой, ровно как у
					 * прививки: `/Envelope/Body/0`
					 *
					 * @warning Узел, по пути не разысканный, выдаётся НЕПРИГОДНЫМ, а не отказом:
					 *          пригодность его спрашивается ходом `valid()`
					 *
					 * @param path разыскиваемый путь
					 * @return     узел дерева разметки по указанному пути
					 *
					 * \~english
					 * @brief Method of the extraction of a node of the tree by a path
					 * @param path path being searched for
					 * @return     node of the markup tree at the specified path
					 *
					 * \~
					 */
					node_t at(const string & path) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества вложенных узлов корня разметки
					 *
					 * @details Считаются ДЕТИ КОРНЕВОГО УЗЛА РАЗМЕТКИ первого уровня, а не узлы
					 * арены: у дерева `<r><a/><b><c/></b></r>` счёт этот даёт 2, тогда как арена
					 * несёт 7 узлов. Счёт годен для обхода `for(i = 0; i < size(); i++)`
					 *
					 * @details Число узлов арены выдаётся ходом `nodes()`. Развести их пришлось
					 * оттого, что арены семи кодеков ведут себя по-разному: у одних дописываются и
					 * сносом не убавляются, у других сжимаются, у третьих дерева правке не
					 * подлежат вовсе, - и одного смысла «узлы арены» у семи иметь не могут, тогда
					 * как «дети корня» определимы у всех одинаково
					 *
					 * @note Дерево без корневого узла разметки отвечает нулём: считать у него
					 *       нечего, а узлы арены к содержимому отношения не имеют
					 *
					 * @return количество вложенных узлов корня разметки
					 *
					 * \~english
					 * @brief Method of getting the number of the nested nodes of the root of the markup
					 * @details THE CHILDREN OF THE ROOT MARKUP NODE of the first level are counted
					 * rather than the nodes of the arena: for the tree `<r><a/><b><c/></b></r>` this
					 * count gives 2, while the arena carries 7 nodes
					 * @details The number of the nodes of the arena is given by the call `nodes()`
					 * @return number of the nested nodes of the root of the markup
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества узлов арены дерева
					 *
					 * @details Арена эта лишь ДОПИСЫВАЕТСЯ: узлы снесённого поддерева остаются в
					 * ней недостижимыми и место своё возвращают только с очисткою дерева либо с
					 * новым разбором. Судить по этому счёту о содержимом дерева НЕЛЬЗЯ - для того
					 * есть `size()` да `empty()`
					 *
					 * @return количество узлов в арене дерева разметки
					 *
					 * \~english
					 * @brief Method of getting the number of the nodes of the arena of the tree
					 * @details The arena is only APPENDED TO: the nodes of a removed subtree remain
					 * in it unreachable. This count MUST NOT be used to judge the content of the tree
					 * @return number of the nodes in the arena of the markup tree
					 *
					 * \~
					 */
					size_t nodes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева на пустоту
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking the tree for emptiness
					 * @return result of the check
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод очистки дерева разметки
					 *
					 * \~english
					 * @brief Method of clearing the markup tree
					 *
					 * \~
					 */
					void clear() noexcept;
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
					~Document() noexcept;
			} document_t;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_XML_DOCUMENT__
