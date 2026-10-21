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
#pragma once

/**
 * Стандартный заголовочный файл
 */
#include <type_traits>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "writer.hpp"
#include "document.hpp"

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
			 * @brief Класс владеющего значением XML
			 *
			 * @details Тип этот стоит **над** деревом разметки, а не вместо него. Дерево
			 *          разбирает текст и хранит узлы ареной, отчего узел `Document::Node` владельцем
			 *          не является и дерево пережить не может.
			 *
			 * @details Владеющее значение решает ровно ту задачу, какую узел решать не
			 *          вправе: оно держит своё поддерево собственной памятью и потому складывается
			 *          из значений языка, копируется, кладётся во вместилища языка, передаётся
			 *          вглубь и **отдаётся наружу итогом метода**.
			 *
			 * @details Облик этого типа общий у всех кодеков рамки: имена действий и правила
			 *          их поведения одинаковы у JSON, XML, YAML, TOML и INI. Своеобразия разметки
			 *          прибавлены сверху, а не втиснуты в общее ядро: имя с пространством имён,
			 *          свойства узла, связывания префиксов и девять видов узла против шести у JSON.
			 *
			 * @details Мост обратный - `Document::graft`: значение, наружу отданное и правке
			 *          подвергнутое, становится обратно на место узла дерева. Арена дерева при том
			 *          лишь дописывается, а узлы заменённого поддерева остаются в ней недостижимыми:
			 *          перенумерование их обесценило бы всякую ссылку, выданную наружу прежде.
			 *
			 * @note Оформления исходного текста значение не удерживает: расстановка
			 *       пробелов принадлежит дереву, и правка чужого текста с сохранением его
			 *       неприкосновенности остаётся за деревом. Это два разных употребления
			 *
			 * \~english
			 * @brief Class representing an XML value
			 *
			 * @details This type stands **above** the markup tree rather than instead of it. The tree
			 *          parses a text and stores the nodes in an arena, whereby a node `Document::Node` is not an owner
			 *          and cannot outlive the tree.
			 *
			 * @details The owning value solves exactly the task which a node has no
			 *          right to solve: it holds its subtree by its own memory and therefore is assembled
			 *          from the values of the language, is copied, is placed into the containers of the language, is passed
			 *          inwards and **is given away outwards as the result of a method**.
			 *
			 * @details The shape of this type is common for all the codecs of the framework: the names of the actions and the rules
			 *          of their behaviour are identical for JSON, XML, YAML, TOML and INI. The peculiarities of the markup
			 *          are added on top rather than squeezed into the common core: a name with a namespace,
			 *          the properties of a node, the bindings of the prefixes and nine kinds of a node against six for JSON.
			 *
			 * @details The reverse bridge is `Document::graft`: a value given away outwards and subjected
			 *          to editing takes its place back at a node of the tree. The arena of the tree is thereby
			 *          only appended to, while the nodes of the replaced subtree remain in it unreachable:
			 *          a renumbering of them would invalidate every reference given away outwards before.
			 *
			 * @note The value does not retain the formatting of the source text: the placement
			 *       of the spaces belongs to the tree, and the editing of a foreign text with the preservation of its
			 *       intactness remains with the tree. These are two different usages
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Value {
				public:
					/**
					 * \~russian
					 * @brief Структура свойства владеющего значения
					 *
					 * @details Свойство хранится строками собственными, а не отрезками чужого
					 *          текста: значение владеющее дерево пережить обязано.
					 *
					 * @note Тип этот заведён вложенным намеренно: свойство узла дерева,
					 *       живущее отрезками разбираемого текста, зовётся `attribute_t` и
					 *       лежит рядом, в пространстве имён кодека
					 *
					 * \~english
					 * @brief Structure of a property that holds a value
					 *
					 * @details A property is stored by its own strings rather than by the segments of a foreign
					 *          text: an owning value must outlive the tree.
					 *
					 * @note This type is made nested deliberately: a property of a node of a tree,
					 *       living by the segments of the text being parsed, is called `attribute_t` and
					 *       lies alongside, in the namespace of the codec
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Property {
						// Обозначение связанного пространства имён
						string uri;
						// Значение свойства, приведённое к окончательному виду
						string value;
						// Местное имя без префикса
						string local;
						// Префикс пространства имён без разделителя
						string prefix;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Property() noexcept;
					} property_t;
					/**
					 * \~russian
					 * @brief Структура связывания префикса с пространством имён
					 *
					 * @details Связывание принадлежит узлу, где объявлено, и действует внутри
					 *          всех вложенных в него, пока не будет переопределено.
					 *
					 * \~english
					 * @brief Structure for binding a prefix to a namespace
					 *
					 * @details A binding belongs to the node where it is declared and acts inside
					 *          all the ones nested into it until it is redefined.
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Namespace {
						// Обозначение пространства имён, пустое у отмены связывания
						string uri;
						// Префикс без разделителя, пустой у объявления по умолчанию
						string prefix;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Namespace() noexcept;
					} namespace_t;
				private:
					/**
					 * \~russian
					 * @brief Структура записи отображения имён: первый узел с именем и число одноимённых
					 *
					 * @details Числа одноимённых довольно, чтобы выдать ВСЕХ совпавших без
					 *          перебора там, где он всего дороже: у широкого узла разных имён совпавший
					 *          один, и перебор всего перечня ради него был квадратичен - 40.8 мкс на
					 *          обращение при 20000 детях.
					 *
					 * @note Хранить положения ВСЕХ одноимённых незачем: там, где их много, и
					 *       выдача велика, и перебор платится не зря
					 *
					 * \~english
					 * @brief Structure of a name-mapping entry: the first node containing the name, and the count of items with that same name
					 *
					 * @details The counts of the namesakes are enough to issue ALL the coincided ones without
					 *          a traversal where it costs the most: at a wide node of different names the
					 *          coincided one is a single one, and a traversal of the whole list for its sake was
					 *          quadratic - 40.8 µs per an addressing at 20000 children.
					 *
					 * @note There is no sense in keeping the positions of ALL the namesakes: there where there
					 *       are many of them, both the issue is great and the traversal is paid not in vain
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Entry {
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
						explicit Entry(const size_t first = 0) noexcept;
					} entry_t;
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
					 * @note A sign separate from an emptiness of the mapping is mandatory: at a node of only
					 *       text nodes the mapping comes out empty, and a comparison of the sizes would order to
					 *       create it again at every addressing
					 *
					 * \~
					 */
					mutable bool _indexed;
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
					 * @note A sign separate from an emptiness of the mapping is mandatory for the same reason
					 *       as at the mapping of the nested nodes: at a node without properties the mapping
					 *       comes out empty, and a comparison of the sizes would order to create it again at
					 *       every addressing
					 *
					 * \~
					 */
					mutable bool _propertied;
				private:
					// Вид хранимого узла
					kind_t _kind;
					/**
					 * \~russian
					 * @brief Код отказа последней работы над значением
					 *
					 * @details Поле изменчиво нарочно: выдача текста значения идёт работой
					 *          неизменной, а сообщать отказ обязана - иначе пустой текст неотличим
					 *          от значения пустого.
					 *
					 * \~english
					 * @brief Code of the refusal of the last operation over the value
					 *
					 * @details The field is mutable deliberately: the issuance of the text of a value
					 *          is a constant operation, while it is obliged to report a refusal — otherwise
					 *          an empty text is indistinguishable from an empty value.
					 *
					 * \~
					 */
					mutable error_t _error;
					// Место отказа последней работы над значением
					mutable location_t _errorLocation;
				private:
					// Обозначение пространства имён узла разметки
					string _uri;
					/**
					 * \~russian
					 * Собственное содержимое узла
					 *
					 * @details Содержимым владеют узлы текстовые, дословные разделы, примечания,
					 *          указания обработчику и описание типа документа. У узла разметки
					 *          собственного содержимого нет вовсе: содержимое его лежит вложенными узлами.
					 *
					 * \~english
					 * Own content of the node
					 *
					 * @details The content is owned by the text nodes, the literal sections, the comments,
					 *          the processing instructions and the description of the type of the document. A markup node
					 *          has no own content at all: its content lies in the nested nodes.
					 *
					 * \~
					 */
					string _text;
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
					/**
					 * \~russian
					 * Префикс пространства имён имени узла
					 *
					 * @note Префикс удерживается наравне с обозначением пространства имён, хотя
					 *       сличению имён он и не подлежит: без него запись выданного текста
					 *       разошлась бы с записью исходного, а отвечающие по UPnP ставят
					 *       префиксы всякий по-своему
					 *
					 * \~english
					 * Prefix of the namespace of the name of the node
					 *
					 * @note The prefix is retained on a par with the designation of the namespace, although
					 *       it is not subject to the comparison of the names: without it the record of the issued text
					 *       would diverge from the record of the source one, while those answering by UPnP put
					 *       the prefixes each in its own way
					 *
					 * \~
					 */
					string _prefix;
				private:
					// Вложенные узлы в порядке их следования
					vector <Value> _items;
					// Связывания префиксов, объявленные узлом разметки
					vector <namespace_t> _bindings;
					// Свойства узла разметки в порядке их следования
					vector <property_t> _attributes;
				private:
					/**
					 * \~russian
					 * Отображение местных имён вложенных узлов на их номера, заводимое по требованию
					 *
					 * @details Узел о немногих детях разыскивается перебором, широкий -
					 *          отображением. Хранится ПЕРВОЕ вхождение имени: повтор имени у соседних
					 *          узлов - обычный вид разметки, а розыск отвечает первым из них.
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
					 * Mapping of the local names of the nested nodes onto their numbers, created on demand
					 *
					 * @details A node of a few children is searched for by a traversal, a wide one - by a
					 *          mapping. The FIRST occurrence of a name is kept: a repeating name at the
					 *          neighbouring nodes is an ordinary form of a markup, while the search answers by
					 *          the first of them.
					 *
					 * @note The mapping serves a search by ONE LOCAL NAME ONLY - an `operator []` addressing
					 *       and a walk by a path. A search by a pair of a name and a namespace does not use it:
					 *       the first coincidence of a local name may turn out to be a node of an alien
					 *       namespace, and the mapping would answer with not that node
					 *
					 * @warning Any editing of the list of the nested nodes is BOUND to drop the mapping: a
					 *          forgotten drop gives not a slow answer but a wrong one - or even a stop of the
					 *          work. A number from a stale mapping goes beyond the end of the list, `at()` throws
					 *          an exception from inside a `noexcept`, and the application ends. Checked on
					 *          2026-08-22 by a deliberately emptied drop: `CodecXmlValue.WideNodeMutators`
					 *          brings the stand down exactly that way
					 *
					 * @warning Giving out an MUTABLE reference to an already CREATED nested node drops the
					 *          mapping, and that is not an over-insurance. The name of a markup node is kept at
					 *          THE NODE ITSELF, and not at the parent, and the one who received a reference has a
					 *          right to assign the node another value in full - together with another name. The
					 *          parent does not observe that editing in any way. There are FOUR such places: an
					 *          addressing by a name, an addressing by a number within the list, an addressing by a
					 *          number beyond it, and a walk by a path. Found by a probe on 2026-08-22: the mapping
					 *          answered that a removed name is there while a new one is not, and `contains`
					 *          parted from an addressing by a name at that, answering oppositely about ONE node
					 *
					 * @note The codec JSON does not know this trouble at all, and therefore a comparison with
					 *       it here is deceptive: the name of a field lies there at the PARENT, as a separate
					 *       list, and an assigning of a value to a field does not touch its name
					 *
					 * @note The creation of a NEW node needs no drop and goes by a top-up: its name is known to
					 *       the parent at that very moment
					 *
					 * @note A comparison of a number with the length of the list is deliberately NOT here: it
					 *       would turn a forgotten drop into a quiet self-repair, paying a comparison at every
					 *       search, and would hide a defect instead of showing it. Instead, every path of an
					 *       editing is fixed by a check of the set
					 *
					 * \~
					 */
					mutable unordered_map <string, entry_t> _index;
					/**
					 * \~russian
					 * Отображение имён свойств узла на их номера, заводимое по требованию
					 *
					 * @details Ключ двухуровневый - местное имя, а под ним обозначение пространства
					 *          имён, - и составного ключа отсюда не собирается нигде. Ключ, склеенный из
					 *          двух имён, требовал бы выделения памяти на ВСЯКОМ розыске, тогда как
					 *          розыск обязан обходиться без выделений вовсе.
					 *
					 * @note Отображение это отвечает ТОЧНО, в отличие от отображения вложенных
					 *       узлов: сличение свойств идёт по паре имени и пространства имён, и
					 *       двух свойств с одинаковой парой у узла не бывает - установка
					 *       перезаписывает такое свойство на его же месте. Оттого отката к
					 *       перебору здесь нет ни на одном пути
					 *
					 * \~english
					 * Mapping of the names of the properties of a node onto their numbers, created on demand
					 *
					 * @details The key is two-leveled - a local name, and under it a designation of a
					 *          namespace, - and a composite key is assembled nowhere out of it. A key glued of
					 *          two names would demand an allocation of the memory at EVERY search, while a search
					 *          is bound to do without any allocations at all.
					 *
					 * @note This mapping answers EXACTLY, in distinction from the mapping of the nested nodes:
					 *       a comparison of the properties goes by a pair of a name and a namespace, and a node
					 *       never has two properties with the same pair - an installation rewrites such a
					 *       property at its own place. Therefore there is no rollback to a traversal here at any
					 *       path
					 *
					 * \~
					 */
					mutable unordered_map <string, unordered_map <string, size_t>> _properties;
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
					 * @note A reset is mandatory at ANY editing of the list of the nested nodes, and not at a
					 *       removal alone: a creation of a node with a new name would leave the mapping answering
					 *       that its name is absent
					 *
					 * \~
					 */
					void reindex() const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия вложенного узла разметки с правкой отображения имён
					 *
					 * @details Снятие узла ПОСЛЕДНЕГО номеров прочих узлов не двигает, и сноса
					 *          отображения не требует: довольно изъять из него одну запись. Снос обошёлся
					 *          бы разбором всего отображения, тогда как сдвигать в перечне при том нечего.
					 *
					 * @note Замерено 23.08.2026: снос отображения стоил 132 мкс при 16384 узлах,
					 *       тогда как само изъятие из перечня - 15 нс. Разряд сложности снятия
					 *       последнего сносом обращался из постоянного в линейный
					 *
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
					 * @details A removal of the LAST node does not shift the numbers of the rest and needs no
					 *          drop of the mapping: it is enough to extract a single record from it. A drop would
					 *          come to a rebuilding of the whole mapping, when there is nothing to shift in the
					 *          list at that.
					 *
					 * @note Measured on 2026-08-23: a drop of the mapping cost 132 µs at 16384 nodes, while the
					 *       extraction from the list itself - 15 ns. The complexity class of a removal of the
					 *       last node would turn from a constant into a linear one by that drop
					 *
					 * @warning A removal from the MIDDLE of the list is not covered by an editing: it shifts the
					 *          numbers of all the nodes following the removed one. A removal of a first
					 *          occurrence of a name which has namesakes is not covered either: the place of the
					 *          next of them is unknown to the mapping
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
					 *          перечня: сброс вместо долива обращает заведение узлов подряд в
					 *          перестроение отображения на всякой вставке, и плата на одну вставку
					 *          растёт с числом уже заведённых.
					 *
					 * @note Метод зовётся ПОСЛЕ добавления узла в перечень: номером служит
					 *       место, какое узел в перечне занял
					 *
					 * @param local местное имя заведённого вложенного узла
					 *
					 * \~english
					 * @brief Method of the accounting of an added nested markup node in the mapping of names
					 *
					 * @details A top-up of the mapping is mandatory there where a node is created AT THE END of
					 *          the list: a drop instead of a top-up turns a creating of the nodes in a row into a
					 *          rebuilding of the mapping at every insertion, and the price per an insertion grows
					 *          with the number of the already created ones.
					 *
					 * @note The method is called AFTER the adding of a node into the list: the number serves the
					 *       place which the node took in the list
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
					 *          содержимое его: документ, вложенный в разметку, записывается верно, а
					 *          вот число корней он вносит своё. Подсчёт потому и ведётся возвратом.
					 *
					 * @return количество корневых узлов разметки
					 *
					 * \~english
					 * @brief Method of the counting of the root markup nodes brought in by a value
					 *
					 * @details The writing UNWRAPS a node of the kind of a document, issuing only its
					 *          content: a document nested into a markup is written correctly, while the number
					 *          of the roots it brings in is its own. Therefore the counting is performed by a recursion.
					 *
					 * @return number of the root markup nodes
					 *
					 * \~
					 */
					uint32_t roots() const noexcept;
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
					 * @note A reset is mandatory at any editing of the list of the properties, EXCEPT a
					 *       creating of a property at the end: there the mapping is topped up, and not
					 *       dropped
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
					 * @note The method is called AFTER the adding of a property into the list: the number
					 *       serves the place which the property took in the list
					 *
					 * @param local the local name of the added property
					 * @param uri   the namespace designation of the added property
					 *
					 * \~
					 */
					void propertied(const string & local, const string & uri) const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон метода извлечения числа затребованным видом
					 *
					 * @tparam T затребованный вид числа
					 *
					 * \~english
					 * @brief Method template for extracting a number in a requested format
					 *
					 * @tparam T demanded kind of the number
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод извлечения числа затребованным видом
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method for extracting a number in a requested format
					 *
					 * @param result variable where the extracted value is placed
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool extract(T & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения в поток записи
					 *
					 * @details Отказ потока записи разбирается немедля и наверх передаётся
					 *          отказом: продолжение записи после отказа оставило бы в тексте узел
					 *          открытым либо содержимое его недописанным.
					 *
					 * @param writer поток записи, куда ложится значение
					 * @return       признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of the value into a writing stream
					 *
					 * @details A refusal of the writing stream is handled at once and is passed upwards
					 *          as a refusal: a continuation of the writing after a refusal would leave in the text a node
					 *          open or its content unfinished.
					 *
					 * @param writer writing stream where the value is placed
					 * @return       sign of the success of the writing
					 *
					 * \~
					 */
					bool compose(writer_t & writer, const bool preserve = false) const noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия значения с узла дерева разметки
					 *
					 * @details Дерево глубже предела `MAX_DEPTH` не снимается вовсе: снятие
					 *          возвратно, и глубина дерева ложится прямо на стек вызовов - замер дал
					 *          срыв стека около 20 000 уровней. Дерево, снятию не поддавшееся,
					 *          оставляет значение неопределённым, как оставляет его и узел
					 *          недействительный, а причина уходит в журнал.
					 *
					 * @param node  узел дерева разметки
					 * @param depth глубина вложенности снимаемого узла
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the taking of a value from a node of a markup tree
					 *
					 * @details A tree deeper than the limit `MAX_DEPTH` is not taken at all: the taking
					 *          is recursive, and the depth of the tree falls directly upon the stack of the calls — a measurement gave
					 *          an overflow of the stack at about 20 000 levels. A tree that has not yielded to the taking
					 *          leaves the value undefined, just as an invalid node leaves it, while the reason goes to the log.
					 *
					 * @param node  node of the markup tree
					 * @param depth depth of the nesting of the node being taken
					 * @return      flag of the success of the taking
					 *
					 * \~
					 */
					bool absorb(const node_t & node, const uint32_t depth = 0, const bool reset = true) noexcept;
					/**
					 * \~russian
					 * @brief Метод сбора содержимого вложенных текстовых узлов
					 *
					 * @param result строка, куда собирается содержимое
					 *
					 * \~english
					 * @brief Method of the gathering of the content of the nested text nodes
					 *
					 * @param result string where the content is gathered
					 *
					 * \~
					 */
					void gather(string & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения неопределённого
					 *
					 * @details Значение это одно на все отказы обращения только для чтения:
					 *          выдать ссылку метод обязан, а ссылки на несуществующее не бывает.
					 *
					 * @return значение неопределённое
					 *
					 * \~english
					 * @brief Method of the extraction of an undefined value
					 *
					 * @details This value is one for all the refusals of a read-only access:
					 *          a method is obliged to give away a reference, while a reference to a non-existent thing does not exist.
					 *
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
					 *          изменяемом. Записанное в него пропадает при следующем же неудачном
					 *          обращении, и полагаться на него нельзя.
					 *
					 * @note Значение это своё у всякого потока: одно на приложение оно обратило
					 *       бы отказ обращения в состязание за общую память
					 *
					 * @return значение мусорное
					 *
					 * \~english
					 * @brief Method of the extraction of a scrap value
					 *
					 * @details This value takes upon itself a writing at an unsuccessful mutable
					 *          access. What is written into it disappears at the very next unsuccessful
					 *          access, and one must not rely upon it.
					 *
					 * @note This value is its own for every thread: one for the whole application it would turn
					 *       a refusal of an access into a contention for a common memory
					 *
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
					 *          неопределёнными до затребованного номера, и номер, пришедший извне -
					 *          из настроек либо из запроса, - обращается требованием памяти по нему.
					 *          Предел этот рост и стережёт: обращение за пределом заводит вместо
					 *          значения значение мусорное, а вместилище не растит вовсе.
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
					 *
					 * @details An access by an index grows a container by undefined values up to the requested
					 *          index, and an index that came from outside — from the settings or from a request — turns
					 *          into a demand of the memory by it. This limit guards that growth: an access beyond the limit
					 *          creates a scrap value instead of a value and does not grow the container at all.
					 *
					 * @note The limit guards only the growth by an access by an index. A container built by a parsing
					 *       or by an addition of values at the end is not subject to it: the length of a text being
					 *       parsed is set by the user of the framework, and the framework has no right to decide
					 *       for them how many values they are allowed
					 *
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
					 *          приложения, ведомо ему одному. Значение по умолчанию - 65536 значений.
					 *
					 * @param value устанавливаемый предел, нуль снимает предел вовсе
					 *
					 * \~english
					 * @brief Method of the setting of the limit of the growth of a container by an index
					 *
					 * @details This limit is set by the user of the framework: how much memory an application has
					 *          is known to it alone. The default value is 65536 values.
					 *
					 * @param value limit being set, zero removes the limit altogether
					 *
					 * \~
					 */
					static void limit(const size_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки определённости значения
					 *
					 * @return признак определённости значения
					 *
					 * \~english
					 * @brief Method of the check of the definiteness of the value
					 *
					 * @return sign of the definiteness of the value
					 *
					 * \~
					 */
					bool valid() const noexcept;
					/**
					 * \~russian
					 * @brief Оператор приведения к логическому типу
					 *
					 * @return признак действительности значения
					 *
					 * \~english
					 * @brief Conversion operator to the logical type
					 *
					 * @return sign of the validity of the value
					 *
					 * \~
					 */
					explicit operator bool () const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения кода отказа последней работы над значением
					 *
					 * @details Код этот отвечает на вопрос, отчего работа не сбылась: разбор
					 *          текста, чтение файла, выдача текста и запись его в файл ставят его
					 *          заново каждая. Прежде канала отказа у значения не было вовсе, и пустой
					 *          текст выдачи был неотличим от значения пустого: замер дал одиннадцать
					 *          способов завести значение, записи не подлежащее, и все они молчали.
					 *
					 * @note Работы, отказать не могущие, кода не трогают: снятие вида, имени
					 *       и содержимого идёт мимо этого канала
					 *
					 * @return код отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the code of the refusal of the last operation over the value
					 *
					 * @details This code answers the question of why an operation did not come true: the parsing
					 *          of a text, the reading of a file, the issuance of a text and the writing of it into a file
					 *          each set it anew. Formerly a value had no channel of a refusal at all, and the empty text
					 *          of the issuance was indistinguishable from an empty value: a measurement gave eleven ways
					 *          to obtain a value not subject to the writing, and all of them kept silent.
					 *
					 * @note The operations that cannot refuse do not touch the code: the taking of the kind,
					 *       of the name and of the content goes past this channel
					 *
					 * @return code of the refusal of the last operation
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места отказа последней работы над значением
					 *
					 * @details Место называет строку и столбец текста, на которых разбор
					 *          споткнулся. У работ, места не имеющих - запись в файл, выдача
					 *          текста, - оно остаётся пустым.
					 *
					 * @return место отказа последней работы
					 *
					 * \~english
					 * @brief Method of the getting of the location of the refusal of the last operation over the value
					 *
					 * @details The location names the line and the column of the text at which the parsing has
					 *          stumbled. Among the operations having no location - the writing into a file, the
					 *          issuing of a text - it stays empty.
					 *
					 * @return location of the refusal of the last operation
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида узла
					 *
					 * @note Вид этот — вид УЗЛА разметки, а не общий вид значения: у прочих
					 *       кодеков `kind()` отвечает на вопрос о значении, здесь — о
					 *       строении дерева. Подводить его под `codec::kind_t` значило бы
					 *       смешать два разных вопроса
					 *
					 * @return вид хранимого узла
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the node
					 *
					 * @note This kind is the kind of a MARKUP NODE, not the common kind of a value:
					 *       at the other codecs `kind()` answers the question about a value, while here -
					 *       about the arrangement of the tree. To bring it under `codec::kind_t` would mean
					 *       mixing two different questions
					 *
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
					 *
					 * @param kind kind of the node being compared
					 * @return     sign of the coincidence of the kind
					 *
					 * \~
					 */
					bool is(const kind_t kind) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения количества вложенных узлов
					 *
					 * @return количество вложенных узлов
					 *
					 * \~english
					 * @brief Method of the extraction of the number of the nested nodes
					 *
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
					 *
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
					 *
					 * @note The segments of the issued name point to the memory of the value itself and live
					 *       exactly as long as it lives itself
					 *
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
					 *
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
					 *
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
					 *
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
					 *
					 * @param local  local name of the node being set
					 * @param uri    designation of the namespace being set
					 * @param prefix prefix of the namespace being set
					 *
					 * \~
					 */
					void name(const string & local, const string & uri = "", const string & prefix = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения содержимого узла
					 *
					 * @details У узла текстового, дословного раздела, примечания, указания
					 *          обработчику и описания типа документа выдаётся собственное содержимое их.
					 *          У узла разметки - содержимое всех вложенных в него текстовых узлов
					 *          подряд, ровно как это делает узел дерева.
					 *
					 * @note Возврат идёт значением, а не ссылкой: у узла разметки содержимое
					 *       собирается из вложенных узлов, и ссылке было бы не на что указывать
					 *
					 * @return содержимое узла
					 *
					 * \~english
					 * @brief Method of the extraction of the content of the node
					 *
					 * @details For a text node, a literal section, a comment, a processing
					 *          instruction and a description of the type of the document their own content is issued.
					 *          For a markup node — the content of all the text nodes nested into it
					 *          in a row, exactly as a node of the tree does it.
					 *
					 * @note The content is returned by value rather than by reference: at a markup node
					 *       it is gathered from the nested nodes, and a reference would have nothing to point at
					 *
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
					 *          одним узлом текстовым: содержимого своего у разметки нет вовсе.
					 *
					 * @param text устанавливаемое содержимое узла
					 * @return     признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of the own content of the node
					 *
					 * @details For a markup node the setting of the content replaces all the nested nodes
					 *          by one text node: the markup has no own content at all.
					 *
					 * @param text content of the node being set
					 * @return     sign of the success of the setting
					 *
					 * \~
					 */
					bool text(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения свойств узла разметки
					 *
					 * @return перечень свойств узла в порядке их следования
					 *
					 * \~english
					 * @brief Method of the extraction of the properties of a markup node
					 *
					 * @return list of the properties of the node in the order of their following
					 *
					 * \~
					 */
					const vector <property_t> & attributes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения свойства узла разметки
					 *
					 * @warning Розыск отличается от ЗАВЕДЕНИЯ свойства лишь неизменяемостью
					 *          узла: зов о двух доводах у узла изменяемого выбирает заведение
					 *          `attribute(local, value)` и записывает пустое значение вместо розыска.
					 *          Разыскивать надлежит у неизменяемой ссылки на узел
					 *
					 * @param local местное имя разыскиваемого свойства
					 * @param uri   обозначение пространства имён свойства
					 * @return      значение свойства, пустое - свойства такого нет
					 *
					 * \~english
					 * @brief Method of the extraction of the value of a property of a markup node
					 *
					 * @warning A search differs from a CREATING of a property by the immutability of the node
					 *          alone: a call of two arguments at a mutable node chooses the creating
					 *          `attribute(local, value)` and writes an empty value instead of a search. To search
					 *          is to be done at an immutable reference to a node
					 *
					 * @param local local name of the property being searched for
					 * @param uri   designation of the namespace of the property
					 * @return      value of the property, an empty one - there is no such property
					 *
					 * \~
					 */
					const string & attribute(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки свойства узла разметки
					 *
					 * @details Свойство с уже занятым именем перезаписывается на своём месте, а
					 *          порядок свойств при том сохраняется.
					 *
					 * @param local  местное имя свойства
					 * @param value  устанавливаемое значение свойства
					 * @param uri    обозначение пространства имён свойства
					 * @param prefix префикс пространства имён свойства
					 * @return       признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a property of a markup node
					 *
					 * @details A property with an already occupied name is overwritten in its place, while
					 *          the order of the properties is thereby preserved.
					 *
					 * @param local  local name of the property
					 * @param value  value of the property being set
					 * @param uri    designation of the namespace of the property
					 * @param prefix prefix of the namespace of the property
					 * @return       sign of the success of the setting
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
					 *
					 * @param local local name of the property being searched for
					 * @param uri   designation of the namespace of the property
					 * @return      sign of the presence of the property
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
					 *
					 * @param local local name of the property being removed
					 * @param uri   designation of the namespace of the property
					 * @return      sign of the success of the removal
					 *
					 * \~
					 */
					bool detach(const string & local, const string & uri = "") noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения связываний префиксов, объявленных узлом
					 *
					 * @return перечень связываний префиксов
					 *
					 * \~english
					 * @brief Method of the extraction of the bindings of the prefixes declared by the node
					 *
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
					 *
					 * @param prefix prefix being declared, an empty one — a declaration by default
					 * @param uri    designation of the namespace, an empty one — a cancellation of the binding
					 * @return       sign of the success of the declaration
					 *
					 * \~
					 */
					bool binding(const string & prefix, const string & uri) noexcept;
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
					 *
					 * @param local local name of the node being searched for
					 * @param uri   designation of the namespace of the node
					 * @return      sign of the presence of the nested node
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
					 *
					 * @param local local name of the nodes being searched for
					 * @param uri   designation of the namespace of the nodes
					 * @return      list of the pointers to the found nodes
					 *
					 * \~
					 */
					vector <const Value *> children(const string & local, const string & uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к узлу по пути
					 *
					 * @details Путь записывается частями, разделёнными косой чертой:
					 *          `/Envelope/Body/0`. Часть числовая обращается к вложенному узлу по
					 *          номеру, а прочие - к узлу разметки по местному имени. Обращение к
					 *          отсутствующему пути ничего не заводит и отдаёт значение неопределённое.
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param path путь к разыскиваемому узлу
					 * @return     ссылка на разысканный узел
					 *
					 * \~english
					 * @brief Method of the access to a node by a path
					 *
					 * @details The path is written by the parts separated by a slash:
					 *          `/Envelope/Body/0`. A numeric part accesses a nested node by an
					 *          index, while the others access a markup node by a local name. An access to
					 *          an absent path creates nothing and gives away an undefined value.
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param path path to the node being searched for
					 * @return     reference to the found node
					 *
					 * \~
					 */
					const Value & at(const string & path) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения к узлу по пути с заведением недостающего
					 *
					 * @details Недостающие звенья пути заводятся узлами разметки с местным
					 *          именем звена. Звено числовое недостающего не заводит и отвечает
					 *          значением мусорным: имени у заводимого узла взять неоткуда.
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param path путь к разыскиваемому узлу
					 * @return     ссылка на разысканный либо заведённый узел
					 *
					 * \~english
					 * @brief Method of the access to a node by a path with the creation of the missing ones
					 *
					 * @details The missing links of the path are created as markup nodes with the local
					 *          name of the link. A numeric link does not create a missing one and answers
					 *          with a scrap value: there is nowhere to take the name of the node being created from.
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param path path to the node being searched for
					 * @return     reference to the found or created node
					 *
					 * \~
					 */
					Value & place(const string & path) noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к вложенному узлу разметки по местному имени
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param local местное имя вложенного узла
					 * @return      ссылка на первый вложенный узел с таким именем
					 *
					 * \~english
					 * @brief Operator for accessing a nested markup node by its local name
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param local local name of the nested node
					 * @return      reference to the first nested node with such name
					 *
					 * \~
					 */
					const Value & operator [] (const string & local) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к вложенному узлу разметки по местному имени с
					 *        заведением недостающего
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param local местное имя вложенного узла
					 * @return      ссылка на первый вложенный узел с таким именем
					 *
					 * \~english
					 * @brief Operator for accessing a nested markup node by its local name,
					 *        with creation of the node if it is missing
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param local local name of the nested node
					 * @return      reference to the first nested node with such name
					 *
					 * \~
					 */
					Value & operator [] (const string & local) noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к вложенному узлу по номеру
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param index номер вложенного узла
					 * @return      ссылка на вложенный узел
					 *
					 * \~english
					 * @brief Operator for accessing a nested node by index
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param index index of the nested node
					 * @return      reference to the nested node
					 *
					 * \~
					 */
					const Value & operator [] (const size_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Оператор обращения к вложенному узлу по номеру с заведением недостающего
					 *
					 * @details Обращение за границу перечня растит его до затребованного номера
					 *          узлами неопределёнными.
					 *
					 * @warning Выданная ссылка живёт лишь до ближайшего заведения соседа:
					 *          дети лежат в перемещаемом вместилище, и рост его ОБЕСЦЕНИВАЕТ ссылки на
					 *          прежних детей. Замер: чтение ссылки после заведения соседей есть обращение
					 *          к освобождённой памяти, надзор валит его по праву. Держать надлежит не
					 *          ссылку, а путь к узлу, и снимать её заново
					 *
					 * @param index номер вложенного узла
					 * @return      ссылка на вложенный узел
					 *
					 * \~english
					 * @brief Operator for accessing a nested node by index, with automatic creation of any missing nodes
					 *
					 * @details An access beyond the boundary of the list grows it up to the demanded index
					 *          by the undefined nodes.
					 *
					 * @warning The issued reference lives only until the nearest institution of a sibling:
					 *          the children lie in a movable container, and its growth INVALIDATES the references to
					 *          the previous children. Measurement: a reading of the reference after the institution of the siblings is an access
					 *          to a freed memory, the sanitizer rightfully aborts on it. What should be held is not
					 *          the reference but the path to the node, and it should be taken anew
					 *
					 * @param index index of the nested node
					 * @return      reference to the nested node
					 *
					 * \~
					 */
					Value & operator [] (const size_t index) noexcept;
					/**
					 * \~russian
					 * @brief Шаблон оператора обращения по написанию знака
					 *
					 * @tparam T разряд обращения, ограда пропускает ровно знак
					 *
					 * \~english
					 * @brief Operator template for character writing
					 *
					 * @tparam T width of the access, the guard admits exactly a character
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Оператор обращения по написанию знака
					 *
					 * @details Написание знака есть ТЕКСТ, и обращение по нему разыскивает
					 *          ИМЯ из одного знака, а не номер. Правило это одно на весь кодек.
					 *
					 * @note Без этого метода `v['z']` молча обращался к элементу под номером
					 *       122 и отдавал пустоту, тогда как `v["z"]` отдавал значение.
					 *       Перечнем беда не лечится: заводитель по разряду `char` делает
					 *       двусмысленным обычное `v[0]`, оттого здесь шаблон с оградой
					 *
					 * @param name разыскиваемое имя из одного знака
					 * @return     ссылка на дочерний узел
					 *
					 * \~english
					 * @brief Operator for handling the writing of a character
					 *
					 * @details A writing of a character is a TEXT, and an access by it searches for a NAME
					 *          of a single character rather than for an index. This rule is one for the whole codec.
					 *
					 * @note Without this method `v['z']` silently addressed the element under the index 122
					 *       and gave away an emptiness, while `v["z"]` gave away a value. By a list the
					 *       trouble is not cured: a creator by the `char` width makes the ordinary `v[0]`
					 *       ambiguous, hence a template with a guard here
					 *
					 * @param name name of a single character being searched for
					 * @return     reference to the child node
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <T, char>::value, const Value &>::type operator [] (const T name) const noexcept;
					/**
					 * \~russian
					 * @brief Шаблон оператора обращения по написанию знака
					 *
					 * @tparam T разряд обращения, ограда пропускает ровно знак
					 *
					 * \~english
					 * @brief Operator template for character writing
					 *
					 * @tparam T width of the access, the guard admits exactly a character
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Оператор обращения по написанию знака
					 *
					 * @details Написание знака есть ТЕКСТ, и обращение по нему разыскивает
					 *          ИМЯ из одного знака, а не номер. Правило это одно на весь кодек.
					 *
					 * @note Без этого метода `v['z']` молча обращался к элементу под номером
					 *       122 и отдавал пустоту, тогда как `v["z"]` отдавал значение.
					 *       Перечнем беда не лечится: заводитель по разряду `char` делает
					 *       двусмысленным обычное `v[0]`, оттого здесь шаблон с оградой
					 *
					 * @param name разыскиваемое имя из одного знака
					 * @return     ссылка на дочерний узел
					 *
					 * \~english
					 * @brief Operator for handling the writing of a character
					 *
					 * @details A writing of a character is a TEXT, and an access by it searches for a NAME
					 *          of a single character rather than for an index. This rule is one for the whole codec.
					 *
					 * @note Without this method `v['z']` silently addressed the element under the index 122
					 *       and gave away an emptiness, while `v["z"]` gave away a value. By a list the
					 *       trouble is not cured: a creator by the `char` width makes the ordinary `v[0]`
					 *       ambiguous, hence a template with a guard here
					 *
					 * @param name name of a single character being searched for
					 * @return     reference to the child node
					 *
					 * \~
					 */
					typename std::enable_if <std::is_same <T, char>::value, Value &>::type operator [] (const T name) noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления узла в конец перечня вложенных
					 *
					 * @details Значение неопределённое перерождается узлом разметки, а узел
					 *          текстовый вложенных узлов не имеет вовсе и добавление отвечает отказом.
					 *
					 * @param value добавляемый узел
					 * @return      признак успешности добавления
					 *
					 * \~english
					 * @brief Method of the addition of a node to the end of the list of the nested ones
					 *
					 * @details An undefined value is reborn as a markup node, while a text node
					 *          has no nested nodes at all and the addition responds with a refusal.
					 *
					 * @param value node being added
					 * @return      sign of the success of the addition
					 *
					 * \~
					 */
					bool push(const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод установки вложенного узла разметки по местному имени
					 *
					 * @details Узел с уже занятым именем перезаписывается на своём месте, а
					 *          порядок вложенных узлов при том сохраняется. Имя узла устанавливается
					 *          заданным именем, каким бы имя подаваемого узла ни было.
					 *
					 * @param local местное имя вложенного узла
					 * @param value устанавливаемый узел
					 * @return      признак успешности установки
					 *
					 * \~english
					 * @brief Method of the setting of a nested markup node by a local name
					 *
					 * @details A node with an already occupied name is overwritten in its place, while
					 *          the order of the nested nodes is thereby preserved. The name of the node is set
					 *          by the given name, whatever the name of the node being passed may be.
					 *
					 * @param local local name of the nested node
					 * @param value node being set
					 * @return      sign of the success of the setting
					 *
					 * \~
					 */
					bool insert(const string & local, const Value & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия вложенного узла разметки по местному имени
					 *
					 * @details Снимается первый вложенный узел с таким именем, а не все они:
					 *          узлов с одним именем у разметки бывает сколько угодно.
					 *
					 * @param local местное имя снимаемого узла
					 * @param uri   обозначение пространства имён узла
					 * @return      признак успешности снятия
					 *
					 * \~english
					 * @brief Method of the removal of a nested markup node by a local name
					 *
					 * @details The first nested node with such name is removed rather than all of them:
					 *          a markup may have any number of the nodes with one name.
					 *
					 * @param local local name of the node being removed
					 * @param uri   designation of the namespace of the node
					 * @return      sign of the success of the removal
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
					 *
					 * @param index index of the node being removed
					 * @return      sign of the success of the removal
					 *
					 * \~
					 */
					bool erase(const size_t index) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения логического значения
					 *
					 * @details Истиной считаются записи `true` и `1`, ложью - `false` и `0`:
					 *          иных записей логического значения описания разметки не знают.
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a logical value
					 *
					 * @details The records `true` and `1` are considered a truth, `false` and `0` a falsehood:
					 *          the descriptions of the markup know no other records of a logical value.
					 *
					 * @param result variable where the extracted value is placed
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(bool & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения числа
					 *
					 * @details Число разбирается из содержимого узла: своих видов у разметки нет
					 *          вовсе, всякое содержимое её есть текст. Отказом извлечение завершается
					 *          лишь тогда, когда содержимое числом не является.
					 *
					 * @note Договор извлечения общий у всех кодеков рамки: целое сужается
					 *       обычным приведением языка, дробное ОКРУГЛЯЕТСЯ до целого с уводом
					 *       половины от нуля (`1.5` даёт `2`, `-1.5` даёт `-2`), дробное за
					 *       пределом затребованного целого вида выдаётся пределом этого вида,
					 *       а `NaN` - нулём
					 *
					 * @note Дробного с НУЛЕВОЙ дробной частью правило это не касается: `300.0`
					 *       извлекается ровно как `300` и заворачивается по кругу. Запись числа
					 *       ответа менять не должна - решение от 30.08.2026; предел
					 *       вида остаётся лишь там, где дробная часть есть на деле. Совпадение
					 *       записей держится до 2^53: выше `double` целых точно не представляет,
					 *       и `9007199254740993.0` теряет число ещё у разбора
					 *
					 * @param result переменная, куда помещается извлечённое значение
					 * @return       признак успешности извлечения
					 *
					 * \~english
					 * @brief Method of the extraction of a number
					 *
					 * @details A number is parsed from the content of the node: the markup has no kinds of its own
					 *          at all, all its content is a text. The extraction ends with a refusal
					 *          only when the content is not a number.
					 *
					 * @note The contract of the extraction is common to all the codecs of the framework: an
					 *       integer is narrowed by an ordinary casting of the language, a fractional one ROUNDS
					 *       to an integer with a carrying of a half away from zero (`1.5` gives `2`, `-1.5` gives
					 *       `-2`), a fractional one beyond the limit of the demanded integer kind is issued as the
					 *       limit of that kind, while a `NaN` - as a zero
					 *
					 * @note A fractional number with a ZERO fractional part is not touched by this rule: `300.0`
					 *       is extracted exactly as `300` and turns around the circle. The notation of a number of
					 *       an answer is not to be changed - the decision of 2026-08-30; the limit of a kind is
					 *       left only there where a fractional part is in fact present. A coincidence of the
					 *       notations holds up to 2^53: above that a `double` does not represent the integers
					 *       exactly, and `9007199254740993.0` loses the number already at the parsing
					 *
					 * @param result variable where the extracted value is placed
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(int8_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(int16_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(int32_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(int64_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(uint8_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(uint16_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(uint32_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(uint64_t & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(float & result) const noexcept;
					/**
					 * \~russian
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~english
					 * @copydoc awh::codec::xml::Value::value(int8_t &) const
					 *
					 * \~
					 */
					bool value(double & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения строкового значения
					 *
					 * @details Извлечение это отдаёт содержимое узла ВСЕГДА, каким бы оно ни
					 *          было: своих видов у разметки нет вовсе, всякое значение здесь есть текст,
					 *          и отказывать по виду попросту нечему. Узел с записью `1` отвечает истиной
					 *          и выдаёт `1`, хотя извлечение логическим и числовым тот же узел разбирают.
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
					 *
					 * @details This extraction issues the content of the node ALWAYS, whatever it may
					 *          be: the markup has no kinds of its own at all, every value here is a text,
					 *          and there is simply nothing to refuse by the kind. A node with the record `1` answers with a truth
					 *          and issues `1`, although the extraction by a logical and by a numeric parse the same node.
					 *
					 * @note The codecs JSON and TOML answer with a REFUSAL at the same place, and this is NOT
					 *       a divergence by an oversight: they have the kinds, and the extraction by a string
					 *       serves them as a check of the kind, while the record is issued by a separate means.
					 *       The rule follows from the structure of the notation rather than from the taste of the codec
					 *
					 * @param result variable where the extracted value is placed
					 * @return       sign of the success of the extraction
					 *
					 * \~
					 */
					bool value(string & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста настроек
					 *
					 * @param text разбираемый текст настроек
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a settings text
					 *
					 * @param text settings text being parsed
					 * @return     sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки во владеющее значение
					 *
					 * @details Разбор ведётся тем же деревом, каким он ведётся всегда: обёртка
					 *          лишь снимает с него поддерево собственной памятью.
					 *
					 * @note Настройки чтения принимаются доводом, как и у дерева: без него ни
					 *       одна из них - ни разрешение префиксов, ни строгость к внешним сущностям,
					 *       ни подстановка объявленных умолчаний - у владеющего значения недостижима
					 *       вовсе, и разбор обрывка вёлся бы одними умолчаниями
					 *
					 * @param text     разбираемый текст разметки
					 * @param settings настройки чтения текста разметки
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a markup text into an owning value
					 *
					 * @details The parsing is conducted by the same tree by which it is always conducted: the wrapper
					 *          only takes a subtree off it by its own memory.
					 *
					 * @note The settings of the reading are taken as an argument, as with a tree: without it not
					 *       a single one of them — neither the resolution of the prefixes, nor the strictness towards
					 *       the external entities, nor the substitution of the declared defaults — would be reachable
					 *       at all for an owning value, and the parsing of a fragment would be conducted by the defaults alone
					 *
					 * @param text     markup text being parsed
					 * @param settings settings of the reading of a markup text
					 * @return         sign of the success of the parsing
					 *
					 * \~
					 */
					bool parse(const string & text, const reader_t::settings_t & settings) noexcept;
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
					 *
					 * @note The file is raised into the memory in full rather than read by chunks: the
					 *       reservation of the room for the tree is taken by a pre-scan of the whole text, and
					 *       such a parsing cannot be a streaming one. That is what tells the present method from
					 *       the same-named one of the json codec, which reads a file by chunks. A file that does
					 *       not fit into the memory is what the streaming reading of reader_t is meant for
					 *
					 * @param filename address of the file being parsed
					 * @return         sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текста разметки из файла с заданными настройками
					 *
					 * @details Настройки те же, что у разбора текста, и слушаются одинаково:
					 *          вход файловый и вход текстовый суть два входа к ОДНОМУ разбору, и
					 *          пределы, оберегающие его, обязаны быть достижимы с обоих.
					 *
					 * @warning Перегрузка эта заведена 01.09.2026: прежде файловый вход
					 *          настроек не принимал вовсе и разбирал текст умолчаниями, оттого
					 *          пределы имени, глубины и количества сущностей на нём были
					 *          недостижимы. Соседние кодеки рамки перегрузку эту несли, а
					 *          разметка - нет
					 *
					 * @param filename адрес разбираемого файла
					 * @param settings настройки разбора текста разметки
					 * @return         признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of a text of a markup from a file with the given settings
					 *
					 * @details The settings are the same as at the parsing of a text and are obeyed alike:
					 *          the file input and the text input are two inputs to ONE parsing, and the limits
					 *          fencing it are to be reachable from both.
					 *
					 * @warning This overload was made on 2026-09-01: before, the file input took no settings at
					 *          all and parsed a text by the defaults, so the limits of a name, of a depth and of
					 *          the count of the entities were unreachable at it. The neighbouring codecs of the
					 *          framework carried this overload, while the markup did not
					 *
					 * @param filename address of the file being parsed
					 * @param settings settings of the parsing of the text of the markup
					 * @return         sign of the success of the parsing
					 *
					 * \~
					 */
					bool load(const string & filename, const reader_t::settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст разметки
					 *
					 * @param format вид записи собираемого текста
					 * @return       текст разметки, пустой - записать значение не удалось
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a markup text
					 *
					 * @param format kind of the writing of the text being assembled
					 * @return       markup text, an empty one — the value could not be written
					 *
					 * \~
					 */
					string dump(const format_t format = format_t::COMPACT) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перезаписи значения в текст разметки с указанными настройками
					 *
					 * @details Отказ записи хотя бы одного узла отдаёт текст пустым: текст
					 *          усечённый, с узлом открытым и незакрытым, негоден вовсе, а выдавать
					 *          негодное молча кодек не вправе.
					 *
					 * @param settings настройки записи текста
					 * @return         текст разметки, пустой - записать значение не удалось
					 *
					 * \~english
					 * @brief Method of the rewriting of the value into a markup text with the indicated settings
					 *
					 * @details A refusal of the writing of at least one node gives away an empty text: a truncated
					 *          text with a node opened and unclosed is unusable at all, while the codec has no right
					 *          to give away an unusable thing silently.
					 *
					 * @param settings settings of the writing of the text
					 * @return         markup text, an empty one — the value could not be written
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
					 *
					 * @param filename address of the file being written
					 * @param format   kind of the writing of the text being assembled
					 * @return         sign of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename, const format_t format = format_t::COMPACT) const noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения в файл с заданными настройками сборки
					 *
					 * @details Настройки те же, что у выдачи текста, и слушаются одинаково:
					 *          выдача в строку и запись в файл суть две двери к ОДНОЙ сборке, и
					 *          настройки её обязаны быть достижимы с обеих.
					 *
					 * @warning Перегрузка эта заведена 01.09.2026: прежде дверь файловая
					 *          принимала лишь ВИД оформления, а прочие настройки сборки -
					 *          уклад экранирования, обращение с негодной последовательностью,
					 *          пределы записи - на ней были недостижимы. Замер: значение
					 *          «{"a":"/"}» укладом `SOLIDUS` выдавало в строку «{"a":"\\/"}»,
					 *          а в файл - «{"a":"/"}»
					 *
					 * @param filename адрес записываемого файла
					 * @param settings настройки сборки текста
					 * @return         признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a value into a file with the given settings of the assembly
					 *
					 * @details The settings are the same as at the issuance of a text and are obeyed alike: an
					 *          issuance into a string and a writing into a file are two doors to ONE assembling,
					 *          and its settings are bound to be reachable from both.
					 *
					 * @warning This overload was made on 2026-09-01: before, the file door took only a FORM of
					 *          the adornment, while the other settings of the assembling - the order of the
					 *          escaping, the handling of an unfit sequence, the limits of the writing - were
					 *          unreachable at it. A measurement: a value «{"a":"/"}» by the `SOLIDUS` order
					 *          issued into a string «{"a":"\\/"}» while into a file - «{"a":"/"}»
					 *
					 * @param filename address of the file being written
					 * @param settings settings of the assembly of the text
					 * @return         sign of the success of the writing
					 *
					 * \~
					 */
					bool save(const string & filename, const writer_t::settings_t & settings) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сличения значений
					 *
					 * @details Сличаются вид узла, имя его с пространством имён, содержимое,
					 *          свойства и вложенные узлы. Префикс пространства имён и связывания
					 *          сличению не подлежат: одно и то же имя, записанное разными префиксами,
					 *          есть одно и то же имя.
					 *
					 * @note Порядок свойств сличению не подлежит, а порядок вложенных узлов -
					 *       подлежит: свойства узла суть набор, а содержимое разметки
					 *       определено порядком своим
					 *
					 * @warning Равенство значений НЕ означает совпадения их записей. Замеры при
					 *          `==`, отвечающем истиной: свойства пишутся в своём порядке у каждого, а
					 *          префиксы пространств имён сохраняются каждый свой. Сличение отвечает о
					 *          ЗНАЧЕНИИ, а запись хранит ОФОРМЛЕНИЕ, и решать по сличению, будут ли
					 *          тексты одинаковы, нельзя. Так же поступает и сличение значения JSON
					 *
					 * @param value сличаемое значение
					 * @return      признак совпадения значений
					 *
					 * \~english
					 * @brief Method of the comparison of the values
					 *
					 * @details The kind of the node, its name with the namespace, the content,
					 *          the properties and the nested nodes are compared. The prefix of the namespace and the bindings
					 *          are not subject to the comparison: one and the same name written by different prefixes
					 *          is one and the same name.
					 *
					 * @note The order of the properties is not subject to the comparison, while the order of the nested nodes
					 *       is: the properties of a node are a set, while the content of a markup
					 *       is defined by its order
					 *
					 * @warning The equality of the values does NOT mean the coincidence of their records. Measurements with
					 *          `==` answering true: the attributes are written in their own order for each, while
					 *          the prefixes of the namespaces are preserved each its own. The comparison answers about the
					 *          VALUE, while the writing keeps the FORM, and it is not permissible to decide by the comparison whether the
					 *          texts will be identical. The comparison of a JSON value acts the same way
					 *
					 * @param value value being compared
					 * @return      sign of the coincidence of the values
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
					 *
					 * @param value value being compared
					 * @return      sign of the mismatch of the values
					 *
					 * \~
					 */
					bool operator != (const Value & value) const noexcept;
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
					 *
					 * @param value value being moved
					 * @return      reference to the current value
					 *
					 * \~
					 */
					Value & operator = (Value && value) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @details Вид этот и есть общий у всех кодеков рамки: работа с файловой
					 *          системой требует обоих объектов, и значение, заведённое без них, сохранять
					 *          себя в файл не умеет.
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @details This kind is the common one for all the codecs of the framework: a work with
					 *          a file system asks both objects, and a value created without them cannot save
					 *          itself into a file.
					 *
					 * \~
					 */
					explicit Value() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор узла указанного вида
					 *
					 * @param kind вид заводимого узла
					 *
					 * \~english
					 * @brief Constructor of a node of the indicated kind
					 *
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
					 *
					 * @param local  local name of the node being created
					 * @param uri    designation of the namespace of the node
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
					 *
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
					 *          пережить не может, а снятое им значение - может.
					 *
					 * @param node узел дерева разметки
					 *
					 * \~english
					 * @brief Constructor of the taking of a value from a node of a markup tree
					 *
					 * @details This constructor is the very bridge from the parsing to the ownership: a node
					 *          cannot outlive the tree, while a value taken by it — can.
					 *
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
					 *
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
					 *
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
					~Value() noexcept = default;
			} value_t;
			/**
			 * \~russian
			 * @brief Класс потоковой сборки владеющего значения XML
			 *
			 * @details Сборщик этот повторяет договор потока записи слово в слово: открыть
			 *          узел, объявить свойство, объявить связывание, записать содержимое, закрыть
			 *          узел, завершить. Оттого «пишу текстом» и «строю дерево» отличаются у
			 *          потребителя одной буквой, а не двумя разными договорами.
			 *
			 * \~english
			 * @brief Streaming construction class for an XML value that holds ownership
			 *
			 * @details This builder repeats the contract of the writing stream word for word: to open
			 *          a node, to declare a property, to declare a binding, to write a content, to close
			 *          a node, to finish. Whereby "I write a text" and "I build a tree" differ for
			 *          a consumer by one letter rather than by two different contracts.
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Builder {
				private:
					// Собираемое значение
					value_t _result;
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
					 *
					 * @param value node being placed
					 * @return      pointer to the placed node, a zero — the placement has failed
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
					 *
					 * @param local  local name of the node being opened
					 * @param uri    designation of the namespace of the node
					 * @param prefix prefix of the namespace of the node
					 * @return       sign of the success of the opening
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
					 *
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
					 *
					 * @param local  local name of the property
					 * @param value  value of the property
					 * @param uri    designation of the namespace of the property
					 * @param prefix prefix of the namespace of the property
					 * @return       sign of the success of the declaration
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
					 *
					 * @param prefix prefix being declared, an empty one — a declaration by default
					 * @param uri    designation of the namespace
					 * @return       sign of the success of the declaration
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
					 *
					 * @param text content being written
					 * @return     sign of the success of the writing
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
					 *
					 * @param text content being written
					 * @return     sign of the success of the writing
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
					 *
					 * @param text content being written
					 * @return     sign of the success of the writing
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
					 *
					 * @param target target of the processing instruction
					 * @param text   content of the processing instruction
					 * @return       sign of the success of the writing
					 *
					 * \~
					 */
					bool processing(const string & target, const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи готового узла
					 *
					 * @details Метод этот вставляет в собираемое дерево целое поддерево,
					 *          собранное где-то ещё.
					 *
					 * @param value записываемый узел
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the writing of a ready node
					 *
					 * @details This method inserts into the tree being assembled a whole subtree
					 *          assembled somewhere else.
					 *
					 * @param value node being written
					 * @return      sign of the success of the writing
					 *
					 * \~
					 */
					bool value(const value_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения текущей глубины вложенности
					 *
					 * @return количество открытых и ещё не закрытых узлов
					 *
					 * \~english
					 * @brief Method of the extraction of the current depth of the nesting
					 *
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
					 *          не может, а отказ здесь оставил бы потребителя вовсе без итога.
					 *
					 * @note Собранное отдаётся узлом разметки, коль скоро он у сборки один, и
					 *       узлом корневым, коль скоро их несколько: разметка одного корня
					 *       требует, а сборщик волен собрать и примечание перед ним
					 *
					 * @return собранное значение
					 *
					 * \~english
					 * @brief Method of the finishing of the assembly and of the taking away of the assembled value
					 *
					 * @details The unclosed nodes are closed by themselves: a streaming assembly cannot be
					 *          interrupted, while a refusal here would leave a consumer entirely without a result.
					 *
					 * @note What has been assembled is given away as a markup node if the assembly has one of them, and
					 *       as a root node if there are several: a markup requires one root, while
					 *       a builder is free to assemble a comment before it as well
					 *
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
					explicit Builder() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Builder() noexcept = default;
			} builder_t;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"
