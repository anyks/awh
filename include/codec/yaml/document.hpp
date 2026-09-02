/**
 * @file document.hpp
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
 * @brief Заголовочный файл документа YAML — дерево, удерживаемое целиком, с обходом
 *        узлов и извлечением значений затребованным видом
 *
 * \~english
 * @brief Header file of a YAML document — a tree held in full with the traversal of the nodes
 *        and the extraction of the values by a demanded kind
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_DOCUMENT__
#define __AWH_CODEC_YAML_DOCUMENT__

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <vector>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "reader.hpp"
#include "writer.hpp"

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
			 * @brief Документ YAML, удерживаемый целиком
			 *
			 * @details Дерево держится плоским перечнем узлов, а не россыпью
			 * самостоятельных вместилищ: узлы ложатся порядком обхода, и переход к соседу
			 * есть сложение с размахом поддерева. Расход памяти тем выходит одним куском, а
			 * обход - последовательным чтением
			 *
			 * @note Текст YAML вправе нести много документов, и дерево удерживает их все:
			 *       `documents()` сказывает число их, а `root(номер)` отдаёт корень каждого
			 *
			 * \~english
			 * @brief YAML document held in full
			 * @details The tree is held by a flat list of the nodes rather than by a scattering of
			 * the independent containers: the nodes lie in the order of the traversal, and the transition to a sibling
			 * is an addition with the extent of the subtree
			 * @note A YAML text is entitled to carry many documents, and the tree holds all of them
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Объявление владеющего значения наперёд
			 *
			 * @note Объявление нужно дружбе дерева: владеющее значение описано файлом
			 *       иным, включаемым позже, а имя его дереву требуется здесь
			 *
			 * \~english
			 * @brief Forward declaration of the owning value
			 * @note The declaration is needed by the friendship of the tree
			 *
			 * \~
			 */
			class Value;
			typedef class __AWH_SHARED_EXPORT__ Document {
				private:
					/**
					 * \~russian
					 * @brief Владеющее значение, дереву доступ дающее
					 *
					 * @details Доступ этот нужен переносу: ведётся он на копии дерева ради
					 *          целости своей, а код отказа по неудаче обязан достаться дереву
					 *          ИСХОДНОМУ - копия с ним пропадает, и потребитель остался бы с
					 *          отказом без названной причины
					 *
					 * @note Имя пишется полным: у дерева есть свой вложенный вид `Value` -
					 *       ссылка на узел, - и краткое имя назвало бы дружбой именно его
					 *
					 * \~english
					 * @brief Owning value granted the access to the tree
					 * @details This access is needed by the grafting: it is conducted on a copy of the tree
					 * for the sake of its own integrity, while the code of the refusal must reach the
					 * ORIGINAL tree — the copy vanishes with it
					 *
					 * \~
					 */
					friend class awh::codec::yaml::Value;
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора документа
					 *
					 * \~english
					 * @brief Settings of the parsing of a document
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Схема разрешения видов скалярных значений
						schema_t schema;
						// Кодировка, навязанная извне вопреки метке порядка байтов
						encoding_t encoding;
						// Правило обращения с повторяющимся именем пары отображения
						duplicate_t duplicates;
						// Наибольшая допустимая глубина вложенности, ноль - предел по умолчанию
						uint32_t maxDepth;
						// Наибольшая допустимая длина скалярного значения, ноль - предел по умолчанию
						uint32_t maxScalar;
						/**
						 * Наибольшее допустимое количество узлов раскрытия ссылок, ноль - предел
						 * по умолчанию
						 *
						 * @note Предел этот стережёт от беды, известной под именем миллиарда
						 *       смешков: девять меток, каждая из которых десятикратно повторяет
						 *       предыдущую, раскрываются в миллиард узлов из текста в двести байт.
						 *       Потоковое чтение ссылок не раскрывает вовсе и предела того не
						 *       знает, а дерево раскрывает - ему и стеречь
						 */
						uint32_t maxExpansion;
						/**
						 * Признак удержания исходного текста ради дословной перезаписи
						 *
						 * @details Настройки YAML правятся и человеком, и приложением, и перезапись
						 * обязана сохранять всё, чего правка не касалась: примечания, пустые строки,
						 * ограду значений, ширину отступов и построение вместилищ скобками. Собрать
						 * это заново дерево не может - примечаний оно не держит вовсе, - и оттого
						 * исходный текст удерживается целиком, а поддеревья нетронутые переписываются
						 * дословно
						 *
						 * @note Удержание стоит памяти в размер текста, и оттого спрашивается
						 *       прямо: читающему настройки на один раз оно ни к чему
						 *
						 * @note БЕЗ удержания перезапись ведётся деревом заново, и построение
						 *       скобками обращается в построение отступом: `[a, b]` переписывается
						 *       перечнем на отступе. Решено это намеренно - дерево построения не
						 *       хранит, ни чтение его не выдаёт, ни узел не держит, - и удержание
						 *       есть тот самый ответ кодека на нужду сохранить написанное человеком.
						 *       Закреплено проверкой CodecYamlDocument.FlowSurvivesRetainedRewrite
						 *
						 * \~english
						 * @brief Sign of the retention of the source text for the sake of the verbatim rewriting
						 * @details The YAML settings are edited both by a human and by an application, and the rewriting
						 * must preserve everything the editing did not touch
						 * @note The retention costs the memory in the size of the text and is therefore asked for directly
						 *
						 * \~
						 */
						bool retain;
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
						Settings() noexcept;
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Свойства узла, предпосланные ему записью
					 *
					 * @note Свойства эти держатся отдельным перечнем, а не полями узла: метки и
					 *       метки типов встречаются единицами на документ, и поля под них сделали
					 *       бы тяжелее всякий узел ради немногих
					 *
					 * \~english
					 * @brief Properties of a node placed before it by the notation
					 * @note These properties are held by a separate list rather than by the fields of a node
					 *
					 * \~
					 */
					typedef struct Properties {
						// Смещение имени метки в хранилище знаков
						uint32_t anchor;
						// Длина имени метки в байтах
						uint32_t anchored;
						// Смещение метки типа в хранилище знаков
						uint32_t tag;
						// Длина метки типа в байтах
						uint32_t tagged;
						/**
						 * \~russian
						 * Смещение имени метки, ИМЕНИ ПАРЫ предпосланной, в хранилище знаков
						 *
						 * @details Метка эта принадлежит имени пары, а не значению её: написание
						 * `&m a: b` метит запись `a`, и держать её вместе с меткою значения нельзя -
						 * они разные. Узла же у имени пары дерево не держит вовсе, оттого метка
						 * его и кладётся сюда, к узлу самой пары
						 *
						 * @warning Прежде метка эта отбрасывалась: держать её было негде. Ссылка на
						 * неё разрешалась записью имени, а вот ПЕРЕЗАПИСЬ её теряла - и текст,
						 * собранный заново, нёс ссылку на метку, ни разу не объявленную
						 *
						 * \~english
						 * Offset of the name of the anchor placed before the NAME OF A PAIR in the storage of characters
						 * @details The anchor belongs to the name of a pair rather than to its value
						 *
						 * \~
						 */
						uint32_t keyAnchor;
						// Длина имени метки, имени пары предпосланной, в байтах
						uint32_t keyAnchored;
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
						Properties() noexcept : anchor(0), anchored(0), tag(0), tagged(0), keyAnchor(0), keyAnchored(0) {}
					} props_t;
					/**
					 * \~russian
					 * @brief Узел дерева документа
					 *
					 * @note Узел шире узла контейнера JSON, и шире он нарочно: запись значения
					 *       удерживается всегда, даже у числа, разобранного в родной вид.
					 *       Перезапись обязана вернуть `0x1F` записью его, а не числом 31, и без
					 *       записи вернуть её было бы неоткуда
					 *
					 * \~english
					 * @brief Node of the tree of a document
					 * @note The node is wider than the node of the JSON container, and it is wider on purpose:
					 *       the record of a value is held always, even for a number parsed into a native kind
					 *
					 * \~
					 */
					typedef struct Node {
						// Вид значения узла документа
						type_t type;
						// Вид записи значения в исходном тексте
						style_t style;
						// Признак того, что узел является парой отображения
						bool keyed;
						// Признак того, что содержимое узла правлено после разбора
						bool touched;
						/**
						 * Смещение начала записи узла в удержанном исходном тексте
						 *
						 * @details Считается оно от начала строки, узел открывающей, вместе с
						 * примечаниями да пустыми строками, ей предшествующими: примечание стоит над
						 * тем, к чему относится, и уходить обязано вместе с ним. Значение
						 * `NO_ORIGIN` знаменует, что текст не удержан либо узел взят раскрытием
						 * ссылки, и записи в тексте за ним не стоит
						 */
						uint32_t origin;
						/**
						 * \~russian
						 * Смещение начала собственной строки узла в удержанном тексте
						 *
						 * @details Начало записи узла вправе стоять выше собственной строки его:
						 * примечания да пустые строки, над узлом стоящие, уходят вместе с ним, а
						 * черта записи перечня и вовсе переносится к узлу отдельным проходом.
						 * Собственная же строка есть та, где стоит само содержимое, и запоминать
						 * её приходится при постановке узла: после переноса черты по началу
						 * записи её уже не вычислить
						 *
						 * \~english
						 * Offset of the beginning of the own line of the node in the retained text
						 * @details The beginning of the record of a node may stand above its own line:
						 * the comments and the blank lines standing above the node leave together with it, while
						 * the dash of an entry of a sequence is moved to the node by a separate pass altogether.
						 * The own line, however, is the one where the content itself stands, and it has to be remembered
						 * at the placement of the node: after the moving of the dash it can no longer be computed
						 * from the beginning of the record
						 *
						 * \~
						 */
						uint32_t dwelling;
						/**
						 * Смещение за концом записи узла в удержанном исходном тексте
						 *
						 * @details Границею служит начало записи следующего узла обхода: всякий
						 * байт текста принадлежит ровно одному узлу, и примечания, между соседями
						 * стоящие, достаются тому, над кем они стоят. Держится граница полем, а не
						 * считается по соседу: снятие узла соседа сдвинуло бы, и байты снятого
						 * достались бы тому, кто стоял над ним
						 */
						uint32_t edge;
						// Смещение имени пары в хранилище знаков
						uint32_t offset;
						// Длина имени пары в байтах
						uint32_t named;
						// Номер свойств узла в перечне свойств, ноль - свойств нет
						uint32_t props;
						/**
						 * Количество детей вместилища либо длина записи значения в байтах
						 *
						 * @details Вторым числом у вместилища лежит размах поддерева его - число
						 * узлов, вместилищем занятых вместе с ним самим
						 */
						uint32_t content[2];
						// Разобранное число значения шириною в восемь байтов
						uint32_t number[2];
						/**
						 * \~russian
						 * @brief Метод получения количества детей вместилища либо длины записи
						 *
						 * @return количество детей вместилища либо длина записи значения
						 *
						 * \~english
						 * @brief Method of the obtaining of the number of the children of a container or of the length of the record
						 * @return number of the children of a container or the length of the record of the value
						 *
						 * \~
						 */
						AWH_YAML_INLINE uint32_t length() const noexcept {
							// Выводим количество детей вместилища либо длину записи значения
							return this->content[0];
						}
						/**
						 * \~russian
						 * @brief Метод получения размаха поддерева вместилища
						 *
						 * @return количество узлов, вместилищем занятых вместе с ним самим
						 *
						 * \~english
						 * @brief Method of the obtaining of the extent of the subtree of a container
						 * @return number of the nodes occupied by the container together with itself
						 *
						 * \~
						 */
						AWH_YAML_INLINE uint32_t extent() const noexcept {
							// Выводим размах поддерева вместилища
							return this->content[1];
						}
						/**
						 * \~russian
						 * @brief Метод установки количества детей вместилища либо длины записи
						 *
						 * @param value устанавливаемое количество детей либо длина записи
						 *
						 * \~english
						 * @brief Method of the setting of the number of the children of a container or of the length of the record
						 * @param value number of the children or the length of the record being set
						 *
						 * \~
						 */
						AWH_YAML_INLINE void length(const uint32_t value) noexcept {
							// Устанавливаем количество детей вместилища либо длину записи
							this->content[0] = value;
						}
						/**
						 * \~russian
						 * @brief Метод установки размаха поддерева вместилища
						 *
						 * @param value устанавливаемый размах поддерева
						 *
						 * \~english
						 * @brief Method of the setting of the extent of the subtree of a container
						 * @param value extent of the subtree being set
						 *
						 * \~
						 */
						AWH_YAML_INLINE void extent(const uint32_t value) noexcept {
							// Устанавливаем размах поддерева вместилища
							this->content[1] = value;
						}
						/**
						 * \~russian
						 * @brief Шаблонный метод получения разобранного числа значения
						 *
						 * @tparam T вид получаемого числа
						 * @return   разобранное число значения
						 *
						 * \~english
						 * @brief Template method of the obtaining of the parsed number of a value
						 * @tparam T kind of the number being obtained
						 * @return parsed number of the value
						 *
						 * \~
						 */
						template <typename T>
						AWH_YAML_INLINE T number_of() const noexcept {
							// Получаемое разобранное число значения
							T result = T();
							// Выполняем перенос разобранного числа значения
							::memcpy(&result, this->number, sizeof(result));
							// Выводим разобранное число значения
							return result;
						}
						/**
						 * \~russian
						 * @brief Шаблонный метод установки разобранного числа значения
						 *
						 * @tparam T     вид устанавливаемого числа
						 * @param  value устанавливаемое число значения
						 *
						 * \~english
						 * @brief Template method of the setting of the parsed number of a value
						 * @tparam T kind of the number being set
						 * @param value number of the value being set
						 *
						 * \~
						 */
						template <typename T>
						AWH_YAML_INLINE void number_of(const T value) noexcept {
							// Выполняем сброс разобранного числа значения
							this->number[0] = 0;
							// Выполняем сброс старшей половины разобранного числа
							this->number[1] = 0;
							// Выполняем перенос устанавливаемого числа значения
							::memcpy(this->number, &value, sizeof(value));
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
						 type(type_t::UNDEFINED), style(style_t::PLAIN), keyed(false), touched(false),
						 origin(NO_ORIGIN), dwelling(NO_ORIGIN), edge(NO_ORIGIN), offset(0), named(0), props(0), content{0, 1}, number{0, 0} {}
					} node_t;
				public:
					/**
					 * \~russian
					 * @brief Ссылка на узел дерева документа
					 *
					 * @details Ссылка есть указатель на узел, а не сам узел: копируется она
					 * даром, а узлы всё это время лежат в документе одним куском
					 *
					 * @warning Ссылка действительна, покуда документ не перестроен: разбор
					 *          нового текста перестраивает перечень узлов, и всякая ссылка,
					 *          прежде взятая, указывает после того неизвестно куда
					 *
					 * \~english
					 * @brief Reference to a node of the tree of a document
					 * @details A reference is a pointer to a node rather than the node itself: it is copied
					 * for free, while the nodes lie in the document as one piece all this time
					 * @warning A reference is valid as long as the document is not rebuilt
					 *
					 * \~
					 */
					typedef class __AWH_SHARED_EXPORT__ Value {
						private:
							/**
							 * Документ, дерево какого правится, ссылке доверяется
							 *
							 * @note Правка ведётся документом, а не ссылкою: ссылка на узел смотрит,
							 *       а правит держащий дерево целиком - ему одному ведомы предки узла,
							 *       кои правкой тоже тронуты
							 */
							friend class Document;
						private:
							// Документ, которому принадлежит узел
							const Document * _doc;
							// Номер узла в перечне узлов документа
							uint32_t _index;
						private:
							/**
							 * \~russian
							 * @brief Клеймо поколения перечня узлов, при заведении ссылки снятое
							 *
							 * @details Ссылка живёт номером узла, а перестроение перечня номера
							 * смещает: без клейма ссылка, перестроение пережившая, отвечала бы
							 * годной и отдавала бы содержимое совсем другого узла
							 *
							 * \~english
							 * @brief Stamp of the generation of the list of the nodes taken at the creation of the reference
							 *
							 * \~
							 */
							uint32_t _stamp;
							/**
							 * Номер узла за последним узлом вместилища, какому узел принадлежит
							 *
							 * @details Указаний на родителя узел не несёт, а переход к соседу обязан
							 * останавливаться на границе своего вместилища: без границы обход
							 * перечня продолжился бы соседом родителя
							 */
							uint32_t _bound;
						private:
							/**
							 * \~russian
							 * @brief Шаблонный метод извлечения числа затребованным видом
							 *
							 * @tparam T      вид извлекаемого числа
							 * @param  result извлечённое число
							 * @return        признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Template method of the extraction of a number by a demanded kind
							 * @tparam T kind of the number being extracted
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							template <typename T>
							bool extract(T & result) const noexcept;
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
							 * @brief Method of the extraction of the kind of a node
							 * @return kind of the node of the document
							 *
							 * \~
							 */
							kind_t kind() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида значения узла
							 *
							 * @return вид значения узла документа
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of the value of a node
							 * @return kind of the value of the node of the document
							 *
							 * \~
							 */
							type_t type() const noexcept;
							/**
							 * \~russian
							 * @brief Метод проверки вида значения узла
							 *
							 * @param type проверяемый вид значения
							 * @return     признак соответствия вида значения
							 *
							 * \~english
							 * @brief Method of the check of the kind of the value of a node
							 * @param type kind of the value being checked
							 * @return sign of the correspondence of the kind of the value
							 *
							 * \~
							 */
							bool is(const type_t type) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения вида записи значения
							 *
							 * @return вид записи значения в исходном тексте
							 *
							 * \~english
							 * @brief Method of the extraction of the kind of the notation of a value
							 * @return kind of the notation of the value in the source text
							 *
							 * \~
							 */
							style_t style() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения схемы, над разбором действовавшей
							 *
							 * @details Схема эта настройками задана не всегда: директива
							 * `%YAML 1.1` переводит разбор на схему наречия 1.1. Знать её надобно
							 * тому, кто значение с дерева снимает: запись `on` логическою является
							 * лишь под нею, и перезапись иною схемою переменила бы смысл
							 *
							 * @return схема, над разбором действовавшая
							 *
							 * \~english
							 * @brief Method of the extraction of the schema which was acting over the parsing
							 * @details This schema is not always set by the settings: the `%YAML 1.1` directive
							 * switches the parsing to the schema of the 1.1 dialect. It is needed by whoever
							 * takes a value off the tree: the record `on` is a logical one only under it,
							 * and a rewriting by another schema would change the meaning
							 * @return schema which was acting over the parsing
							 *
							 * \~
							 */
							schema_t schema() const noexcept;
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
							 * @return признак пустоты вместилища
							 *
							 * \~english
							 * @brief Method of the check of a container for the emptiness
							 * @return sign of the emptiness of the container
							 *
							 * \~
							 */
							bool empty() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени пары отображения
							 *
							 * @return имя пары отображения, пусто у значения перечня
							 *
							 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
							 *          содержимое видами в своё хранилище знаков, а правка его
							 *          наращивает и по мере накопления мусора уплотняет, хранилище
							 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
							 *          снимайте копию
							 *
							 * \~english
							 * @brief Method of the extraction of the name of a pair of a mapping
							 * @return name of the pair of the mapping, empty for a value of a sequence
							 *
							 * @warning The issued views live until the next EDITING of the tree. The tree issues
							 *          the content by the views into its storage of the characters, while an editing grows it
							 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
							 *          before an editing dangles. Should a view be needed longer — take a copy
							 *
							 * \~
							 */
							string_view name() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения записи значения, исходным текстом данной
							 *
							 * @details Запись выдаётся приведённой к окончательному виду: ограда с
							 * неё снята, а отменяющие последовательности раскрыты. Запись числа
							 * выдаётся тою же, какою она в тексте и стояла - `0x1F` останется
							 * записью `0x1F`, а не обратится числом 31
							 *
							 * @return запись значения узла
							 *
							 * @warning Выданные виды живут до следующей ПРАВКИ дерева. Дерево выдаёт
							 *          содержимое видами в своё хранилище знаков, а правка его
							 *          наращивает и по мере накопления мусора уплотняет, хранилище
							 *          перемещая: вид, взятый до правки, повисает. Нужен вид дольше -
							 *          снимайте копию
							 *
							 * \~english
							 * @brief Method of the extraction of the record of a value given by the source text
							 * @details The record is issued brought to the final kind: the quoting is removed from it,
							 * and the escape sequences are expanded
							 * @return record of the value of the node
							 *
							 * @warning The issued views live until the next EDITING of the tree. The tree issues
							 *          the content by the views into its storage of the characters, while an editing grows it
							 *          and compacts it as the garbage accumulates, relocating the storage: a view taken
							 *          before an editing dangles. Should a view be needed longer — take a copy
							 *
							 * \~
							 */
							string_view text() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения имени метки, узлу предпосланной
							 *
							 * @return имя метки узла, пусто при отсутствии её
							 *
							 * \~english
							 * @brief Method of the extraction of the name of the anchor placed before a node
							 * @return name of the anchor of the node, empty at the absence of it
							 *
							 * \~
							 */
							string_view anchor() const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения метки типа, узлу предпосланной
							 *
							 * @return метка типа узла, пусто при отсутствии её
							 *
							 * \~english
							 * @brief Method of the extraction of the tag placed before a node
							 * @return tag of the node, empty at the absence of it
							 *
							 * \~
							 */
							string_view tag() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод получения ссылки на пару отображения по имени её
							 *
							 * @param name имя разыскиваемой пары отображения
							 * @return     ссылка на найденную пару, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a pair of a mapping by its name
							 * @param name name of the pair of the mapping being searched for
							 * @return reference to the found pair, invalid at the absence
							 *
							 * \~
							 */
							Value operator [] (const string & name) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на значение перечня по номеру его
							 *
							 * @param index номер разыскиваемого значения перечня
							 * @return      ссылка на найденное значение, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a value of a sequence by its index
							 * @param index index of the value of the sequence being searched for
							 * @return reference to the found value, invalid at the absence
							 *
							 * \~
							 */
							Value operator [] (const size_t index) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на узел по пути к нему
							 *
							 * @details Путь записывается тем же порядком, каким записывает его
							 * контейнер JSON: части его отделяются косою чертою, а числом
							 * обозначается номер значения перечня. Путь `/server/hosts/0` ведёт к
							 * первому значению перечня `hosts` отображения `server`
							 *
							 * @param path путь к разыскиваемому узлу
							 * @return     ссылка на найденный узел, недействительная при отсутствии
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to a node by the path to it
							 * @details The path is written by the same order by which the JSON container writes it
							 * @param path path to the node being searched for
							 * @return reference to the found node, invalid at the absence
							 *
							 * \~
							 */
							Value at(const string & path) const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на соседний узел вместилища
							 *
							 * @return ссылка на соседний узел, недействительная за границей вместилища
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to the neighbouring node of a container
							 * @return reference to the neighbouring node, invalid beyond the boundary of the container
							 *
							 * \~
							 */
							Value next() const noexcept;
							/**
							 * \~russian
							 * @brief Метод получения ссылки на первого ребёнка вместилища
							 *
							 * @return ссылка на первого ребёнка, недействительная у пустого вместилища
							 *
							 * \~english
							 * @brief Method of the obtaining of a reference to the first child of a container
							 * @return reference to the first child, invalid for an empty container
							 *
							 * \~
							 */
							Value begin() const noexcept;
						public:
							/**
							 * \~russian
							 * @brief Метод извлечения логического значения
							 *
							 * @param result извлечённое логическое значение
							 * @return       признак успешного извлечения значения
							 *
							 * \~english
							 * @brief Method of the extraction of a boolean value
							 * @param result extracted boolean value
							 * @return sign of the successful extraction of the value
							 *
							 * \~
							 */
							bool value(bool & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в один байт
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer one byte wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в два байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer two bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в четыре байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer four bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа со знаком шириною в восемь байтов
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a signed integer eight bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(int64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в один байт
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer one byte wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint8_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в два байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer two bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint16_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в четыре байта
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer four bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint32_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения целого числа без знака шириною в восемь байтов
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of an unsigned integer eight bytes wide
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(uint64_t & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения дробного числа одинарной точности
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a floating point number of a single precision
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(float & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения дробного числа двойной точности
							 *
							 * @param result извлечённое число
							 * @return       признак успешного извлечения числа
							 *
							 * \~english
							 * @brief Method of the extraction of a floating point number of a double precision
							 * @param result extracted number
							 * @return sign of the successful extraction of the number
							 *
							 * \~
							 */
							bool value(double & result) const noexcept;
							/**
							 * \~russian
							 * @brief Метод извлечения строкового значения
							 *
							 * @param result извлечённое строковое значение
							 * @return       признак успешного извлечения значения
							 *
							 * \~english
							 * @brief Method of the extraction of a string value
							 * @param result extracted string value
							 * @return sign of the successful extraction of the value
							 *
							 * \~
							 */
							bool value(string & result) const noexcept;
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
							Value() noexcept : _doc(nullptr), _index(0), _bound(0) {}
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
							Value(const Document * doc, const uint32_t index, const uint32_t bound) noexcept :
							 _doc(doc), _index(index), _stamp((doc != nullptr) ? doc->_generation : 0), _bound(bound) {}
					} value_t;
					/**
					 * Открываем ссылке доступ к внутреннему устройству документа
					 *
					 * @note Доступ этот нужен ссылке затем, что живёт она узлами документа, а
					 *       выдавать наружу перечень узлов ради неё одной незачем
					 */
					friend class Value;
				private:
					/**
					 * \~russian
					 * @brief Метод вывода сообщения об отказе в лог
					 *
					 * @details Сообщение собирается из кода отказа да места его: код остаётся
					 * доступен потребителю через error(), а место - через location(), и журнал
					 * их не заменяет, а лишь оповещает о случившемся
					 *
					 * \~english
					 * @brief Method of the output of the message about a refusal into the log
					 * @details The message is composed of the code of the refusal and of its place: the code
					 * remains available to the consumer through error(), and the place through location(),
					 * and the log does not replace them but merely notifies about what has happened
					 *
					 * \~
					 */
					void report() const noexcept;
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * \~english
					 * Object for working with logs
					 *
					 * \~
					 */
					const log_t * _log;
				private:
					// Настройки разбора документа
					settings_t _settings;
					// Плоский перечень узлов дерева документа
					vector <node_t> _nodes;
				private:
					/**
					 * \~russian
					 * @brief Клеймо поколения перечня узлов
					 *
					 * @details Ссылка на узел держит НОМЕР его в перечне, а очистка, вставка и
					 * снос номера смещают: ссылка, перестроение пережившая, указывала бы на
					 * совсем другой узел - и отвечала бы годной, отдавая правдоподобное чужое
					 * содержимое. Ни кода отказа, ни падения при этом нет, и отличить подмену
					 * потребителю нечем вовсе
					 *
					 * @details Клеймо растёт при всяком перестроении, ссылка снимает его при
					 * заведении, а действительность сличает
					 *
					 * @note Дозапись узла в КОНЕЦ перечня клейма не трогает намеренно: смысла
					 * прежних номеров она не меняет, и клеймение её обрывало бы ссылки, взятые
					 * при обходе растущего дерева
					 *
					 * @warning Беду эту нашёл Василий у кодеков JSON и XML и передал соседям;
					 * у YAML она подтвердилась щупом - узел, снятый до повторного разбора,
					 * отвечал годным и отдавал содержимое НОВОГО документа
					 *
					 * \~english
					 * @brief Stamp of the generation of the list of the nodes
					 * @details A reference to a node holds the INDEX of it in the list, while a clearing,
					 * an insertion and a removal shift the indexes: a reference which has survived a rebuilding
					 * would point at an entirely different node
					 *
					 * \~
					 */
					uint32_t _generation;
					// Перечень свойств узлов, записью им предпосланных
					vector <props_t> _props;
					// Хранилище имён и записей значений
					string _storage;
					/**
					 * \~russian
					 * Указатели имён пар отображений, по требованию заведённые
					 *
					 * @details Ключом стоит номер узла отображения, значением - имена детей его,
					 * на номера узлов отображённые. Заводится указатель лишь тем отображениям,
					 * у каких пар больше `INDEX_THRESHOLD`, и лишь при первом обращении по имени:
					 * отображение, к какому по имени не обращались, не платит ничего
					 *
					 * @warning Имена держатся здесь видами на хранилище, а не своими записями, и
					 *          всякая правка дерева указатели те обращает в прах: перенос записи
					 *          в хранилище вправе переселить его целиком, а вставка узла и снос
					 *          его сдвигают номера. Оттого правка обязана указатели сбрасывать,
					 *          и сброс тот стоит у самих действий - у переноса записи, у вставки
					 *          узла и у сноса его, - а не у каждого способа наружного
					 *
					 * @warning Заведение идёт по требованию, из способа неизменяющего, оттого поле
					 *          и объявлено изменчивым: обращение по имени к одному дереву из двух
					 *          потоков разом требует ограды от потребителя. Дерево, к какому
					 *          обращаются лишь читая и лишь из одного потока, ограды не требует
					 *
					 * \~english
					 * Indexes of the names of the mappings, created on demand
					 * @details The key here is the number of the node of a mapping, the value — the names of its children
					 * mapped onto the numbers of the nodes. An index is created only for those mappings which have
					 * more pairs than `INDEX_THRESHOLD`, and only at the first access by a name: a mapping
					 * which has not been accessed by a name pays nothing
					 * @warning The names are held here as the views onto the storage rather than as their own records, and
					 *          any editing of the tree turns those indexes into a dust: a transfer of a record into
					 *          the storage is entitled to relocate it in full, while an insertion of a node and a removal
					 *          of it shift the numbers. Hence the editing must reset the indexes, and that reset stands
					 *          at the actions themselves — at the transfer of a record, at the insertion of a node and at
					 *          the removal of it — rather than at every outward method
					 *
					 * \~
					 */
					mutable unordered_map <uint32_t, unordered_map <string_view, uint32_t>> _index;
					/**
					 * Удержанный исходный текст, настройкою затребованный
					 *
					 * @note Держится он в кодировке подачи своей: смещения событий чтение
					 *       считает по тексту исходному, до приведения к UTF-8, и дословная
					 *       перезапись обязана вырезать из него же
					 */
					string _source;
					/**
					 * Длина метки порядка байтов в начале удержанного текста
					 *
					 * @note Смещения событий чтение считает по тексту, метки той лишённому, а
					 *       удержан текст поданным целиком: разница эта и есть длина метки, и
					 *       прибавляется она ко всякому смещению
					 */
					uint32_t _prologue;
					// Кодировка, какою текст прочитан
					encoding_t _encoding;
				private:
					/**
					 * \~russian
					 * Признак того, что наречие текста объявлено директивой
					 *
					 * @details Признак этот нужен перезаписи: директива `%YAML 1.1` переводит
					 * разбор на схему наречия 1.1, и перезапись без неё переменила бы смысл
					 * документа - запись `on`, логическая под 1.1, вернулась бы строкою
					 *
					 * \~english
					 * Sign that the dialect of the text is declared by a directive
					 * @details This sign is needed by the rewriting: the `%YAML 1.1` directive switches
					 * the parsing to the schema of the 1.1 dialect, and a rewriting without it would change the meaning
					 * of the document — the record `on`, logical under 1.1, would return as a string
					 *
					 * \~
					 */
					bool _versioned;
				private:
					// Схема, над разбором действовавшая
					schema_t _schema;
					// Номера корневых узлов документов текста
					vector <uint32_t> _roots;
					/**
					 * Наречия, каждым документом потока объявленные
					 *
					 * @details Директива `%YAML 1.1` переводит на наречие 1.1 один свой документ,
					 *          а поток вправе нести документы обоих наречий. Схема одна на поток
					 *          отдавала наречие документа последнего всякому узлу: записи `on` да
					 *          `0b1010`, прочтённые наречием 1.1, возвращались строкою у того, кто
					 *          снимал их владеющим значением. Нашёл это ворошитель круговым ходом
					 */
					vector <schema_t> _dialects;
					/**
					 * Признаки того, что документ наречие своё директивой объявил
					 *
					 * @note Директива есть принадлежность документа, а не потока, и стоять она
					 *       обязана перед каждою чертою начала своею: перезапись возвращает её
					 *       ровно тем документам, у каких она стояла
					 */
					vector <bool> _versions;
					/**
					 * Смещения начала документов в удержанном исходном тексте
					 *
					 * @details Перечень этот идёт об руку с перечнем корней: смещение отвечает
					 * началу строки, документ открывающей, - черте `---` либо первой строке
					 * содержимого его. Документ первый начинается всегда с нуля: директивы,
					 * черте предпосланные, принадлежат ему
					 */
					vector <uint32_t> _starts;
					// Код ошибки разбора текста
					error_t _error;
					// Положение отказа разбора в исходном тексте
					location_t _location;
				private:
					/**
					 * \~russian
					 * @brief Метод переноса записи в хранилище знаков
					 *
					 * @param text переносимая запись
					 * @return     смещение перенесённой записи в хранилище
					 *
					 * \~english
					 * @brief Method of the transfer of a record into the storage of the characters
					 * @param text record being transferred
					 * @return offset of the transferred record in the storage
					 *
					 * \~
					 */
					uint32_t deposit(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод постройки дерева по выданным чтением событиям
					 *
					 * @param reader чтение, события выдающее
					 * @return       признак успешной постройки дерева
					 *
					 * \~english
					 * @brief Method of the building of the tree by the events issued by the reading
					 * @param reader reading issuing the events
					 * @return sign of the successful building of the tree
					 *
					 * \~
					 */
					bool digest(reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по поддереву узла
					 *
					 * @param writer сборка текста
					 * @param index  номер узла, поддерево какого собирается
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the subtree of a node
					 * @param writer assembling of the text
					 * @param index index of the node whose subtree is being assembled
					 *
					 * \~
					 */
					void compose(writer_t & writer, const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения границы записи узла в удержанном исходном тексте
					 *
					 * @details Границею служит начало записи следующего узла обхода: всякий байт
					 * текста принадлежит ровно одному узлу, и примечания, между соседями стоящие,
					 * достаются тому, над кем они стоят
					 *
					 * @param index номер узла, границы записи какого получаются
					 * @return      смещение за концом записи узла, `NO_ORIGIN` - записи нет
					 *
					 * \~english
					 * @brief Method of the obtaining of the boundary of the record of a node in the retained source text
					 * @param index index of the node whose boundary of the record is being obtained
					 * @return offset past the end of the record of the node
					 *
					 * \~
					 */
					uint32_t bound(const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод дословной записи пролёта соседних узлов исходными байтами
					 *
					 * @details Пролёт переписывается дословно лишь тогда, когда ни один узел его
					 * правкой не тронут и отступ записи совпадает с отступом исходным: иначе
					 * перенесённые байты легли бы не на своё место
					 *
					 * @param writer сборка текста
					 * @param first  номер первого узла переписываемого пролёта
					 * @param last   номер последнего узла переписываемого пролёта
					 * @param entry  признак того, что пролёт переписывается записями перечня
					 * @return       признак успешной дословной записи пролёта
					 *
					 * \~english
					 * @brief Method of the verbatim writing of a run of the neighbouring nodes by the source bytes
					 * @param writer assembling of the text
					 * @param first index of the first node of the run being rewritten
					 * @param last index of the last node of the run being rewritten
					 * @return sign of the successful verbatim writing of the run
					 *
					 * \~
					 */
					bool verbatim(writer_t & writer, const uint32_t first, const uint32_t last, const bool entry, const bool gapped) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки детей вместилища пролётами нетронутых
					 *
					 * @param writer сборка текста
					 * @param index  номер узла вместилища, дети какого собираются
					 *
					 * \~english
					 * @brief Method of the assembling of the children of a container by the runs of the untouched ones
					 * @param writer assembling of the text
					 * @param index index of the node of the container whose children are being assembled
					 *
					 * \~
					 */
					bool enter(writer_t & writer, const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи предисловия узла дословными исходными байтами
					 *
					 * @param writer сборка текста
					 * @param index  номер узла, предисловие какого выдаётся
					 * @param gapped признак разрыва удержанного текста перед записью узла
					 *
					 * \~english
					 * @brief Method of the output of the preface of a node by the verbatim source bytes
					 * @param writer assembling of the text
					 * @param index index of the node whose preface is being output
					 * @param gapped sign of the gap of the retained text before the record of the node
					 *
					 * \~
					 */
					void preamble(writer_t & writer, const uint32_t index, const bool gapped) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения начала собственной строки узла
					 *
					 * @details Начало записи узла считается вместе с примечаниями да пустыми
					 * строками, ему предшествующими, а строка собственная стоит за ними: розыск
					 * её пропускает строки пустые и несущие одно примечание
					 *
					 * @param index номер узла, собственная строка какого разыскивается
					 * @return      смещение начала собственной строки, `NO_ORIGIN` - записи нет
					 *
					 * \~english
					 * @brief Method of the obtaining of the beginning of the own line of a node
					 * @param index index of the node whose own line is being sought
					 * @return offset of the beginning of the own line of the node
					 *
					 * \~
					 */
					uint32_t leading(const uint32_t index) const noexcept;
					/**
					 * @brief Метод подрезки границ записей узлов, в снимаемый упирающихся
					 *
					 * @param index номер снимаемого узла
					 *
					 */
					void tighten(const uint32_t index) noexcept;
					/**
					 * @brief Метод получения конца содержимого блочного значения, документ замыкающего
					 *
					 * @param root номер корневого узла документа
					 * @return     смещение за концом содержимого блока, `NO_ORIGIN` - блока нет
					 *
					 */
					uint32_t trailing(const uint32_t root) const noexcept;
					/**
					 * @brief Метод опознания блочного значения, хвост свой сохраняющего
					 *
					 * @param index номер опознаваемого узла
					 * @return      признак блочного значения, хвост свой сохраняющего
					 *
					 */
					bool hanging(const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод отнесения пустых строк к узлам, под ними стоящим
					 *
					 * @details Пустая строка, узлу предшествующая, отделяет его от соседа сверху и
					 * уходить обязана вместе с ним. Чтение о таких строках не извещает - простое
					 * значение, выдачи ожидающее, считает их складками своими, - и разыскиваются
					 * они по удержанному тексту прямо
					 *
					 * @note Розыск не заходит за узел, блочное значение несущий: пустые строки в
					 *       конце его суть содержимое, правилом усечения сохранённое, а вовсе не
					 *       отступ перед соседом
					 *
					 * \~english
					 * @brief Method of the attribution of the empty lines to the nodes standing under them
					 * @details An empty line preceding a node separates it from the neighbour above and must leave together with it
					 * @note The search does not go past a node carrying a block scalar
					 *
					 * \~
					 */
					void spread() noexcept;
					/**
					 * \~russian
					 * @brief Метод пометки узла и предков его правлеными
					 *
					 * @param index номер помечаемого узла
					 *
					 * \~english
					 * @brief Method of the marking of a node and of its ancestors as edited
					 * @param index index of the node being marked
					 *
					 * \~
					 */
					void mark(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи имени и значения узла в хранилище знаков
					 *
					 * @details Вид значения решается тем же телом, каким решает его разбор: запись
					 * кладётся в хранилище, а разрешение да сужение числа зовутся над нею.
					 * Иначе два свода правил разошлись бы, и число, правкой поставленное, читалось
					 * бы иначе, нежели то же число, текстом прочитанное
					 *
					 * @param index номер записываемого узла
					 * @param name  имя пары, пусто у значения перечня
					 * @param text  запись значения, исходным текстом данная
					 * @param style ограда, какою обносится значение
					 *
					 * \~english
					 * @brief Method of the writing of the name and of the value of a node into the storage of the characters
					 * @param index index of the node being written
					 * @param name name of the pair, empty for a value of a sequence
					 * @param text record of the value as given by the source text
					 * @param style quoting by which the value is enclosed
					 *
					 * \~
					 */
					void inscribe(const uint32_t index, const string_view name, const string_view text, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения узла последним ребёнком вместилища
					 *
					 * @param owner номер вместилища, ребёнок какого заводится
					 * @return      номер заведённого узла
					 *
					 * \~english
					 * @brief Method of the creation of a node as the last child of a container
					 * @param owner index of the container whose child is being created
					 * @return index of the created node
					 *
					 * \~
					 */
					uint32_t implant(const uint32_t owner) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления дерева правленым при уходе метки
					 *
					 * @details Метка, правкой уходящая, оставила бы ссылки на себя без
					 * объявления, а ссылки эти держатся дословным переносом исходных байтов:
					 * раскрытие их дерево помнит, а запись `*метка` вернулась бы в перезапись
					 * как есть. Оттого дерево целиком объявляется правленым - раскрытия
					 * соберутся заново значениями своими, и ссылок в тексте не останется
					 *
					 * @note Правило это едино для снятия узла и для замены его скалярным
					 *       значением: уходит поддерево и там, и там, а разойдись оно
					 *       телами - один из путей потерял бы его молча
					 *
					 * @param index  номер первого узла уходящего поддерева
					 * @param extent размах уходящего поддерева в узлах
					 *
					 * \~english
					 * @brief Method of the declaring of a tree as edited at the leaving of an anchor
					 * @param index index of the first node of the leaving subtree
					 * @param extent extent of the leaving subtree in the nodes
					 *
					 * \~
					 */
					void disown(const uint32_t index, const uint32_t extent) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения конца последней строки записи поддерева
					 *
					 * @details Границы записей узлов к этому времени ещё не сочтены: считаются
					 * они по началам записей, а начала эти правятся переносом черты. Оттого
					 * нижний предел переноса берётся не границею прежней записи, а концом
					 * последней строки её - тем, что известно и до счёта границ
					 *
					 * @param index    номер узла, поддерево какого обходится
					 * @param boundary предел, ниже какого опускаться нельзя
					 * @return         смещение за концом последней строки записи
					 *
					 * \~english
					 * @brief Method of the obtaining of the end of the last line of the record of a subtree
					 * @param index index of the node whose subtree is being traversed
					 * @param boundary limit below which it is not allowed to descend
					 * @return offset past the end of the last line of the record
					 *
					 * \~
					 */
					uint32_t closing(const uint32_t index, const uint32_t boundary) const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия узла вместе с поддеревом его
					 *
					 * @param index номер снимаемого узла
					 * @return      признак успешного снятия узла
					 *
					 * \~english
					 * @brief Method of the removal of a node together with its subtree
					 * @param index index of the node being removed
					 * @return sign of the successful removal of the node
					 *
					 * \~
					 */
					bool extract(const uint32_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора повторяющихся имён пар отображения
					 *
					 * @details Описание языка имена пар отображения объявляет неповторимыми, а
					 * обхождение с повторами оставляет читающему: правило берётся из настроек
					 * разбора, и по умолчанию повтор отвергается отказом
					 *
					 * @note Разбор ведётся при закрытии отображения, а не при заведении пары:
					 *       имя пары становится известно раньше значения её, но поддерево
					 *       значения дописывается позже, и снимать пару прежде конца нельзя
					 *
					 * @param parent  номер узла разбираемого отображения пар
					 * @param anchors имена меток, документом объявленные, вместе с номерами узлов их
					 * @param reader  поток разбора, положение отказа несущий
					 * @return        признак успешного разбора
					 *
					 * \~english
					 * @brief Method of the handling of the repeating names of the pairs of a mapping
					 * @param parent index of the node of the mapping being handled
					 * @param reader parsing stream carrying the location of a refusal
					 * @return sign of the successful handling
					 *
					 * \~
					 */
					bool deduplicate(const uint32_t parent, ::std::unordered_map <::std::string, uint32_t> & anchors, const reader_t & reader) noexcept;
					/**
					 * \~russian
					 * @brief Метод розыска узла по пути к нему с заведением недостающего
					 *
					 * @param path   путь к разыскиваемому узлу
					 * @param index  номер найденного либо заведённого узла
					 * @param create признак заведения узла, розыском не найденного
					 * @return       признак успешного розыска узла
					 *
					 * \~english
					 * @brief Method of the search of a node by the path to it with the creation of a missing one
					 * @param path path to the node being sought
					 * @param index index of the found or created node
					 * @param create sign of the creation of a node not found by the search
					 * @return sign of the successful search of the node
					 *
					 * \~
					 */
					bool place(const string & path, uint32_t & index, const bool create) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения узла записью его без ограды
					 *
					 * @note Ограда не решается содержимым, а не ставится вовсе: запись числа,
					 *       логического значения да пустоты обязана вернуться обратным чтением тем
					 *       же видом, а ограда обратила бы её в строку
					 *
					 * @param path путь к устанавливаемому узлу
					 * @param text устанавливаемая запись значения
					 * @return     признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of a node by its record without the quoting
					 * @param path path to the node being set
					 * @param text record of the value being set
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool settle(const string & path, const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения узла заданною оградою
					 *
					 * @param index номер устанавливаемого узла
					 * @param text  устанавливаемая запись значения
					 * @param style ограда, какою обносится значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value of a node by a given quoting
					 * @param index index of the node being set
					 * @param text record of the value being set
					 * @param style quoting by which the value is enclosed
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					/**
					 * \~russian
					 * @brief Метод снятия детей узла
					 *
					 * @details Вместилище, значением иным заменяемое, детей своих лишается:
					 * держать их некуда, и записаны они уже не будут
					 *
					 * @param index номер узла, детей лишаемого
					 *
					 * \~english
					 * @brief Method of the removal of the children of a node
					 * @details A container being replaced by another value loses its children:
					 * there is nowhere to hold them, and they will not be written any more
					 * @param index number of the node being deprived of the children
					 *
					 * \~
					 */
					void prune(const uint32_t index) noexcept;
					bool assign(const uint32_t index, const string_view text, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи узла вместе с примечаниями, ему предпосланными
					 *
					 * @details Узел, правкой тронутый, собирается заново, а примечания над ним
					 * стоящие переносятся дословно: правка значения примечания над ним не
					 * касается, и терять его было бы неправдой
					 *
					 * @param writer сборка текста
					 * @param index  номер записываемого узла
					 *
					 * \~english
					 * @brief Method of the writing of a node together with the comments placed before it
					 * @param writer assembling of the text
					 * @param index index of the node being written
					 *
					 * \~
					 */
					void produce(writer_t & writer, const uint32_t index, const bool preface = true, const bool gapped = false) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста в дерево документа
					 *
					 * @param text разбираемый текст
					 * @return     признак успешного разбора текста
					 *
					 * \~english
					 * @brief Method of the parsing of a text into the tree of a document
					 * @param text text being parsed
					 * @return sign of the successful parsing of the text
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод чтения текста из файла в дерево документа
					 *
					 * @param filename путь к читаемому файлу
					 * @return         признак успешного чтения текста
					 *
					 * \~english
					 * @brief Method of the reading of a text from a file into the tree of a document
					 * @param filename path to the file being read
					 * @return sign of the successful reading of the text
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по дереву документа
					 *
					 * @return собранный текст документа
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the tree of a document
					 * @return assembled text of the document
					 *
					 * \~
					 */
					string dump() const noexcept;
					/**
					 * \~russian
					 * @brief Метод пометки узла правленым
					 *
					 * @details Помечается им не один узел, а весь путь до корня документа: запись
					 * узла лежит внутри записи родителя его, и дословно переписать родителя после
					 * правки ребёнка уже нельзя. Узел помеченный собирается заново, а соседи его,
					 * правкой не тронутые, переписываются дословными исходными байтами
					 *
					 * @note Пометка нужна лишь при удержании исходного текста: без него собирается
					 *       заново весь документ, и различать тронутое от нетронутого не для чего
					 *
					 * @param value ссылка на помечаемый узел
					 * @return      признак успешной пометки узла
					 *
					 * \~english
					 * @brief Method of the marking of a node as edited
					 * @details It marks not one node but the whole path up to the root of the document
					 * @param value reference to the node being marked
					 * @return sign of the successful marking of the node
					 *
					 * \~
					 */
					bool touch(const value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки строкового значения по пути к нему
					 *
					 * @details Ограда выбирается содержимым, коли не задана прямо: запись `12`,
					 * значением строковым поставленная, ограду получает - иначе обратное чтение
					 * вернуло бы её числом. Путь ведётся от корня первого документа, части его
					 * делятся косою чертой, а часть внутри перечня есть номер значения
					 *
					 * @note Узла по пути может и не быть: недостающая пара отображения заводится
					 *       последнею, а значение перечня добавляется концом его. Части пути,
					 *       кроме последней, обязаны быть налицо - вместилищ по пути не заводится
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое строковое значение
					 * @param style ограда, какою обносится значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a string value by the path to it
					 * @param path path to the node being set
					 * @param value string value being set
					 * @param style quoting by which the value is enclosed
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const string_view value, const style_t style = style_t::PLAIN) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки логического значения по пути к нему
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое логическое значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a boolean value by the path to it
					 * @param path path to the node being set
					 * @param value boolean value being set
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки целого значения по пути к нему
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое целое значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of an integer value by the path to it
					 * @param path path to the node being set
					 * @param value integer value being set
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки дробного значения по пути к нему
					 *
					 * @note Записывается оно семнадцатью значащими разрядами - столько нужно,
					 *       чтобы всякое число двойной точности вернулось обратным чтением тем же
					 *
					 * @param path  путь к устанавливаемому узлу
					 * @param value устанавливаемое дробное значение
					 * @return      признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a floating point value by the path to it
					 * @param path path to the node being set
					 * @param value floating point value being set
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool set(const string & path, const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки пустого значения по пути к нему
					 *
					 * @note Пустое значение от снятия узла отлично: пара остаётся налицо, а
					 *       значения при ней нет
					 *
					 * @param path путь к устанавливаемому узлу
					 * @return     признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of an empty value by the path to it
					 * @param path path to the node being set
					 * @return sign of the successful setting of the value
					 *
					 * \~
					 */
					bool reset(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления узла вместилищем
					 *
					 * @details Объявленное вместилище пусто, а наполняется оно установкою
					 * значений по пути внутрь него: части пути, кроме последней, обязаны быть
					 * налицо, и объявление это их и заводит. Прежнее содержимое узла
					 * объявлением снимается - вместе со всем поддеревом его
					 *
					 * @note Узла по пути может и не быть: заводится он тем же порядком, каким
					 *       заводит его установка значения
					 *
					 * @param path     путь к объявляемому узлу
					 * @param sequence признак объявления перечня значений заместо отображения пар
					 * @return         признак успешного объявления вместилища
					 *
					 * \~english
					 * @brief Method of the declaring of a node as a container
					 * @details The declared container is empty and is filled by the setting of the
					 * values by a path inside it: the parts of a path except the last one must be
					 * present, and this declaring establishes them. The previous content of the node
					 * is removed by the declaring — together with the whole of its subtree
					 * @note The node by the path may be absent: it is established by the same order by which
					 *       the setting of a value establishes it
					 * @param path     path to the node being declared
					 * @param sequence flag of the declaring of a sequence of the values instead of a mapping of the pairs
					 * @return         sign of the successful declaring of the container
					 *
					 * \~
					 */
					bool arrange(const string & path, const bool sequence = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки значения дословною записью
					 *
					 * @details Ограды запись не получает никакой: кладётся она в дерево как
					 * подана, и вид значения решается обратным чтением её. Тем и отличается
					 * установка эта от строковой: та ограду подбирает содержимым, дабы запись
					 * `12` числом не обернулась
					 *
					 * @note Способ этот берут значения, вида родного не имеющие вовсе -
					 *       отметки времени, двоичное содержимое и числа, ни в один родной вид
					 *       не вместимые: записью своею они и держатся
					 *
					 * @param path   путь к устанавливаемому узлу
					 * @param record устанавливаемая запись значения
					 * @return       признак успешной установки значения
					 *
					 * \~english
					 * @brief Method of the setting of a value by a verbatim record
					 * @details The record receives no quoting at all: it is laid into the tree as
					 * it is given, and the kind of the value is decided by the reading back of it
					 * @note This way is taken by the values having no native kind at all —
					 *       the timestamps, the binary content and the numbers not fitting into any native kind
					 * @param path   path to the node being set
					 * @param record record of the value being set
					 * @return       sign of the successful setting of the value
					 *
					 * \~
					 */
					bool imprint(const string & path, const string_view record) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия узла вместе с поддеревом его
					 *
					 * @details Снимается узел целиком - вместе с именем пары, значением её и всем
					 * поддеревом. Записи его в исходном тексте достаются небытию: перезапись их не
					 * переносит, а примечания, над узлом стоявшие, уходят вместе с ним
					 *
					 * @param path путь к снимаемому узлу
					 * @return     признак успешного снятия узла
					 *
					 * \~english
					 * @brief Method of the removal of a node together with its subtree
					 * @param path path to the node being removed
					 * @return sign of the successful removal of the node
					 *
					 * \~
					 */
					bool erase(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста по дереву документа заданными настройками
					 *
					 * @param settings настройки записи собираемого текста
					 * @return         собранный текст документа
					 *
					 * \~english
					 * @brief Method of the assembling of a text by the tree of a document by given settings
					 * @param settings settings of the writing of the text being assembled
					 * @return assembled text of the document
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи текста документа в файл
					 *
					 * @param filename путь к записываемому файлу
					 * @return         признак успешной записи текста
					 *
					 * \~english
					 * @brief Method of the writing of the text of a document into a file
					 * @param filename path to the file being written
					 * @return sign of the successful writing of the text
					 *
					 * \~
					 */
					bool save(const string & filename) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения количества документов текста
					 *
					 * @return количество документов, текстом несомых
					 *
					 * \~english
					 * @brief Method of the obtaining of the number of the documents of a text
					 * @return number of the documents carried by the text
					 *
					 * \~
					 */
					size_t documents() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кодировки, какою текст прочитан
					 *
					 * @details Опознаётся она по метке порядка байтов, а при отсутствии её - по
					 * расположению нулевых байтов в первых четырёх октетах, либо навязывается
					 * настройками прямо
					 *
					 * @note Удержание исходного текста работает лишь у кодировки UTF-8: смещения
					 *       событий чтение считает по тексту, к ней приведённому, и дословный
					 *       перенос из текста иной кодировки лёг бы вперемешку
					 *
					 * @return кодировка, какою текст прочитан
					 *
					 * \~english
					 * @brief Method of the obtaining of the encoding by which the text has been read
					 * @return encoding by which the text has been read
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения ссылки на корень первого документа текста
					 *
					 * @return ссылка на корень документа
					 *
					 * \~english
					 * @brief Method of the obtaining of a reference to the root of the first document of a text
					 * @return reference to the root of the document
					 *
					 * \~
					 */
					value_t root() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения ссылки на корень документа по номеру его
					 *
					 * @param index номер документа в потоке текста
					 * @return      ссылка на корень документа
					 *
					 * \~english
					 * @brief Method of the obtaining of a reference to the root of a document by its index
					 * @param index index of the document in the stream of the text
					 * @return reference to the root of the document
					 *
					 * \~
					 */
					value_t root(const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения количества узлов дерева документа
					 *
					 * @return количество узлов дерева
					 *
					 * \~english
					 * @brief Method of the obtaining of the number of the nodes of the tree of a document
					 * @return number of the nodes of the tree
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки дерева документа на пустоту
					 *
					 * @return признак пустоты дерева документа
					 *
					 * \~english
					 * @brief Method of the check of the tree of a document for the emptiness
					 * @return sign of the emptiness of the tree of the document
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора текста
					 *
					 * @return код ошибки разбора текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the error code of the parsing of a text
					 * @return error code of the parsing of a text
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения отказа в исходном тексте
					 *
					 * @return положение отказа разбора в исходном тексте
					 *
					 * \~english
					 * @brief Method of the obtaining of the position of a refusal in the source text
					 * @return position of the refusal of the parsing in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса дерева документа
					 *
					 * \~english
					 * @brief Method of the reset of the tree of a document
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора документа
					 *
					 * @return настройки разбора документа
					 *
					 * \~english
					 * @brief Method of the obtaining of the settings of the parsing of a document
					 * @return settings of the parsing of a document
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора документа
					 *
					 * @param settings устанавливаемые настройки разбора
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a document
					 * @param settings settings of the parsing being set
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Document(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки разбора документа
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the parsing of a document
					 *
					 * \~
					 */
					Document(const log_t * log, const settings_t & settings) noexcept;
			} document_t;
		};
	};
};

#endif // __AWH_CODEC_YAML_DOCUMENT__
