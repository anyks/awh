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
#include <vector>
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
			typedef class __AWH_SHARED_EXPORT__ Document {
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
						uint32_t depth;
						// Наибольшая допустимая длина скалярного значения, ноль - предел по умолчанию
						uint32_t scalar;
						/**
						 * Наибольшее допустимое количество узлов раскрытия ссылок
						 *
						 * @note Предел этот стережёт от беды, известной под именем миллиарда
						 *       смешков: девять меток, каждая из которых десятикратно повторяет
						 *       предыдущую, раскрываются в миллиард узлов из текста в двести байт.
						 *       Потоковое чтение ссылок не раскрывает вовсе и предела того не
						 *       знает, а дерево раскрывает - ему и стеречь
						 */
						uint32_t expansion;
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
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Properties() noexcept : anchor(0), anchored(0), tag(0), tagged(0) {}
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
						 offset(0), named(0), props(0), content{0, 1}, number{0, 0} {}
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
							// Документ, которому принадлежит узел
							const Document * _doc;
							// Номер узла в перечне узлов документа
							uint32_t _index;
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
							 * \~english
							 * @brief Method of the extraction of the name of a pair of a mapping
							 * @return name of the pair of the mapping, empty for a value of a sequence
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
							 * \~english
							 * @brief Method of the extraction of the record of a value given by the source text
							 * @details The record is issued brought to the final kind: the quoting is removed from it,
							 * and the escape sequences are expanded
							 * @return record of the value of the node
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
							 _doc(doc), _index(index), _bound(bound) {}
					} value_t;
					/**
					 * Открываем ссылке доступ к внутреннему устройству документа
					 *
					 * @note Доступ этот нужен ссылке затем, что живёт она узлами документа, а
					 *       выдавать наружу перечень узлов ради неё одной незачем
					 */
					friend class Value;
				private:
					// Настройки разбора документа
					settings_t _settings;
					// Плоский перечень узлов дерева документа
					vector <node_t> _nodes;
					// Перечень свойств узлов, записью им предпосланных
					vector <props_t> _props;
					// Хранилище имён и записей значений
					string _storage;
					// Номера корневых узлов документов текста
					vector <uint32_t> _roots;
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
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Document() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки разбора документа
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the parsing of a document
					 *
					 * \~
					 */
					explicit Document(const settings_t & settings) noexcept;
			} document_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_DOCUMENT__
