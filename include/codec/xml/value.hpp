/**
 * @file value.hpp
 * @date 2026-08-18
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
 * @brief Заголовочный файл владеющего значения XML — самостоятельный тип данных,
 *        хранящий поддерево разметки собственной памятью, собираемый из значений языка
 *        и пригодный к передаче наружу как обычное значение
 *
 * \~english
 * @brief Header file of the owning value of XML — a standalone data type
 *        storing a subtree of the markup by its own memory, assembled from the values of the language
 *        and suitable for the passing outwards as an ordinary value
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_VALUE__
#define __AWH_CODEC_XML_VALUE__

/**
 * Подключаем заголовочные файлы модуля
 */
/**
 * Стандартные заголовочные файлы
 *
 * @note Свойства типов нужны обращению по написанию знака: ограда пропускает ровно
 *       разряд `char`, а перечнем это не лечится - заводитель по знаку делает
 *       двусмысленным обычное `v[0]`
 */
#include <type_traits>

#include "writer.hpp"
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
		 * @brief Пространство имён контейнера XML
		 *
		 * \~english
		 * @brief XML container namespace
		 *
		 * \~
		 */
		namespace xml {
			/**
			 * \~russian
			 * @brief Владеющее значение XML
			 *
			 * @details Тип этот стоит **над** деревом разметки, а не вместо него. Дерево
			 * разбирает текст и хранит узлы ареной, отчего узел `Document::Node` владельцем
			 * не является и дерево пережить не может
			 *
			 * @details Владеющее значение решает ровно ту задачу, какую узел решать не
			 * вправе: оно держит своё поддерево собственной памятью и потому складывается
			 * из значений языка, копируется, кладётся во вместилища языка, передаётся
			 * вглубь и **отдаётся наружу итогом метода**
			 *
			 * @details Облик этого типа общий у всех кодеков рамки: имена действий и правила
			 * их поведения одинаковы у JSON, XML, YAML, TOML и INI. Своеобразия разметки
			 * прибавлены сверху, а не втиснуты в общее ядро: имя с пространством имён,
			 * свойства узла, связывания префиксов и девять видов узла против шести у JSON
			 *
			 * @details Мост обратный - `Document::graft`: значение, наружу отданное и правке
			 * подвергнутое, становится обратно на место узла дерева. Арена дерева при том
			 * лишь дописывается, а узлы заменённого поддерева остаются в ней недостижимыми:
			 * перенумерование их обесценило бы всякую ссылку, выданную наружу прежде
			 *
			 * @note Оформления исходного текста значение не удерживает: расстановка
			 * пробелов принадлежит дереву, и правка чужого текста с сохранением его
			 * неприкосновенности остаётся за деревом. Это два разных употребления
			 *
			 * \~english
			 * @brief Owning value of XML
			 * @details This type stands **above** the markup tree rather than instead of it. The tree
			 * parses a text and stores the nodes in an arena, whereby a node `Document::Node` is not an owner
			 * and cannot outlive the tree
			 * @details The owning value solves exactly the task which a node has no
			 * right to solve: it holds its subtree by its own memory and therefore is assembled
			 * from the values of the language, is copied, is placed into the containers of the language, is passed
			 * inwards and **is given away outwards as the result of a method**
			 * @details The shape of this type is common for all the codecs of the framework: the names of the actions and the rules
			 * of their behaviour are identical for JSON, XML, YAML, TOML and INI. The peculiarities of the markup
			 * are added on top rather than squeezed into the common core: a name with a namespace,
			 * the properties of a node, the bindings of the prefixes and nine kinds of a node against six for JSON
			 * @details The reverse bridge is `Document::graft`: a value given away outwards and subjected
			 * to editing takes its place back at a node of the tree. The arena of the tree is thereby
			 * only appended to, while the nodes of the replaced subtree remain in it unreachable:
			 * a renumbering of them would invalidate every reference given away outwards before
			 * @note The value does not retain the formatting of the source text: the placement
			 * of the spaces belongs to the tree, and the editing of a foreign text with the preservation of its
			 * intactness remains with the tree. These are two different usages
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				public:
					/**
					 * \~russian
					 * @brief Свойство владеющего значения
					 *
					 * @details Свойство хранится строками собственными, а не отрезками чужого
					 * текста: значение владеющее дерево пережить обязано
					 *
					 * @note Тип этот заведён вложенным намеренно: свойство узла дерева,
					 *       живущее отрезками разбираемого текста, зовётся `attribute_t` и
					 *       лежит рядом, в пространстве имён кодека
					 *
					 * \~english
					 * @brief Property of an owning value
					 * @details A property is stored by its own strings rather than by the segments of a foreign
					 * text: an owning value must outlive the tree
					 * @note This type is made nested deliberately: a property of a node of a tree,
					 *       living by the segments of the text being parsed, is called `attribute_t` and
					 *       lies alongside, in the namespace of the codec
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Property {
						// Префикс пространства имён без разделителя
						string prefix;
						// Местное имя без префикса
						string local;
						// Обозначение связанного пространства имён
						string uri;
						// Значение свойства, приведённое к окончательному виду
						string value;
					} property_t;
					/**
					 * \~russian
					 * @brief Связывание префикса с пространством имён
					 *
					 * @details Связывание принадлежит узлу, где объявлено, и действует внутри
					 * всех вложенных в него, пока не будет переопределено
					 *
					 * \~english
					 * @brief Binding of a prefix to a namespace
					 * @details A binding belongs to the node where it is declared and acts inside
					 * all the ones nested into it until it is redefined
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Namespace {
						// Префикс без разделителя, пустой у объявления по умолчанию
						string prefix;
						// Обозначение пространства имён, пустое у отмены связывания
						string uri;
					} namespace_t;
				private:
					// Вид хранимого узла
					kind_t _kind;
				private:
					/**
					 * \~russian
					 * Префикс пространства имён имени узла
					 *
					 * @note Префикс удерживается наравне с обозначением пространства имён, хотя
					 * сличению имён он и не подлежит: без него запись выданного текста
					 * разошлась бы с записью исходного, а отвечающие по UPnP ставят
					 * префиксы всякий по-своему
					 *
					 * \~english
					 * Prefix of the namespace of the name of the node
					 * @note The prefix is retained on a par with the designation of the namespace, although
					 * it is not subject to the comparison of the names: without it the record of the issued text
					 * would diverge from the record of the source one, while those answering by UPnP put
					 * the prefixes each in its own way
					 *
					 * \~
					 */
					string _prefix;
				private:
					/**
					 * \~russian
					 * Местное имя узла разметки, а у указания обработчику - цель его
					 *
					 * \~english
					 * Local name of a markup node, and for a processing instruction — its target
					 *
					 * \~
					 */
					string _local;
				private:
					// Обозначение пространства имён узла разметки
					string _uri;
				private:
					/**
					 * \~russian
					 * Собственное содержимое узла
					 *
					 * @details Содержимым владеют узлы текстовые, дословные разделы, примечания,
					 * указания обработчику и описание типа документа. У узла разметки
					 * собственного содержимого нет вовсе: содержимое его лежит вложенными узлами
					 *
					 * \~english
					 * Own content of the node
					 * @details The content is owned by the text nodes, the literal sections, the comments,
					 * the processing instructions and the description of the type of the document. A markup node
					 * has no own content at all: its content lies in the nested nodes
					 *
					 * \~
					 */
					string _text;
				private:
					// Свойства узла разметки в порядке их следования
					vector <property_t> _attributes;
				private:
					// Связывания префиксов, объявленные узлом разметки
					vector <namespace_t> _bindings;
				private:
					// Вложенные узлы в порядке их следования
					vector <Value> _items;
				private:
					/**
					 * \~russian
					 * @brief Запись отображения имён: первый узел с именем и число одноимённых
					 *
					 * @details Числа одноимённых довольно, чтобы выдать ВСЕХ совпавших без
					 * перебора там, где он всего дороже: у широкого узла разных имён совпавший
					 * один, и перебор всего перечня ради него был квадратичен - 40.8 мкс на
					 * обращение при 20000 детях
					 *
					 * @note Хранить положения ВСЕХ одноимённых незачем: там, где их много, и
					 * выдача велика, и перебор платится не зря
					 *
					 * \~english
					 * @brief Record of the mapping of the names: the first node with the name and the count of the namesakes
					 *
					 * \~
					 */
					typedef struct Entry {
						// Номер первого вложенного узла разметки с этим именем
						size_t first;
						// Количество вложенных узлов разметки с этим именем
						size_t count;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param first номер первого вложенного узла с этим именем
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * @param first the number of the first nested node with this name
						 *
						 * \~
						 */
						Entry(const size_t first = 0) noexcept : first(first), count(1) {}
					} entry_t;
				private:
					/**
					 * \~russian
					 * Отображение местных имён вложенных узлов на их номера, заводимое по требованию
					 *
					 * @details Узел о немногих детях разыскивается перебором, широкий -
					 * отображением. Хранится ПЕРВОЕ вхождение имени: повтор имени у соседних
					 * узлов - обычный вид разметки, а розыск отвечает первым из них
					 *
					 * @note Отображение служит розыску по ОДНОМУ ЛИШЬ местному имени -
					 *       обращению `operator []` и обходу по пути. Розыск по паре имени и
					 *       пространства имён им не пользуется: первое совпадение местного
					 *       имени может оказаться узлом чужого пространства, и отображение
					 *       отвечало бы не тем узлом
					 *
					 * @warning Всякая правка перечня вложенных узлов ОБЯЗАНА сбрасывать
					 *          отображение: забытый сброс даёт не медленный ответ, а неверный -
					 *          либо вовсе останов работы. Номер из устаревшего отображения
					 *          уходит за конец перечня, `at()` бросает исключение изнутри
					 *          `noexcept`, и приложение прекращается. Проверено 22.08.2026
					 *          нарочно опустошённым сбросом: `CodecXmlValue.WideNodeMutators`
					 *          валит стенд именно так
					 *
					 * @warning Отдача наружу ИЗМЕНЯЕМОЙ ссылки на УЖЕ ЗАВЕДЁННЫЙ вложенный узел
					 *          сбрасывает отображение, и это не перестраховка. Имя узла
					 *          разметки хранится у САМОГО УЗЛА, а не у родителя, и получивший
					 *          ссылку вправе присвоить узлу иное значение целиком - вместе с
					 *          иным именем. Правку эту родитель не наблюдает никак. Мест таких
					 *          ЧЕТЫРЕ: обращение по имени, обращение по номеру в пределах
					 *          перечня, обращение по номеру за его пределом и обход по пути.
					 *          Найдено щупом 22.08.2026: отображение отвечало о снесённом
					 *          имени, что оно есть, а о новом - что его нет, и `contains`
					 *          расходился при этом с обращением по имени, отвечая об ОДНОМ
					 *          узле противоположно
					 *
					 * @note Кодек JSON этой беды не знает вовсе, и оттого сличение с ним здесь
					 *       обманчиво: имя поля лежит там у РОДИТЕЛЯ, отдельным перечнем, и
					 *       присваивание значения полю имени его не трогает
					 *
					 * @note Заведение узла НОВОГО сброса не требует и идёт доливом: имя его
					 *       родителю известно в тот же миг
					 *
					 * @note Сличения номера с длиной перечня здесь намеренно НЕТ: оно обратило
					 *       бы забытый сброс в тихую самопочинку, платя сличением на всяком
					 *       розыске, и скрыло бы дефект вместо того, чтобы его показать.
					 *       Взамен всякий путь правки закреплён проверкой набора
					 *
					 * \~english
					 * Mapping of the local names of the nested nodes onto their numbers, built on demand
					 *
					 * \~
					 */
					mutable unordered_map <string, entry_t> _index;
				private:
					/**
					 * \~russian
					 * Признак заведённости отображения имён вложенных узлов
					 *
					 * @note Признак отдельный от пустоты отображения обязателен: у узла об
					 *       одних лишь узлах текста отображение выходит пустым, и сличение
					 *       размеров велело бы заводить его снова на всяком обращении
					 *
					 * \~english
					 * Flag of the readiness of the mapping of the names of the nested nodes
					 *
					 * \~
					 */
					mutable bool _indexed;
				private:
					/**
					 * \~russian
					 * @brief Метод розыска номера вложенного узла разметки по местному имени
					 *
					 * @param  local разыскиваемое местное имя вложенного узла
					 * @return       номер разысканного узла либо признак отсутствия
					 *
					 * \~english
					 * @brief Method of the search of the number of a nested markup node by local name
					 *
					 * @param  local the searched local name of a nested node
					 * @return       the number of the found node or the flag of absence
					 *
					 * \~
					 */
					size_t lookup(const string & local) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса отображения имён вложенных узлов
					 *
					 * @note Сброс обязателен при ВСЯКОЙ правке перечня вложенных узлов, а не
					 *       при одном лишь снятии: заведение узла с новым именем оставило бы
					 *       отображение отвечающим об его отсутствии
					 *
					 * \~english
					 * @brief Method of the reset of the mapping of the names of the nested nodes
					 *
					 * \~
					 */
					void reindex() const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия вложенного узла разметки с правкой отображения имён
					 *
					 * @details Снятие узла ПОСЛЕДНЕГО номеров прочих узлов не двигает, и сноса
					 * отображения не требует: довольно изъять из него одну запись. Снос обошёлся
					 * бы разбором всего отображения, тогда как сдвигать в перечне при том нечего
					 *
					 * @note Замерено 23.08.2026: снос отображения стоил 132 мкс при 16384 узлах,
					 *       тогда как само изъятие из перечня - 15 нс. Разряд сложности снятия
					 *       последнего сносом обращался из постоянного в линейный
					 * @warning Снятие из СЕРЕДИНЫ перечня правкой не обходится: оно сдвигает
					 *          номера всех узлов, следующих за снятым. Не обходится и снятие
					 *          первого вхождения имени, у какого есть одноимённые: место
					 *          следующего из них отображению неизвестно
					 *
					 * @param index номер снимаемого вложенного узла
					 *
					 * \~english
					 * @brief Method of the removal of a nested markup node with the update of the mapping of names
					 *
					 * @param index the number of the removed nested node
					 *
					 * \~
					 */
					void detached(const size_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта заведённого вложенного узла разметки в отображении имён
					 *
					 * @details Долив отображения обязателен там, где узел заводится В КОНЕЦ
					 * перечня: сброс вместо долива обращает заведение узлов подряд в
					 * перестроение отображения на всякой вставке, и плата на одну вставку
					 * растёт с числом уже заведённых
					 *
					 * @note Метод зовётся ПОСЛЕ добавления узла в перечень: номером служит
					 *       место, какое узел в перечне занял
					 *
					 * @param local местное имя заведённого вложенного узла
					 *
					 * \~english
					 * @brief Method of the accounting of an added nested markup node in the mapping of names
					 *
					 * @param local the local name of the added nested node
					 *
					 * \~
					 */
					void indexed(const string & local) const noexcept;
					/**
					 * \~russian
					 * @brief Метод подсчёта корневых узлов разметки, значением вносимых
					 *
					 * @details Запись узел вида документа РАЗВОРАЧИВАЕТ, выдавая одно лишь
					 * содержимое его: документ, вложенный в разметку, записывается верно, а
					 * вот число корней он вносит своё. Подсчёт потому и ведётся возвратом
					 *
					 * @return количество корневых узлов разметки
					 *
					 * \~english
					 * @brief Method of the counting of the root markup nodes brought in by a value
					 * @details The writing UNWRAPS a node of the kind of a document, issuing only its
					 * content: a document nested into a markup is written correctly, while the number
					 * of the roots it brings in is its own. Therefore the counting is performed by a recursion
					 * @return number of the root markup nodes
					 * \~
					 */
					uint32_t roots() const noexcept;
				private:
					/**
					 * \~russian
					 * Отображение имён свойств узла на их номера, заводимое по требованию
					 *
					 * @details Ключ двухуровневый - местное имя, а под ним обозначение пространства
					 * имён, - и составного ключа отсюда не собирается нигде. Ключ, склеенный из
					 * двух имён, требовал бы выделения памяти на ВСЯКОМ розыске, тогда как
					 * розыск обязан обходиться без выделений вовсе
					 *
					 * @note Отображение это отвечает ТОЧНО, в отличие от отображения вложенных
					 *       узлов: сличение свойств идёт по паре имени и пространства имён, и
					 *       двух свойств с одинаковой парой у узла не бывает - установка
					 *       перезаписывает такое свойство на его же месте. Оттого отката к
					 *       перебору здесь нет ни на одном пути
					 *
					 * \~english
					 * Mapping of the names of the node properties onto their numbers, built on demand
					 *
					 * \~
					 */
					mutable unordered_map <string, unordered_map <string, size_t>> _properties;
				private:
					/**
					 * \~russian
					 * Признак заведённости отображения имён свойств узла
					 *
					 * @note Признак отдельный от пустоты отображения обязателен по той же причине,
					 *       что и у отображения вложенных узлов: у узла без свойств отображение
					 *       выходит пустым, и сличение размеров велело бы заводить его снова
					 *       на всяком обращении
					 *
					 * \~english
					 * Flag of the readiness of the mapping of the names of the node properties
					 *
					 * \~
					 */
					mutable bool _propertied;
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
					/**
					 * \~russian
					 * @brief Код отказа последней работы над значением
					 *
					 * @details Поле изменчиво нарочно: выдача текста значения идёт работой
					 * неизменной, а сообщать отказ обязана - иначе пустой текст неотличим
					 * от значения пустого
					 *
					 * \~english
					 * @brief Code of the refusal of the last operation over the value
					 * @details The field is mutable deliberately: the issuance of the text of a value
					 * is a constant operation, while it is obliged to report a refusal — otherwise
					 * an empty text is indistinguishable from an empty value
					 * \~
					 */
					mutable error_t _error = error_t::NONE;
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * @brief Метод розыска номера свойства узла по имени и пространству имён
					 *
					 * @param  local местное имя разыскиваемого свойства
					 * @param  uri   обозначение пространства имён свойства
					 * @return       номер разысканного свойства либо признак отсутствия
					 *
					 * \~english
					 * @brief Method of the search of the number of a node property by name and namespace
					 *
					 * @param  local the local name of the searched property
					 * @param  uri   the namespace designation of the property
					 * @return       the number of the found property or the flag of absence
					 *
					 * \~
					 */
					size_t property(const string & local, const string & uri) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса отображения имён свойств узла
					 *
					 * @note Сброс обязателен при всякой правке перечня свойств, КРОМЕ заведения
					 *       свойства в конец: там отображение доливается, а не сбрасывается
					 *
					 * \~english
					 * @brief Method of the reset of the mapping of the names of the node properties
					 *
					 * \~
					 */
					void reproperty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта заведённого свойства узла в отображении имён
					 *
					 * @note Метод зовётся ПОСЛЕ добавления свойства в перечень: номером служит
					 *       место, какое свойство в перечне заняло
					 *
					 * @param local местное имя заведённого свойства
					 * @param uri   обозначение пространства имён заведённого свойства
					 *
					 * \~english
					 * @brief Method of the accounting of an added node property in the mapping of names
					 *
					 * @param local the local name of the added property
					 * @param uri   the namespace designation of the added property
					 *
					 * \~
					 */
					void propertied(const string & local, const string & uri) const noexcept;
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
					 * отказом: продолжение записи после отказа оставило бы в тексте узел
					 * открытым либо содержимое его недописанным
					 *
					 * @param writer поток записи, куда ложится значение
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a writing stream
					 * @details A refusal of the writing stream is handled at once and is passed upwards
					 * as a refusal: a continuation of the writing after a refusal would leave in the text a node
					 * open or its content unfinished
					 * @param writer writing stream where the value is placed
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool compose(writer_t & writer, const bool preserve = false) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод снятия значения с узла дерева разметки
					 *
					 * @details Дерево глубже предела `MAX_DEPTH` не снимается вовсе: снятие
					 * возвратно, и глубина дерева ложится прямо на стек вызовов - замер дал
					 * срыв стека около 20 000 уровней. Дерево, снятию не поддавшееся,
					 * оставляет значение неопределённым, как оставляет его и узел
					 * недействительный, а причина уходит в журнал
					 *
					 * @param node  узел дерева разметки
					 * @param depth глубина вложенности снимаемого узла
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the taking of a value from a node of a markup tree
					 * @details A tree deeper than the limit `MAX_DEPTH` is not taken at all: the taking
					 * is recursive, and the depth of the tree falls directly upon the stack of the calls — a measurement gave
					 * an overflow of the stack at about 20 000 levels. A tree that has not yielded to the taking
					 * leaves the value undefined, just as an invalid node leaves it, while the reason goes to the log
					 * @param node  node of the markup tree
					 * @param depth depth of the nesting of the node being taken
					 *
					 * \~
					 */
					bool absorb(const node_t & node, const uint32_t depth = 0) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сбора содержимого вложенных текстовых узлов
					 *
					 * @param result строка, куда собирается содержимое
					 *
					 * \~english
					 * @brief Method of the gathering of the content of the nested text nodes
					 * @param result string where the content is gathered
					 *
					 * \~
					 */
					void gather(string & result) const noexcept;
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
					 * изменяемом. Записанное в него пропадает при следующем же неудачном
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
					 * access. What is written into it disappears at the very next unsuccessful
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
					 * @brief Метод получения кода отказа последней работы над значением
					 *
					 * @details Код этот отвечает на вопрос, отчего работа не сбылась: разбор
					 * текста, чтение файла, выдача текста и запись его в файл ставят его
					 * заново каждая. Прежде канала отказа у значения не было вовсе, и пустой
					 * текст выдачи был неотличим от значения пустого: замер дал одиннадцать
					 * способов завести значение, записи не подлежащее, и все они молчали
					 *
					 * @note Работы, отказать не могущие, кода не трогают: снятие вида, имени
					 *       и содержимого идёт мимо этого канала
					 *
					 * @return код отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the code of the refusal of the last operation over the value
					 * @details This code answers the question of why an operation did not come true: the parsing
					 * of a text, the reading of a file, the issuance of a text and the writing of it into a file
					 * each set it anew. Formerly a value had no channel of a refusal at all, and the empty text
					 * of the issuance was indistinguishable from an empty value: a measurement gave eleven ways
					 * to obtain a value not subject to the writing, and all of them kept silent
					 * @note The operations that cannot refuse do not touch the code: the taking of the kind,
					 *       of the name and of the content goes past this channel
					 * @return code of the refusal of the last operation
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида узла
					 *
					 * @return вид хранимого узла
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the node
					 * @return kind of the stored node
					 *
					 * \~
					 */
					kind_t kind() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки вида узла
					 *
					 * @param kind сличаемый вид узла
					 * @return     признак совпадения вида
					 *
					 * \~english
					 * @brief Method of the check of the kind of the node
					 * @param kind kind of the node being compared
					 * @return sign of the coincidence of the kind
					 *
					 * \~
					 */
					bool is(const kind_t kind) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения количества вложенных узлов
					 *
					 * @return количество вложенных узлов
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the nested nodes
					 * @return number of the nested nodes
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки узла на отсутствие вложенных узлов
					 *
					 * @return признак отсутствия вложенных узлов
					 *
					 * \~english
					 * @brief Method of the check of the node for the absence of the nested nodes
					 * @return sign of the absence of the nested nodes
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
					 * @brief Метод извлечения имени узла
					 *
					 * @note Отрезки выданного имени указывают на память самого значения и живут
					 *       ровно столько, сколько живёт оно само
					 *
					 * @return имя узла с учётом пространства имён
					 *
					 * \~english
					 * @brief Method of the extraction of the name of the node
					 * @note The segments of the issued name point to the memory of the value itself and live
					 *       exactly as long as it lives itself
					 * @return name of the node with regard to the namespace
					 *
					 * \~
					 */
					name_t name() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения местного имени узла
					 *
					 * @return местное имя узла, а у указания обработчику - цель его
					 *
					 * \~english
					 * @brief Method of the extraction of the local name of the node
					 * @return local name of the node, and for a processing instruction — its target
					 *
					 * \~
					 */
					const string & local() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения обозначения пространства имён узла
					 *
					 * @return обозначение пространства имён узла, пустое - имя без пространства
					 *
					 * \~english
					 * @brief Method of the extraction of the designation of the namespace of the node
					 * @return designation of the namespace of the node, an empty one — a name without a namespace
					 *
					 * \~
					 */
					const string & uri() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения префикса пространства имён узла
					 *
					 * @return префикс пространства имён, пустой - префикса нет
					 *
					 * \~english
					 * @brief Method of the extraction of the prefix of the namespace of the node
					 * @return prefix of the namespace, an empty one — there is no prefix
					 *
					 * \~
					 */
					const string & prefix() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки имени узла
					 *
					 * @param local  устанавливаемое местное имя узла
					 * @param uri    устанавливаемое обозначение пространства имён
					 * @param prefix устанавливаемый префикс пространства имён
					 *
					 * \~english
					 * @brief Method of the setting of the name of the node
					 * @param local local name of the node being set
					 * @param uri designation of the namespace being set
					 * @param prefix prefix of the namespace being set
					 *
					 * \~
					 */
					void name(const string & local, const string & uri = "", const string & prefix = "") noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого узла
					 *
					 * @details У узла текстового, дословного раздела, примечания, указания
					 * обработчику и описания типа документа выдаётся собственное содержимое их.
					 * У узла разметки - содержимое всех вложенных в него текстовых узлов
					 * подряд, ровно как это делает узел дерева
					 *
					 * @return содержимое узла
					 *
					 * \~english
					 * @brief Method of the extraction of the content of the node
					 * @details For a text node, a literal section, a comment, a processing
					 * instruction and a description of the type of the document their own content is issued.
					 * For a markup node — the content of all the text nodes nested into it
					 * in a row, exactly as a node of the tree does it
					 * @return content of the node
					 *
					 * \~
					 */
					string text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки собственного содержимого узла
					 *
					 * @details У узла разметки установка содержимого заменяет все вложенные узлы
					 * одним узлом текстовым: содержимого своего у разметки нет вовсе
					 *
					 * @param text устанавливаемое содержимое узла
					 * @return     признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of the own content of the node
					 * @details For a markup node the setting of the content replaces all the nested nodes
					 * by one text node: the markup has no own content at all
					 * @param text content of the node being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool text(const string & text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения свойств узла разметки
					 *
					 * @return перечень свойств узла в порядке их следования
					 *
					 * \~english
					 * @brief Method of the extraction of the properties of a markup node
					 * @return list of the properties of the node in the order of their following
					 *
					 * \~
					 */
					const vector <property_t> & attributes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения свойства узла разметки
					 *
					 * @param local местное имя разыскиваемого свойства
					 * @param uri   обозначение пространства имён свойства
					 * @return      значение свойства, пустое - свойства такого нет
					 *
					 * \~english
					 * @brief Method of the extraction of the value of a property of a markup node
					 * @param local local name of the property being searched for
					 * @param uri designation of the namespace of the property
					 * @return value of the property, an empty one — there is no such property
					 *
					 * \~
					 */
					const string & attribute(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки свойства узла разметки
					 *
					 * @details Свойство с уже занятым именем перезаписывается на своём месте, а
					 * порядок свойств при том сохраняется
					 *
					 * @param local  местное имя свойства
					 * @param value  устанавливаемое значение свойства
					 * @param uri    обозначение пространства имён свойства
					 * @param prefix префикс пространства имён свойства
					 * @return       признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a property of a markup node
					 * @details A property with an already occupied name is overwritten in its place, while
					 * the order of the properties is thereby preserved
					 * @param local local name of the property
					 * @param value value of the property being set
					 * @param uri designation of the namespace of the property
					 * @param prefix prefix of the namespace of the property
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool attribute(const string & local, const string & value, const string & uri = "", const string & prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия свойства у узла разметки
					 *
					 * @param local местное имя разыскиваемого свойства
					 * @param uri   обозначение пространства имён свойства
					 * @return      признак наличия свойства
					 *
					 * \~english
					 * @brief Method of the check of the presence of a property of a markup node
					 * @param local local name of the property being searched for
					 * @param uri designation of the namespace of the property
					 * @return sign of the presence of the property
					 *
					 * \~
					 */
					bool has(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия свойства узла разметки
					 *
					 * @param local местное имя снимаемого свойства
					 * @param uri   обозначение пространства имён свойства
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a property of a markup node
					 * @param local local name of the property being removed
					 * @param uri designation of the namespace of the property
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					bool detach(const string & local, const string & uri = "") noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения связываний префиксов, объявленных узлом
					 *
					 * @return перечень связываний префиксов
					 *
					 * \~english
					 * @brief Method of the extraction of the bindings of the prefixes declared by the node
					 * @return list of the bindings of the prefixes
					 *
					 * \~
					 */
					const vector <namespace_t> & bindings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления связывания префикса с пространством имён
					 *
					 * @param prefix объявляемый префикс, пустой - объявление по умолчанию
					 * @param uri    обозначение пространства имён, пустое - отмена связывания
					 * @return       признак успешности объявления
					 *
					 * \~english
					 * @brief Method of the declaration of a binding of a prefix to a namespace
					 * @param prefix prefix being declared, an empty one — a declaration by default
					 * @param uri designation of the namespace, an empty one — a cancellation of the binding
					 * @return sign of the success of the declaration
					 *
					 * \~
					 */
					bool binding(const string & prefix, const string & uri) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки наличия вложенного узла разметки с указанным именем
					 *
					 * @param local местное имя разыскиваемого узла
					 * @param uri   обозначение пространства имён узла
					 * @return      признак наличия вложенного узла
					 *
					 * \~english
					 * @brief Method of the check of the presence of a nested markup node with the indicated name
					 * @param local local name of the node being searched for
					 * @param uri designation of the namespace of the node
					 * @return sign of the presence of the nested node
					 *
					 * \~
					 */
					bool contains(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения всех вложенных узлов разметки с указанным именем
					 *
					 * @param local местное имя разыскиваемых узлов
					 * @param uri   обозначение пространства имён узлов
					 * @return      перечень указаний на разысканные узлы
					 *
					 * \~english
					 * @brief Method of the extraction of all the nested markup nodes with the indicated name
					 * @param local local name of the nodes being searched for
					 * @param uri designation of the namespace of the nodes
					 * @return list of the pointers to the found nodes
					 *
					 * \~
					 */
					vector <const Value *> children(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к узлу по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой:
					 * `/Envelope/Body/0`. Часть числовая обращается к вложенному узлу по
					 * номеру, а прочие - к узлу разметки по местному имени. Обращение к
					 * отсутствующему пути ничего не заводит и отдаёт значение неопределённое
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param path путь к разыскиваемому узлу
					 * @return     ссылка на разысканный узел
					 *
					 * \~english
					 * @brief Method of the access to a node by a path
					 * @details The path is written by the parts separated by a slash:
					 * `/Envelope/Body/0`. A numeric part accesses a nested node by an
					 * index, while the others access a markup node by a local name. An access to
					 * an absent path creates nothing and gives away an undefined value
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param path path to the node being searched for
					 * @return reference to the found node
					 *
					 * \~
					 */
					const Value & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к узлу по пути с заведением недостающего
					 *
					 * @details Недостающие звенья пути заводятся узлами разметки с местным
					 * именем звена. Звено числовое недостающего не заводит и отвечает
					 * значением мусорным: имени у заводимого узла взять неоткуда
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param path путь к разыскиваемому узлу
					 * @return     ссылка на разысканный либо заведённый узел
					 *
					 * \~english
					 * @brief Method of the access to a node by a path with the creation of the missing ones
					 * @details The missing links of the path are created as markup nodes with the local
					 * name of the link. A numeric link does not create a missing one and answers
					 * with a scrap value: there is nowhere to take the name of the node being created from
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param path path to the node being searched for
					 * @return reference to the found or created node
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод обращения к вложенному узлу разметки по местному имени
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param local местное имя вложенного узла
					 * @return      ссылка на первый вложенный узел с таким именем
					 *
					 * \~english
					 * @brief Method of the access to a nested markup node by a local name
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param local local name of the nested node
					 * @return reference to the first nested node with such name
					 *
					 * \~
					 */
					const Value & operator [] (const string & local) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к вложенному узлу разметки по местному имени с
					 *        заведением недостающего
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param local местное имя вложенного узла
					 * @return      ссылка на первый вложенный узел с таким именем
					 *
					 * \~english
					 * @brief Method of the access to a nested markup node by a local name with
					 *        the creation of a missing one
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param local local name of the nested node
					 * @return reference to the first nested node with such name
					 *
					 * \~
					 */
					Value & operator [] (const string & local) noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к вложенному узлу по номеру
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param index номер вложенного узла
					 * @return      ссылка на вложенный узел
					 *
					 * \~english
					 * @brief Method of the access to a nested node by an index
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param index index of the nested node
					 * @return reference to the nested node
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к вложенному узлу по номеру с заведением недостающего
					 *
					 * @details Обращение за границу перечня растит его до затребованного номера
					 * узлами неопределёнными
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 * дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 * прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 * к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 * ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param index номер вложенного узла
					 * @return      ссылка на вложенный узел
					 *
					 * \~english
					 * @brief Method of the access to a nested node by an index with the creation of a missing one
					 * @details An access beyond the boundary of the list grows it up to the demanded index
					 * by the undefined nodes
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 * the children lie in a movable container, and its growth INVALIDATES the references to
					 * the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 * to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 * the reference but the path to the node, and it should be taken anew
					 * @param index index of the nested node
					 * @return reference to the nested node
					 *
					 * \~
					 */
					Value & operator [] (const size_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения по написанию знака
					 *
					 * @details Написание знака есть ТЕКСТ, и обращение по нему разыскивает
					 * ИМЯ из одного знака, а не номер. Правило это одно на весь кодек
					 *
					 * @note Без этого метода `v['z']` молча обращался к элементу под номером
					 *       122 и отдавал пустоту, тогда как `v["z"]` отдавал значение.
					 *       Перечнем беда не лечится: заводитель по разряду `char` делает
					 *       двусмысленным обычное `v[0]`, оттого здесь шаблон с оградой
					 *
					 * @tparam T разряд обращения, ограда пропускает ровно знак
					 * @param  name разыскиваемое имя из одного знака
					 * @return ссылка на дочерний узел
					 *
					 * \~english
					 * @brief Method of the access by a writing of a character
					 * @details A writing of a character is a TEXT, and an access by it searches for a NAME
					 * of a single character rather than for an index. This rule is one for the whole codec
					 * @note Without this method `v['z']` silently addressed the element under the index 122
					 *       and gave away an emptiness, while `v["z"]` gave away a value. By a list the
					 *       trouble is not cured: a creator by the `char` width makes the ordinary `v[0]`
					 *       ambiguous, hence a template with a guard here
					 * @tparam T width of the access, the guard admits exactly a character
					 * @param name name of a single character being searched for
					 * @return reference to the child node
					 *
					 * \~
					 */
					template <typename T>
					typename std::enable_if <std::is_same <T, char>::value, const Value &>::type
					operator [] (const T name) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения по написанию знака
					 *
					 * @details Написание знака есть ТЕКСТ, и обращение по нему разыскивает
					 * ИМЯ из одного знака, а не номер. Правило это одно на весь кодек
					 *
					 * @note Без этого метода `v['z']` молча обращался к элементу под номером
					 *       122 и отдавал пустоту, тогда как `v["z"]` отдавал значение.
					 *       Перечнем беда не лечится: заводитель по разряду `char` делает
					 *       двусмысленным обычное `v[0]`, оттого здесь шаблон с оградой
					 *
					 * @tparam T разряд обращения, ограда пропускает ровно знак
					 * @param  name разыскиваемое имя из одного знака
					 * @return ссылка на дочерний узел
					 *
					 * \~english
					 * @brief Method of the access by a writing of a character
					 * @details A writing of a character is a TEXT, and an access by it searches for a NAME
					 * of a single character rather than for an index. This rule is one for the whole codec
					 * @note Without this method `v['z']` silently addressed the element under the index 122
					 *       and gave away an emptiness, while `v["z"]` gave away a value. By a list the
					 *       trouble is not cured: a creator by the `char` width makes the ordinary `v[0]`
					 *       ambiguous, hence a template with a guard here
					 * @tparam T width of the access, the guard admits exactly a character
					 * @param name name of a single character being searched for
					 * @return reference to the child node
					 *
					 * \~
					 */
					template <typename T>
					typename std::enable_if <std::is_same <T, char>::value, Value &>::type
					operator [] (const T name) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод добавления узла в конец перечня вложенных
					 *
					 * @details Значение неопределённое перерождается узлом разметки, а узел
					 * текстовый вложенных узлов не имеет вовсе и добавление отвечает отказом
					 *
					 * @param value добавляемый узел
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a node to the end of the list of the nested ones
					 * @details An undefined value is reborn as a markup node, while a text node
					 * has no nested nodes at all and the addition responds with a refusal
					 * @param value node being added
					 * @return sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки вложенного узла разметки по местному имени
					 *
					 * @details Узел с уже занятым именем перезаписывается на своём месте, а
					 * порядок вложенных узлов при том сохраняется. Имя узла устанавливается
					 * заданным именем, каким бы имя подаваемого узла ни было
					 *
					 * @param local местное имя вложенного узла
					 * @param value устанавливаемый узел
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a nested markup node by a local name
					 * @details A node with an already occupied name is overwritten in its place, while
					 * the order of the nested nodes is thereby preserved. The name of the node is set
					 * by the given name, whatever the name of the node being passed may be
					 * @param local local name of the nested node
					 * @param value node being set
					 * @return sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & local, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия вложенного узла разметки по местному имени
					 *
					 * @details Снимается первый вложенный узел с таким именем, а не все они:
					 * узлов с одним именем у разметки бывает сколько угодно
					 *
					 * @param local местное имя снимаемого узла
					 * @param uri   обозначение пространства имён узла
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a nested markup node by a local name
					 * @details The first nested node with such name is removed rather than all of them:
					 * a markup may have any number of the nodes with one name
					 * @param local local name of the node being removed
					 * @param uri designation of the namespace of the node
					 * @return sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const string & local, const string & uri = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия вложенного узла по номеру
					 *
					 * @param index номер снимаемого узла
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a nested node by an index
					 * @param index index of the node being removed
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
					 * @details Истиной считаются записи `true` и `1`, ложью - `false` и `0`:
					 * иных записей логического значения описания разметки не знают
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a logical value
					 * @details The records `true` and `1` are considered a truth, `false` and `0` a falsehood:
					 * the descriptions of the markup know no other records of a logical value
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
					 * @details Число разбирается из содержимого узла: своих видов у разметки нет
					 * вовсе, всякое содержимое её есть текст. Отказом извлечение завершается
					 * лишь тогда, когда содержимое числом не является
					 *
					 * @note Договор извлечения общий у всех кодеков рамки: целое сужается
					 *       обычным приведением языка, дробное ОКРУГЛЯЕТСЯ до целого с уводом
					 *       половины от нуля (`1.5` даёт `2`, `-1.5` даёт `-2`), дробное за
					 *       пределом затребованного целого вида выдаётся пределом этого вида,
					 *       а `NaN` - нулём
					 *
					 * @note Дробного с НУЛЕВОЙ дробной частью правило это не касается: `300.0`
					 *       извлекается ровно как `300` и заворачивается по кругу. Запись числа
					 *       ответа менять не должна - решение владельца от 30.08.2026; предел
					 *       вида остаётся лишь там, где дробная часть есть на деле. Совпадение
					 *       записей держится до 2^53: выше `double` целых точно не представляет,
					 *       и `9007199254740993.0` теряет число ещё у разбора
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a number
					 * @details A number is parsed from the content of the node: the markup has no kinds of its own
					 * at all, all its content is a text. The extraction ends with a refusal
					 * only when the content is not a number
					 * @note The contract of the extraction is common for all the codecs of the framework: the narrowing is performed
					 *       by the ordinary casting of the language, a fractional number beyond the limit of the demanded
					 *       integer kind is issued as the limit of that kind, while `NaN` — as a zero
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(float & result) const noexcept;
					/**
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строкового значения
					 *
					 * @details Извлечение это отдаёт содержимое узла ВСЕГДА, каким бы оно ни
					 * было: своих видов у разметки нет вовсе, всякое значение здесь есть текст,
					 * и отказывать по виду попросту нечему. Узел с записью `1` отвечает истиной
					 * и выдаёт `1`, хотя извлечение логическим и числовым тот же узел разбирают
					 *
					 * @note Кодеки JSON и TOML на том же месте отвечают ОТКАЗОМ, и это НЕ
					 *       расхождение недосмотром: виды у них есть, и извлечение строкою
					 *       служит им проверкою вида, а запись выдаётся отдельным способом.
					 *       Правило следует из устройства наречия, а не из вкуса кодека
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a string value
					 * @details This extraction issues the content of the node ALWAYS, whatever it may
					 * be: the markup has no kinds of its own at all, every value here is a text,
					 * and there is simply nothing to refuse by the kind. A node with the record `1` answers with a truth
					 * and issues `1`, although the extraction by a logical and by a numeric parse the same node
					 * @note The codecs JSON and TOML answer with a REFUSAL at the same place, and this is NOT
					 *       a divergence by an oversight: they have the kinds, and the extraction by a string
					 *       serves them as a check of the kind, while the record is issued by a separate means.
					 *       The rule follows from the structure of the notation rather than from the taste of the codec
					 * @param result variable where the extracted value is placed
					 * @return sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки во владеющее значение
					 *
					 * @details Разбор ведётся тем же деревом, каким он ведётся всегда: обёртка
					 * лишь снимает с него поддерево собственной памятью
					 *
					 * @note Настройки чтения принимаются доводом, как и у дерева: без него ни
					 * одна из них - ни разрешение префиксов, ни строгость к внешним сущностям,
					 * ни подстановка объявленных умолчаний - у владеющего значения недостижима
					 * вовсе, и разбор обрывка вёлся бы одними умолчаниями
					 *
					 * @param text     разбираемый текст разметки
					 * @param settings настройки чтения текста разметки
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a markup text into an owning value
					 * @details The parsing is conducted by the same tree by which it is always conducted: the wrapper
					 * only takes a subtree off it by its own memory
					 * @note The settings of the reading are taken as an argument, as with a tree: without it not
					 * a single one of them — neither the resolution of the prefixes, nor the strictness towards
					 * the external entities, nor the substitution of the declared defaults — would be reachable
					 * at all for an owning value, and the parsing of a fragment would be conducted by the defaults alone
					 * @param text markup text being parsed
					 * @param settings settings of the reading of a markup text
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const reader_t::settings_t & settings = reader_t::settings_t()) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки из файла
					 *
					 * @note Файл поднимается в память целиком, а не читается кусками: отвод
					 *       места под дерево снимается пред-разбором всего текста, и потоковым
					 *       такой разбор быть не может. Тем настоящий метод и отличается от
					 *       одноимённого у кодека JSON, читающего файл кусками. Файлу, в память
					 *       не помещающемуся, предназначено потоковое чтение reader_t
					 *
					 * @param filename адрес разбираемого файла
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a markup text from a file
					 * @note The file is raised into the memory in full rather than read by chunks: the
					 * reservation of the room for the tree is taken by a pre-scan of the whole text, and
					 * such a parsing cannot be a streaming one. That is what tells the present method from
					 * the same-named one of the json codec, which reads a file by chunks. A file that does
					 * not fit into the memory is what the streaming reading of reader_t is meant for
					 * @param filename address of the file being parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст разметки
					 *
					 * @param format вид записи собираемого текста
					 * @return       текст разметки, пустой - записать значение не удалось
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a markup text
					 * @param format kind of the writing of the text being assembled
					 * @return markup text, an empty one — the value could not be written
					 *
					 * \~
					 */
					string dump(const format_t format = format_t::COMPACT) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст разметки с указанными настройками
					 *
					 * @details Отказ записи хотя бы одного узла отдаёт текст пустым: текст
					 * усечённый, с узлом открытым и незакрытым, негоден вовсе, а выдавать
					 * негодное молча кодек не вправе
					 *
					 * @param settings настройки записи текста
					 * @return         текст разметки, пустой - записать значение не удалось
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a markup text with the indicated settings
					 * @details A refusal of the writing of at least one node gives away an empty text: a truncated
					 * text with a node opened and unclosed is unusable at all, while the codec has no right
					 * to give away an unusable thing silently
					 * @param settings settings of the writing of the text
					 * @return markup text, an empty one — the value could not be written
					 *
					 * \~
					 */
					string dump(const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения в файл
					 *
					 * @param filename адрес записываемого файла
					 * @param format   вид записи собираемого текста
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a file
					 * @param filename address of the file being written
					 * @param format kind of the writing of the text being assembled
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
					 * @details Сличаются вид узла, имя его с пространством имён, содержимое,
					 * свойства и вложенные узлы. Префикс пространства имён и связывания
					 * сличению не подлежат: одно и то же имя, записанное разными префиксами,
					 * есть одно и то же имя
					 *
					 * @note Порядок свойств сличению не подлежит, а порядок вложенных узлов -
					 *       подлежит: свойства узла суть набор, а содержимое разметки
					 *       определено порядком своим
					 *
					 * @warning Равенство значений НЕ означает совпадения их записей. Замеры при
					 * `==`, отвечающем истиной: свойства пишутся в своём порядке у каждого, а
					 * префиксы пространств имён сохраняются каждый свой. Сличение отвечает о
					 * ЗНАЧЕНИИ, а запись хранит ОФОРМЛЕНИЕ, и решать по сличению, будут ли
					 * тексты одинаковы, нельзя. Так же поступает и сличение значения JSON
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Method of the comparison of the values
					 * @details The kind of the node, its name with the namespace, the content,
					 * the properties and the nested nodes are compared. The prefix of the namespace and the bindings
					 * are not subject to the comparison: one and the same name written by different prefixes
					 * is one and the same name
					 * @note The order of the properties is not subject to the comparison, while the order of the nested nodes
					 *       is: the properties of a node are a set, while the content of a markup
					 *       is defined by its order
					 * @warning The equality of the values does NOT mean the coincidence of their records. Measurements with
					 * `==` answering true: the attributes are written in their own order for each, while
					 * the prefixes of the namespaces are preserved each its own. The comparison answers about the
					 * VALUE, while the writing keeps the FORM, and it is not permissible to decide by the comparison whether the
					 * texts will be identical. The comparison of a JSON value acts the same way
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
					 * @brief Оператор присваивания копией
					 *
					 * @param value присваиваемое значение
					 * @return      ссылка на текущее значение
					 *
					 * \~english
					 * @brief Operator of the assignment by copy
					 *
					 * @param value the assigned value
					 * @return      the reference to the current value
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
					 * @brief Конструктор узла указанного вида
					 *
					 * @param kind вид заводимого узла
					 *
					 * \~english
					 * @brief Constructor of a node of the indicated kind
					 * @param kind kind of the node being created
					 *
					 * \~
					 */
					explicit Value(const kind_t kind) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор узла разметки с именем
					 *
					 * @param local  местное имя заводимого узла
					 * @param uri    обозначение пространства имён узла
					 * @param prefix префикс пространства имён узла
					 *
					 * \~english
					 * @brief Constructor of a markup node with a name
					 * @param local local name of the node being created
					 * @param uri designation of the namespace of the node
					 * @param prefix prefix of the namespace of the node
					 *
					 * \~
					 */
					explicit Value(const string & local, const string & uri = "", const string & prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Конструктор узла указанного вида с содержимым
					 *
					 * @param kind вид заводимого узла
					 * @param text содержимое заводимого узла
					 *
					 * \~english
					 * @brief Constructor of a node of the indicated kind with a content
					 * @param kind kind of the node being created
					 * @param text content of the node being created
					 *
					 * \~
					 */
					Value(const kind_t kind, const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор снятия значения с узла дерева разметки
					 *
					 * @details Конструктор этот и есть мост от разбора к владению: узел дерево
					 * пережить не может, а снятое им значение - может
					 *
					 * @param node узел дерева разметки
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a node of a markup tree
					 * @details This constructor is the very bridge from the parsing to the ownership: a node
					 * cannot outlive the tree, while a value taken by it — can
					 * @param node node of the markup tree
					 *
					 * \~
					 */
					explicit Value(const node_t & node) noexcept;
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
			 * @brief Потоковая сборка владеющего значения XML
			 *
			 * @details Сборщик этот повторяет договор потока записи слово в слово: открыть
			 * узел, объявить свойство, объявить связывание, записать содержимое, закрыть
			 * узел, завершить. Оттого «пишу текстом» и «строю дерево» отличаются у
			 * потребителя одной буквой, а не двумя разными договорами
			 *
			 * \~english
			 * @brief Streaming assembly of an owning value of XML
			 * @details This builder repeats the contract of the writing stream word for word: to open
			 * a node, to declare a property, to declare a binding, to write a content, to close
			 * a node, to finish. Whereby "I write a text" and "I build a tree" differ for
			 * a consumer by one letter rather than by two different contracts
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Builder {
				private:
					// Собираемое значение
					value_t _result;
				private:
					// Стек указаний на открытые узлы разметки
					vector <value_t *> _nesting;
				private:
					/**
					 * \~russian
					 * @brief Метод помещения собранного узла на своё место
					 *
					 * @param value помещаемый узел
					 * @return      указание на помещённый узел, ноль - помещение не удалось
					 *
					 * \~english
					 * @brief Method of the placement of an assembled node at its place
					 * @param value node being placed
					 * @return pointer to the placed node, a zero — the placement has failed
					 *
					 * \~
					 */
					value_t * attach(const value_t & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия узла разметки
					 *
					 * @param local  местное имя открываемого узла
					 * @param uri    обозначение пространства имён узла
					 * @param prefix префикс пространства имён узла
					 * @return       признак успешности открытия
					 *
					 * \~english
					 * @brief Method of the opening of a markup node
					 * @param local local name of the node being opened
					 * @param uri designation of the namespace of the node
					 * @param prefix prefix of the namespace of the node
					 * @return sign of the success of the opening
					 *
					 * \~
					 */
					bool open(const string & local, const string & uri = "", const string & prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого узла разметки
					 *
					 * @return признак успешности закрытия
					 *
					 * \~english
					 * @brief Method of the closing of an opened markup node
					 * @return sign of the success of the closing
					 *
					 * \~
					 */
					bool close() noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления свойства открытого узла разметки
					 *
					 * @param local  местное имя свойства
					 * @param value  значение свойства
					 * @param uri    обозначение пространства имён свойства
					 * @param prefix префикс пространства имён свойства
					 * @return       признак успешности объявления
					 *
					 * \~english
					 * @brief Method of the declaration of a property of an opened markup node
					 * @param local local name of the property
					 * @param value value of the property
					 * @param uri designation of the namespace of the property
					 * @param prefix prefix of the namespace of the property
					 * @return sign of the success of the declaration
					 *
					 * \~
					 */
					bool attribute(const string & local, const string & value, const string & uri = "", const string & prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод объявления связывания префикса открытым узлом разметки
					 *
					 * @param prefix объявляемый префикс, пустой - объявление по умолчанию
					 * @param uri    обозначение пространства имён
					 * @return       признак успешности объявления
					 *
					 * \~english
					 * @brief Method of the declaration of a binding of a prefix by an opened markup node
					 * @param prefix prefix being declared, an empty one — a declaration by default
					 * @param uri designation of the namespace
					 * @return sign of the success of the declaration
					 *
					 * \~
					 */
					bool binding(const string & prefix, const string & uri) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи текстового содержимого
					 *
					 * @param text записываемое содержимое
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a text content
					 * @param text content being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool text(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дословного раздела
					 *
					 * @param text записываемое содержимое
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a literal section
					 * @param text content being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool cdata(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи примечания
					 *
					 * @param text записываемое содержимое
					 * @return     признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a comment
					 * @param text content being written
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool comment(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи указания обработчику
					 *
					 * @param target цель указания обработчику
					 * @param text   содержимое указания обработчику
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a processing instruction
					 * @param target target of the processing instruction
					 * @param text content of the processing instruction
					 * @return sign of the success of the writing
					 *
					 * \~
					 */
					bool processing(const string & target, const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи готового узла
					 *
					 * @details Метод этот вставляет в собираемое дерево целое поддерево,
					 * собранное где-то ещё
					 *
					 * @param value записываемый узел
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a ready node
					 * @details This method inserts into the tree being assembled a whole subtree
					 * assembled somewhere else
					 * @param value node being written
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
					 * @return количество открытых и ещё не закрытых узлов
					 *
					 * \~english
					 * @brief Method of the extraction of the current depth of the nesting
					 * @return number of the opened and not yet closed nodes
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
					 * @details Незакрытые узлы закрываются сами: сборка потоковая прервана быть
					 * не может, а отказ здесь оставил бы потребителя вовсе без итога
					 *
					 * @note Собранное отдаётся узлом разметки, коль скоро он у сборки один, и
					 *       узлом корневым, коль скоро их несколько: разметка одного корня
					 *       требует, а сборщик волен собрать и примечание перед ним
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the finishing of the assembly and of the taking away of the assembled value
					 * @details The unclosed nodes are closed by themselves: a streaming assembly cannot be
					 * interrupted, while a refusal here would leave a consumer entirely without a result
					 * @note What has been assembled is given away as a markup node if the assembly has one of them, and
					 *       as a root node if there are several: a markup requires one root, while
					 *       a builder is free to assemble a comment before it as well
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
					explicit Builder(const log_t * log) noexcept {
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

#endif // __AWH_CODEC_XML_VALUE__
