/**
 * @file bridge.hpp
 * @date 2026-09-02
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
 * @brief Заголовочный файл моста между контейнером ABC и текстовыми кодеками
 *
 * \~english
 * @brief Header file of the bridge between the container ABC and the text codecs
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "ini/ini.hpp"
#include "numeric.hpp"
#include "xml/xml.hpp"
#include "abc/value.hpp"
#include "json/json.hpp"
#include "yaml/yaml.hpp"
#include "toml/toml.hpp"

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/macro/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * STRICT и TEXT у MS Windows. Имена снимаются лишь на время объявлений -
 * возврат в конце файла
 *
 * @warning Препроцессор области видимости не разбирает и заменяет имя даже внутри
 *          «enum class»: объявление обращается в негодное, и сборка встаёт. Замерено
 *          16.09.2026 щупом, подавшим те же макросы ключами собирателя
 *
 */
#include "../sys/macro/suppress.hpp"

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
		 * @brief Мост между контейнером ABC и текстовыми кодеками
		 *
		 * @details Переводит дерево значений контейнера ABC в запись любого из текстовых
		 *          кодеков и обратно. Осью перевода служит именно ABC: система видов его вмещает
		 *          виды всех текстовых кодеков разом, оттого перевод между двумя текстовыми
		 *          кодеками идёт через одно промежуточное дерево, а не через пятнадцать пар.
		 *
		 * @par Намеренные решения
		 *
		 *      Перечисленное ниже не является пробелом реализации: это очерченные границы
		 *      задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Сужение выдаётся отказом, а не молчаливой порчей.** Виды ABC, записи
		 *     кодека неведомые, обращаются по правилу, настройками заданному: либо в
		 *     последовательность знаков, либо в отказ с кодом. Молчаливая же порча
		 *     оставляла бы потребителя с записью, которая разбирается, но означает иное
		 *
		 * @li **Перевод ведётся обходом дерева, а не разбором записи.** Дерево ABC
		 *     подаётся писателю кодека событиями, и второго представления по дороге не
		 *     заводится вовсе
		 *
		 * @li **Дословного совпадения записи перевод не обещает - обещает значение.**
		 *     Запись «1e2» выдаётся как «100.0», «1.50» как «1.5», лишнее экранирование
		 *     снимается, а суррогатная пара обращается самим знаком. Это записанный
		 *     договор кодеков, а не изъян моста: разбор хранит число, а не знаки его.
		 *     Обратимость закреплена сличением ДЕРЕВЬЕВ, а не текстов
		 *
		 * @li **Число сверх родных видов выдаётся последовательностью знаков.** Вид
		 *     `EXTENDED` записи JSON укладывается в ABC записью числа знаками, ибо
		 *     перевод числа в родной вид терял бы разряды. Запись числа берётся ходом
		 *     `raw()`, а НЕ `text()`: последний выдаёт содержимое лишь у строковых узлов.
		 *     Причина знаков лежит не у ABC - ход `Value::decimal` вид этот собрать
		 *     позволяет, - а у самого JSON: величину числа он держит записью знаков, а
		 *     не октетами, и накормить `decimal` мосту нечем, не разбирая десятичную
		 *     запись произвольной длины самому
		 *
		 * @li **Пустой узел разметки укладывается по настройке.** Узел `<br/>` выражает
		 *     то, чего у прочих форматов нет вовсе - наличие без значения, - и согласия в
		 *     мире нет: Parker кладёт пустое значение, xml2js пустую запись, Badgerfish
		 *     пустое отображение, средства настройки логическую истину. Правило вынесено
		 *     настройкою `empty`, умолчанием взята истина: у настроек узел этот значит
		 *     «включено»
		 *
		 * @li **Перечень безымянный несёт пометку, а не обнос по договорённости.**
		 *     Перечень у разметки выражается ПОВТОРОМ одноимённых узлов, и повтор годен
		 *     лишь при двух звеньях и более, да и то полем отображения. Перечень верхнего
		 *     уровня, перечень внутри перечня и перечень из одного звена повтора не дают
		 *     вовсе - им ставится свойство `array`, обратным чтением снимаемое. Догадка по
		 *     виду записи здесь негодна: отображение с полем `item` неотличимо от перечня
		 *     обносов, и всякая догадка портила бы одно ради другого
		 *
		 * @li **Имя корня безымянному дереву даётся настройкою.** У документа разметки
		 *     ровно один корень, и безымянным он быть не может; перечень же и дерево ABC
		 *     корня именованного не имеют. Оттого круг перевода замыкается СО ВТОРОГО
		 *     прохода: первый даёт имя, дальнейшие неподвижны
		 *
		 * @li **Имя поля, разметке негодное, правится приставкой.** Имя узла с ведущей
		 *     цифрой стандарт XML 1.0 запрещает, а у отображения оно законно и приходит
		 *     из записи JSON: поле `3` получает приставку `Item`. Круг на нём НЕ
		 *     замыкается - обратное чтение отдаёт `Item3`, - и замкнуть его нечем
		 *
		 * @li **Содержимое смешанное теряет текст узла.** Запись `<a>текст<b/></a>`
		 *     укладывается отображением с одним лишь полем `b`: у отображения текст лечь
		 *     может только полем, а поле стало бы неотличимо от потомка с тем же именем.
		 *     Правило то же, что у эталонного моста
		 *
		 * \~english
		 * @brief Bridge between the container ABC and the text codecs
		 *
		 * @details Translates a tree of the values of the container ABC into a record of any of the text
		 *          codecs and back. Precisely ABC serves as the axis of the translation: its system of the kinds encompasses
		 *          the kinds of all the text codecs at once, whereby a translation between two text
		 *          codecs goes through one intermediate tree rather than through fifteen pairs.
		 *
		 * @par Intentional decisions
		 *
		 *      The listed below is not a gap of the implementation: these are the outlined
		 *      borders of the task, and every decision is secured by a check test
		 *
		 * @li **The narrowing is issued as a refusal, not as a silent corruption.** The
		 *     kinds of ABC unknown to the record of a codec are treated by the rule given
		 *     with the settings: either into a sequence of characters, or into a refusal
		 *     with a code. A silent corruption would leave the consumer with a record that
		 *     parses but means something else
		 *
		 * @li **The translation is carried out by the traversal of the tree, not by the
		 *     parsing of a record.** The tree of ABC is fed to the writer of the codec with
		 *     events, and a second representation is not created along the way at all
		 *
		 * @li **The translation promises the value, not a literal coincidence of the
		 *     records.** The record «1e2» is issued as «100.0», «1.50» as «1.5», redundant
		 *     escaping is removed, and a surrogate pair turns into the sign itself. This is
		 *     the recorded contract of the codecs, not a flaw of the bridge: the parsing
		 *     keeps the number, not the characters of it. The reversibility is secured by
		 *     the comparison of the TREES, not of the texts
		 *
		 * @li **A number beyond the native kinds is issued as a sequence of
		 *     characters.** The kind `EXTENDED` of the record of JSON is laid into ABC with
		 *     a record of the number by characters, since the translation of the number
		 *     into a native kind would lose the digits. The record of the number is taken
		 *     with the method `raw()`, NOT `text()`: the latter yields the content only of
		 *     string nodes. The cause of the characters lies not at ABC - the method
		 *     `Value::decimal` allows to assemble this kind - but at the very JSON: it
		 *     keeps the magnitude of a number as a record of characters rather than
		 *     octets, and there is nothing to feed `decimal` for the bridge without
		 *     parsing an arbitrary-length decimal record by itself
		 *
		 * @li **An empty markup node is laid out by the setting.** The node `<br/>`
		 *     expresses what the other formats do not have at all - a presence without a
		 *     value - and there is no consent in the world: Parker puts an empty value,
		 *     xml2js an empty record, Badgerfish an empty mapping, and the means of
		 *     configuration a logical truth. The rule is placed into the setting `empty`,
		 *     with the truth taken as the default: for the settings the node means «enabled»
		 *
		 * @li **A nameless list carries a mark, not a wrapper by guessing.** A list in
		 *     the markup is expressed by the REPETITION of same-named nodes, and a
		 *     repetition is fit only with two links and more, and even then as a field of
		 *     a mapping. A top-level list, a list inside a list and a list of one link
		 *     give no repetition at all - the property `array` is set for them, taken off
		 *     by the backward reading. A guess by the look of the record is useless here:
		 *     a mapping with a field `item` is indistinguishable from a list of wrappers,
		 *     and any guess would spoil one for the sake of the other
		 *
		 * @li **The name of the root for a nameless tree is given by the setting.** A
		 *     markup document has exactly one root, and it cannot be nameless; while a
		 *     list and the tree of ABC have no named root. Hence the translation loop
		 *     closes FROM THE SECOND pass: the first one gives the name, the further ones
		 *     stay immovable
		 *
		 * @li **A field name unfit for the markup is fixed with a prefix.** The XML 1.0
		 *     standard forbids a node name with a leading digit, while for a mapping it is
		 *     lawful and comes from the record of JSON: the field `3` receives the prefix
		 *     `Item`. The loop does NOT close on it - the backward reading gives `Item3`,
		 *     - and there is nothing to close it with
		 *
		 * @li **Mixed content loses the text of the node.** The record
		 *     `<a>text<b/></a>` is laid out as a mapping with the single field `b`: for a
		 *     mapping the text can lie only as a field, and a field would become
		 *     indistinguishable from a descendant with the same name. The rule is the same
		 *     as at the reference bridge
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Bridge {
			public:
				/**
				 * \~russian
				 * @brief Коды отказов перевода
				 *
				 * \~english
				 * @brief Error codes of the translation
				 *
				 * \~
				 */
				enum class error_t : uint8_t {
					NONE        = 0x00, // Отказа не произошло
					PARSING     = 0x01, // Разбор записи кодеком окончился отказом
					WRITING     = 0x02, // Запись кодеком окончилась отказом
					UNSUPPORTED = 0x03, // Записи кодека вид значения неведом
					DEEP_TREE   = 0x04, // Глубина дерева превысила предел перевода
					/**
					 * \~russian
					 * Устройство записи кодека дерева не вмещает
					 *
					 * @details Код этот отделён от `UNSUPPORTED` намеренно: тот
					 *          означает «вид значения записи неведом», и чинится он сменою
					 *          ВИДА, а этот означает «устройство записи такого не вмещает», и
					 *          чинится он сменою УСТРОЙСТВА. Запись INI, к примеру, глубже
					 *          раздела со свойством не идёт, и перечень отображений в неё не
					 *          ложится ни при каком виде значений.
					 *
					 * @note Различение это не украшение: потребитель, увидевший один
					 *       код на обе беды, не знает, что ему править - значение либо
					 *       дерево
					 *
					 * \~english
					 * The device of the record of a codec cannot hold the tree
					 *
					 * @details This code is deliberately separated from `UNSUPPORTED`: that one
					 *          means «the kind of the value is unknown to the record», and it is fixed
					 *          by changing the KIND, while this one means «the device of the record
					 *          cannot hold such a thing», and it is fixed by changing the DEVICE. The
					 *          record of INI, for example, goes no deeper than a section with a
					 *          property, and a list of mappings does not fit into it with any kind of values.
					 *
					 * @note The distinction is not an ornament: a consumer who sees one code
					 *       for both troubles does not know what to fix - the value or the
					 *       tree
					 *
					 * \~
					 */
					STRUCTURE = 0x05,
					/**
					 * \~russian
					 * Настройка перевода негодна
					 *
					 * @details Код этот отделён от `WRITING` намеренно: тот означает
					 *          «кодек записать не смог», и искать причину потребитель идёт в
					 *          ДЕРЕВЕ, а этот означает «негодна сама настройка», и чинится он
					 *          сменою НАСТРОЙКИ. Имя корня и имя звена перечня становятся у
					 *          разметки именами узлов, а имя узла бывает не всяким: пустое
					 *          имя либо имя со знаком, стандартом не допущенным, записи
					 *          негодно вовсе.
					 *
					 * @note Прежде негодная настройка уходила прямо кодеку, и тот
					 *       отвечал отказом записи. Потребитель получал `WRITING` и
					 *       шёл искать изъян в своём дереве, тогда как дерево было
					 *       верным, а негодна была настройка. Замерено 15.09.2026
					 *
					 * \~english
					 * A setting of the translation is invalid
					 *
					 * @details This code is deliberately separated from `WRITING`: that one means
					 *          «the codec could not write», and the consumer goes looking for the cause in the
					 *          TREE, while this one means «the setting itself is invalid», and it is fixed
					 *          by changing the SETTING.
					 *
					 * @note Before, an invalid setting went straight to the codec, and that one
					 *       answered with a refusal of the record. The consumer received
					 *       `WRITING` and went looking for a flaw in his tree, while the tree
					 *       was correct, and the setting was the invalid one. Measured 15.09.2026
					 *
					 * \~
					 */
					SETTINGS = 0x06
				};
				/**
				 * \~russian
				 * @brief Вид записи, с которым мост переводит дерево
				 *
				 * @details Кодек CSV в перечень не входит намеренно: содержимое его
				 *          таблично, а не древовидно, и настройками оно не бывает.
				 *
				 * \~english
				 * @brief Kind of a record with which the bridge translates a tree
				 *
				 * @details The codec CSV is deliberately not in the list: its content is
				 *          tabular, not tree-shaped, and it is never settings.
				 *
				 * \~
				 */
				enum class format_t : uint8_t {
					NONE = 0x00, // Вид записи не задан
					ABC  = 0x01, // Запись контейнера ABC как она есть
					JSON = 0x02, // Запись JSON
					XML  = 0x03, // Запись XML
					YAML = 0x04, // Запись YAML
					TOML = 0x05, // Запись TOML
					INI  = 0x06  // Запись INI
				};
				/**
				 * \~russian
				 * @brief Правила обращения с видами, записи кодека неведомыми
				 *
				 *
				 * \~english
				 * @brief Rules of the treatment of the kinds unknown to the record of a codec
				 *
				 * \~
				 */
				enum class narrow_t : uint8_t {
					/**
					 * \~russian
					 * Отвечать отказом
					 *
					 * @note Код отказа берётся ПО ПРИЧИНЕ, а не один и тот же:
					 *       `UNSUPPORTED`, когда записи неведом сам вид значения, и
					 *       `STRUCTURE`, когда вид ей ведом, а устройство записи
					 *       значения такого не вмещает - перечень пустой у разметки и
					 *       у INI, к примеру, выразить нечем, тогда как перечень
					 *       непустой те же записи выражают повтором. Различение это
					 *       описано у самих кодов, и правило сужения его не отменяет
					 *
					 * \~english
					 * Answer with a refusal
					 *
					 * @note The code of the refusal is taken BY THE CAUSE, not one and the
					 *       same: `UNSUPPORTED`, when the kind of the value itself is unknown
					 *       to the record, and `STRUCTURE`, when the kind is known to it, but
					 *       the device of the record cannot hold such a value - an empty list
					 *       in the markup and in INI, for example, cannot be expressed at all,
					 *       while a non-empty list the same records express with a repetition.
					 *       The distinction is described at the codes themselves, and the rule
					 *       of the narrowing does not cancel it
					 *
					 * \~
					 */
					STRICT = 0x00,
					TEXT   = 0x01, // Обращать в последовательность знаков
					SKIP   = 0x02  // Пропускать значение вовсе
				};
				/**
				 * \~russian
				 * @brief Правило укладки пустого узла разметки
				 *
				 * @details Узел без содержимого и свойств (`<br/>`) выражает у разметки
				 *          то, чего у прочих форматов нет вовсе - НАЛИЧИЕ без значения. Согласия
				 *          в мире нет: Parker кладёт `null`, xmltodict - `None`, xml2js и
				 *          Newtonsoft - пустую запись, Badgerfish - пустое отображение, а средства
				 *          настройки - логическую истину. Оттого правило это и вынесено настройкой.
				 *
				 * @note Умолчанием взята ИСТИНА: у средств настройки узел этот означает
				 *       «включено», и прочтение это привычно обиходу
				 *
				 * \~english
				 * @brief Rule for laying out an empty markup node
				 *
				 * @details A node without content and properties (`<br/>`) expresses in
				 *          the markup what the other formats do not have at all - a PRESENCE
				 *          without a value. There is no consent in the world: Parker puts `null`,
				 *          xmltodict - `None`, xml2js and Newtonsoft - an empty record, Badgerfish
				 *          - an empty mapping, and the means of configuration - a logical truth.
				 *          Hence this rule is placed into the setting.
				 *
				 * @note The TRUTH is taken as the default: for the means of configuration
				 *       the node means «enabled», and this reading is habitual to the
				 *       everyday use
				 *
				 * \~
				 */
				enum class empty_t : uint8_t {
					BOOLEAN = 0x00, // Укладывать логическою истиной
					NONE    = 0x01, // Укладывать пустым значением
					STRING  = 0x02, // Укладывать пустою последовательностью знаков
					OBJECT  = 0x03  // Укладывать пустым отображением
				};
				/**
				 * \~russian
				 * @brief Структура настройки перевода
				 *
				 *
				 * \~english
				 * @brief Translation settings structure
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					/**
					 * \~russian
					 * Признак вывода вида значения из его записи
					 *
					 * @warning Настройка эта нужна видам записи, своей системы видов не
					 *          имеющим - разметке прежде всего: у неё всякое содержимое
					 *          есть текст, и `<port>8080</port>` без вывода вида лёг бы
					 *          строкою, а не числом. Вывод ведётся цепью: целое, дробное,
					 *          логическое, иначе строка
					 * 
					 * \~english
					 * Sign of the inference of the kind of a value from its record
					 * 
					 * @warning This setting is needed by the kinds of a record having no system of kinds -
					 *          to the markup first of all: every content of it
					 *          is text, and `<port>8080</port>` without the inference of the kind would lie
					 *          as a string rather than as a number. The inference goes by a chain: integer, floating,
					 *          logical, otherwise string
					 * 
					 * \~
					 */
					bool typed;
					/**
					 * \~russian
					 * Предельная глубина дерева перевода
					 *
					 * @warning Нуль допускает только корень без потомков, а не снимает предел:
					 *          корень имеет уровень нуль, его дети — уровень один.
					 *          Договор этот РАЗНИТСЯ с пределами разборщика
					 *          доводов запуска, где нуль предел снимает, - и разнится
					 *          НАМЕРЕННО: ограда глубины стоит против записи недоброй,
					 *          уводящей перевод в возвратность без дна, и снимать её
					 *          настройкой значило бы дать ту самую беду, от коей она
					 *          поставлена. Замерено 16.09.2026 аудитом, закреплено
					 *          проверкой `CodecBridge.TheDepthOfZeroForbidsRatherThanUnlocks`
					 *
					 * @note Счёт уровней таков: дерево `{"a":{"b":{"c":1}}}` требует
					 *       глубины ТРИ, и обе дороги - выдача с чтением - судят его
					 *       одинаково
					 *
					 * \~english
					 * Maximum depth of a tree of the translation
					 *
					 * @warning Zero permits only a root without children rather than removing the limit:
					 *          a root has level zero, its children are level one.
					 *          This contract DIFFERS from the limits of the parser
					 *          of the launch arguments, where zero removes the limit, - and it differs
					 *          DELIBERATELY: the depth guard stands against a malicious record,
					 *          leading the translation into bottomless recursion, and removing it
					 *          by a setting would mean giving the very trouble it is
					 *          set against. Measured 16.09.2026 by an audit, pinned down
					 *          by the check `CodecBridge.TheDepthOfZeroForbidsRatherThanUnlocks`
					 *
					 * @note The count of the levels is such: the tree `{"a":{"b":{"c":1}}}` requires
					 *       a depth of THREE, and both roads - the output with the reading - judge it
					 *       the same way
					 *
					 * \~
					 */
					uint32_t depth;
					// Правило укладки пустого узла разметки
					empty_t empty;
					// Правило обращения с видами, записи кодека неведомыми
					narrow_t narrow;
					/**
					 * \~russian
					 * Имя поля, под которым ложится текст узла с атрибутами
					 *
					 * @warning Имя это нужно оттого, что узел разметки несёт РАЗОМ и
					 *          атрибуты, и собственный текст, а отображение выразить это
					 *          может лишь полем. При столкновении с одноимённым атрибутом
					 *          имя получает приставку `_` столько раз, сколько нужно
					 * 
					 * \~english
					 * Name of the field under which the text of a node with the attributes is laid
					 * 
					 * @warning This name is needed because a markup node carries AT ONCE both
					 *          the attributes and its own text, while a mapping can express it
					 *          only by a field. Upon a collision with an attribute of the same name
					 *          the name gets the prefix `_` as many times as needed
					 * 
					 * \~
					 */
					string text;
					/**
					 * \~russian
					 * Имя корневого узла собираемой записи разметки
					 *
					 * @warning Настройка эта нужна одному лишь виду XML и зашита быть не
					 *          может: стандарт XML 1.0 требует у документа РОВНО ОДИН
					 *          корневой элемент, а у дерева контейнера ABC корень
					 *          безымянен. Имя взять неоткуда, кроме как у того, кто
					 *          пишет, и молчаливый выбор моста был бы выдумкой
					 * 
					 * \~english
					 * Name of the root node of the record of markup being assembled
					 * 
					 * @warning This setting is needed by the one kind of XML alone and cannot be hardwired:
					 *          the standard XML 1.0 requires EXACTLY ONE root element at a document,
					 *          while at the tree of the container ABC the root
					 *          has no name. There is nowhere to take the name from but from the one who
					 *          writes, and a silent choice of the bridge would be a made-up thing
					 * 
					 * \~
					 */
					string root;
					/**
					 * \~russian
					 * Имя узла, обносящего перечень, во вместилище вложенный
					 *
					 * @warning Имя это нужно оттого, что перечень у разметки ИМЕНИ НЕ
					 *          ИМЕЕТ вовсе: перечень выражается повтором одноимённых
					 *          узлов, и перечень внутри перечня выразить нечем -
					 *          одноимённые узлы легли бы вперемешку и границы вложенного
					 *          перечня пропали бы. Оттого всякое звено вложенного перечня
					 *          обносится узлом с этим именем
					 * 
					 * \~english
					 * Name of the node wrapping a list nested into a container
					 * 
					 * @warning This name is needed because a list in the markup HAS NO NAME
					 *          at all: a list is expressed by repeating same-named nodes,
					 *          and a list inside a list cannot be expressed in any way -
					 *          same-named nodes would lie mixed up and the bounds of the nested
					 *          list would be lost. Hence every link of a nested list
					 *          is wrapped in a node with this name
					 * 
					 * \~
					 */
					string item;
					/**
					 * \~russian
					 * Имя свойства, перечень от отображения отличающего
					 *
					 * @warning Свойство это нужно оттого, что перечень у разметки ИМЕНИ
					 *          НЕ ИМЕЕТ: он выражается повтором одноимённых узлов, и
					 *          обратное чтение узнаёт его лишь по повтору. Перечень из
					 *          ОДНОГО звена повтора не даёт и возвращается значением, а
					 *          перечень безымянный - верхнего уровня либо вложенный в
					 *          другой перечень - возвращается отображением: круг перевода
					 *          не замыкается вовсе. Свойство это ставится РОВНО на те
					 *          узлы, где повтор перечня не выражает, и обратным чтением
					 *          снимается
					 *
					 * @note Пустая запись имени запрещает пометку вовсе: записи выходят
					 *       чище, а круг перевода на безымянных перечнях размыкается
					 *
					 * @note Запрет пометки делает НЕВЫРАЗИМЫМ и перечень ПУСТОЙ: повторять
					 *       там нечего, и узла в записи не появилось бы вовсе. Случай этот
					 *       уходит правилом сужения наравне с прочими невыразимыми -
					 *       отказом, записью знаками либо пропуском, - а не молчаливою
					 *       пропажей поля. То же и у записи INI, где перечень есть череда
					 *       одноимённых свойств, а череда пустая от отсутствия свойства
					 *       неотличима
					 * 
					 * \~english
					 * Name of the property distinguishing a list from a mapping
					 * 
					 * @warning This property is needed because a list in the markup HAS NO NAME:
					 *          it is expressed by repeating same-named nodes, and
					 *          the reverse reading tells it only by the repetition. A list of
					 *          ONE link gives no repetition and is returned as a value, while
					 *          an unnamed list - of the top level or nested into
					 *          another list - is returned as a mapping: the translation round
					 *          does not close at all. This property is put on EXACTLY those
					 *          nodes where the repetition of a list expresses nothing, and the reverse reading
					 *          removes it
					 * 
					 * @note An empty record of the name forbids the marking at all: the records come out
					 *       cleaner, while the translation round on the unnamed lists breaks open
					 * 
					 * @note Forbidding the marking makes an EMPTY list INEXPRESSIBLE TOO: there is nothing
					 *       to repeat there, and no node would appear in the record at all. This case
					 *       goes by the narrowing rule on a par with the rest of the inexpressible -
					 *       by a refusal, a record with signs or a skip, - rather than by a silent
					 *       loss of the field. The same is with the record of INI, where a list is
					 *       a run of same-named properties, while an empty run from the absence of a property
					 *       is indistinguishable
					 * 
					 * \~
					 */
					string array;
					/**
					 * \~russian
					 * Вид оформления собираемой записи
					 *
					 * @warning Настройка эта - МОСТА, а не одного кодека JSON, и слушают её
					 *          четыре дороги из пяти: у записи JSON и у разметки счёт
					 *          оформления свой и тот же самый, а у записей TOML и INI
					 *          плотность выражается снятием украшающего отступа перед
					 *          парами таблицы либо свойствами раздела. Прежде настройку
					 *          слушала ОДНА дорога JSON, а прочие звали сборку умолчанием,
					 *          и потребитель, затребовавший запись плотную, получал её
					 *          нарядной без всякого ответа. Замерено 15.09.2026 аудитом,
					 *          закреплено проверкой `CodecBridge.TheFormatSettingIsHeardByEveryRoadThatHasOne`
					 *
					 * @note Запись YAML настройке этой НЕ подчиняется, и это не пробел:
					 *       построение её блочное задаётся самим отступом, ширина коего
					 *       нуля не принимает вовсе по описанию наречия. Плотная запись
					 *       YAML есть построение ПОТОЧНОЕ - иное письмо, а не снятое
					 *       украшение
					 * 
					 * \~english
					 * Kind of the formatting of the record being assembled
					 * 
					 * @warning This setting belongs to the BRIDGE, not to the codec JSON alone, and it is heard
					 *          by four roads out of five: at the record of JSON and at the markup the count
					 *          of the formatting is its own and the very same, while at the records of TOML and INI
					 *          the density is expressed by removing the decorating indent before
					 *          the pairs of a table or the properties of a section. Before, the setting
					 *          was heard by the ONE road of JSON, while the rest assembled with the default,
					 *          and the consumer requesting a dense record got it
					 *          ornate without any answer. Measured 15.09.2026 by an audit,
					 *          pinned down by the check `CodecBridge.TheFormatSettingIsHeardByEveryRoadThatHasOne`
					 * 
					 * @note The record of YAML does NOT obey this setting, and this is not a gap:
					 *       its block structure is set by the very indent, whose width
					 *       accepts no zero at all by the description of the dialect. A dense record
					 *       of YAML is a STREAMING structure - another writing, not a removed
					 *       decoration
					 * 
					 * \~
					 */
					json::format_t format;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Settings() noexcept;
				} settings_t;
			private:
				/**
				 * \~russian
				 * Признак пропуска значения правилом сужения
				 *
				 * @details Правило `SKIP` велит пропускать значение ВОВСЕ, а ходы подачи
				 *          отвечают лишь успехом либо отказом, и «пропущено» от «подано» ими не
				 *          отличалось: подающий выходил успехом, ничего не тронув, а вкладывающий
				 *          клал НЕТРОНУТОЕ значение полем. Оттого пропуск оборачивался укладкою
				 *          пустого значения у четырёх дорог и отказом записи целиком у пятой -
				 *          настройка не исполняла обещанного НИ НА ОДНОЙ.
				 *
				 * @note Признак ставится полем, а не третьим видом ответа: ходов подачи
				 *       пять, зовутся они друг из друга, и смена вида ответа тронула бы
				 *       всякое место их вызова. Поле же читается сразу после вызова, там
				 *       же, где и код отказа
				 *
				 * @warning Признак этот СБРАСЫВАЕТСЯ подающим ходом при входе и ставится
				 *          лишь на своём пропуске: вместилище само пропущенным не бывает,
				 *          и потому ветви вместилищ сбрасывают его перед своим выходом -
				 *          иначе пропуск ПОСЛЕДНЕГО потомка выдавался бы за пропуск
				 *          всего вместилища
				 *
				 * \~english
				 * Sign of the skipping of a value by the narrowing rule
				 *
				 * @details The rule `SKIP` orders to skip a value ENTIRELY, while the methods of the submission
				 *          answer with success or refusal alone, and "skipped" was not told from
				 *          "submitted" by them: the submitter came out having succeeded without touching
				 *          anything, while the wrapper laid the UNTOUCHED value as a field. Hence the skip
				 *          was wrapped in the laying of an empty value at four roads and in a refusal of the
				 *          whole record at the fifth - the setting kept its promise on NOT A SINGLE ONE.
				 *
				 * @note The sign is set by a field rather than by a third kind of the answer: there are
				 *       five methods of the submission, they call one another, and a change of the kind
				 *       of the answer would touch every place of their call. The field is read right
				 *       after the call, in the same place where the error code is
				 *
				 * @warning This sign is RESET by the submitting method upon the entry and is set
				 *          only on its own skip: a container is never skipped itself,
				 *          and therefore the container branches reset it before their exit -
				 *          otherwise the skip of the LAST child would be issued as the skip
				 *          of the whole container
				 *
				 * \~
				 */
				bool _skipped;
			private:
				// Код отказа последнего перевода
				error_t _error;
			private:
				// Настройки перевода
				settings_t _settings;
			private:
				/**
				 * \~russian
				 * @brief Метод упреждающего спроса пропуска значения правилом сужения
				 *
				 * @details Ход этот нужен ОДНОЙ лишь дороге JSON: писатель её поточный,
				 *          имя поля уходит в запись ДО значения, и пропустить значение после записи
				 *          имени нельзя вовсе. Прочие четыре дороги собирают владеющее значение и
				 *          кладут его полем СЛЕДОМ, и им довольно признака `_skipped`.
				 *
				 * @param value значение контейнера ABC
				 * @param depth глубина обхода дерева
				 * @return      признак пропуска значения правилом сужения
				 *
				 * \~english
				 * @brief Method of the look-ahead inquiry of the skipping of a value by the narrowing rule
				 *
				 * @details This method is needed by the ONE road of JSON alone: its writer is streaming,
				 *          the field name goes into the record BEFORE the value, and skipping the value after
				 *          the name is written is impossible at all. The other four roads assemble an owning
				 *          value and lay it as a field FORTHWITH, and the sign `_skipped` is enough for them.
				 *
				 * @param value value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return      sign of the skipping of the value by the narrowing rule
				 *
				 * \~
				 */
				bool narrows(const abc::value_t & value, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора отказа собирателя записи INI правилом сужения
				 *
				 * @details Наречие INI принимает не всякое имя и не всякое значение, и отказ
				 *          его есть отказ по НЕВЫРАЗИМОСТИ: разбирать его надлежит правилом сужения,
				 *          а не выдачей наружу кодом `WRITING`.
				 *
				 * @param name имя свойства, записи не принятого
				 * @return     результат разбора: истина, когда запись продолжима
				 *
				 * \~english
				 * @brief Method of the treatment of a refusal of the writer of the record INI by the narrowing rule
				 *
				 * @details The dialect INI accepts not every name and not every value, and a refusal
				 *          of it is a refusal of INEXPRESSIBILITY: it is to be treated by the narrowing rule,
				 *          rather than by issuing the code `WRITING` outward.
				 *
				 * @param name name of the property not accepted by the record
				 * @return     result of the treatment: true when the writing can continue
				 *
				 * \~
				 */
				bool narrowed(const string & name) noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки записи значения, разметке годной
				 *
				 * @details Разметка несёт в тексте не всякое кодовое значение, и значение,
				 *          ею невыразимое, ведётся правилом сужения. Спрос стоит у моста оттого, что
				 *          кодек отвечает отказом лишь при посадке собранного узла документом - то
				 *          есть после того, как всё дерево уже подано, и поле, отказ вызвавшее, уже
				 *          не отличить.
				 *
				 * @param value  значение контейнера ABC
				 * @param result собранная запись значения
				 * @return       результат сборки: истина, когда запись продолжима
				 *
				 * \~english
				 * @brief Method of the assembly of a record of a value admissible in the markup
				 *
				 * @details Markup carries in the text not every code value, and a value
				 *          inexpressible by it is led by the narrowing rule. The inquiry stands at the bridge because
				 *          the codec refuses only upon putting the assembled node by the document - that
				 *          is after the whole tree has already been submitted, and the field that caused
				 *          the refusal can no longer be told apart.
				 *
				 * @param value  value of the container ABC
				 * @param result assembled record of the value
				 * @return       result of the assembly: true when the writing can continue
				 *
				 * \~
				 */
				bool carryable(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC писателю JSON
				 *
				 * @param value  значение контейнера ABC
				 * @param writer писатель записи JSON
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to the writer of JSON
				 *
				 * @param value  value of the container ABC
				 * @param writer writer of a record of JSON
				 * @param depth  depth of the traversal of the tree
				 * @return       result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feed(const abc::value_t & value, json::writer_t & writer, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения JSON в значение ABC
				 *
				 * @param value  значение документа JSON
				 * @param result укладываемое значение контейнера ABC
				 * @param depth  глубина обхода дерева
				 * @return       результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of JSON into a value of ABC
				 *
				 * @param value  value of a document of JSON
				 * @param result laid value of the container ABC
				 * @param depth  depth of the traversal of the tree
				 * @return       result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorb(const json::Document::value_t & value, abc::value_t & result, const uint32_t depth) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод спроса о виде, всякой текстовой записи родном
				 *
				 * @details Родными считаются виды, какие всякий текстовый кодек выражает
				 *          сам: пустота, логическое значение, последовательность знаков, число
				 *          целое и дробное да отметка времени, числом записываемая. Прочие -
				 *          двоичные данные, опознаватель, десятичное с точным разрядом, целое
				 *          сверх родных видов и открытое расширение - записи неведомы, и обращение
				 *          с ними решает настройка сужения.
				 *
				 * @param value значение контейнера ABC
				 * @return      признак родного вида
				 *
				 * \~english
				 * @brief Method of the question about a kind native to every text record
				 *
				 * @details Native are the kinds that every text record expresses
				 *          by itself: emptiness, a logical value, a sequence of signs, a number
				 *          integer and floating and a timestamp recorded as a number. The rest -
				 *          binary data, an identifier, a decimal with the exact scale, an integer
				 *          beyond the native kinds and an open extension - are unknown to a record, and the
				 *          treatment of them is decided by the narrowing setting.
				 *
				 * @param value value of the container ABC
				 * @return      sign of a native kind
				 *
				 * \~
				 */
				[[nodiscard]] bool native(const abc::value_t & value) const noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись самого контейнера ABC
				 *
				 * @details Дорога эта не переводит ничего: запись контейнера собирается
				 *          им самим. Мост же ставит её вровень с прочими видами, дабы потребитель,
				 *          вид записи выбирающий настройкой, не знал о ABC особого хода.
				 *
				 * @warning Запись эта ДВОИЧНА и укладывается в строку октетами как есть
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of the container ABC itself
				 *
				 * @details This road translates nothing: the record of the container is assembled
				 *          by the container itself. The bridge puts it on a par with the other kinds, so that the
				 *          consumer choosing a kind of a record by the setting knows of no special method of ABC.
				 *
				 * @warning This record is BINARY and is laid into a string by the octets as they are
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeABC(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи контейнера ABC в дерево ABC
				 *
				 * @param text   запись контейнера ABC для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of the container ABC into a tree of ABC
				 *
				 * @param text   record of the container ABC for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeABC(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись JSON
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись JSON
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of JSON
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of JSON
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeJSON(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи JSON в дерево ABC
				 *
				 * @param text   запись JSON для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of JSON into a tree of ABC
				 *
				 * @param text   record of JSON for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeJSON(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия отменяющей записи со звена пути
				 *
				 * @details Звенья пути выдаются кодеками с отменяющими записями RFC 6901:
				 *          имя `a/b` записывается звеном `a~1b`, имя `a~b` - звеном `a~0b`.
				 *          Ход возвращает звену исходное имя.
				 *
				 * @param link звено пути с отменяющими записями
				 * @return     исходное имя потомка
				 *
				 * \~english
				 * @brief Method of the removal of the escaping from a link of a path
				 *
				 * @details Links of a path are given out by the codecs with the escaping of RFC 6901:
				 *          the name `a/b` is recorded as the link `a~1b`, the name `a~b` - as the link `a~0b`.
				 *          The method returns to the link its original name.
				 *
				 * @param link link of a path with the escaping
				 * @return     original name of the child
				 *
				 * \~
				 */
				[[nodiscard]] string unescape(const string & link) const noexcept;
				/**
				 * \~russian
				 * @brief Метод наложения отменяющей записи на имя потомка
				 *
				 * @details Ход обратный снятию: имя `a/b` обращается в звено `a~1b`,
				 *          имя `a~b` - в звено `a~0b`. Без него имя, косую черту
				 *          несущее, разошлось бы у кодека на два звена пути.
				 *
				 * @param name исходное имя потомка
				 * @return     звено пути с отменяющими записями
				 *
				 * \~english
				 * @brief Method of the applying of the escaping to a name of a child
				 *
				 * @details The method is the reverse of the removal: the name `a/b` is turned into the link `a~1b`,
				 *          the name `a~b` - into the link `a~0b`. Without it a name carrying
				 *          a slash would split at the codec into two links of a path.
				 *
				 * @param name original name of the child
				 * @return     link of a path with the escaping
				 *
				 * \~
				 */
				[[nodiscard]] string escape(const string & name) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи записи простого значения ABC
				 *
				 * @details Ход этот нужен видам записи, у которых своей системы видов
				 *          нет: значение выдаётся ТЕКСТОМ, каким оно записывается.
				 *
				 * @param value значение контейнера ABC
				 * @return      запись простого значения
				 *
				 * \~english
				 * @brief Method of getting the record of a simple value of ABC
				 *
				 * @details This method is needed by the kinds of a record having no system
				 *          of the kinds of their own: the value is given out as the TEXT by which it is recorded.
				 *
				 * @param value value of the container ABC
				 * @return      record of the simple value
				 *
				 * \~
				 */
				[[nodiscard]] string record(const abc::value_t & value) const noexcept;
				/**
				 * \~russian
				 * @brief Метод вывода значения ABC из его записи
				 *
				 * @details Вид выводится цепью: целое, дробное, логическое, иначе
				 *          последовательность знаков. Ход этот нужен видам записи, своей
				 *          системы видов не имеющим.
				 *
				 * @param text запись значения
				 * @return     выведенное значение контейнера ABC
				 *
				 * \~english
				 * @brief Method of the inference of a value of ABC from its record
				 *
				 * @details The kind is inferred by a chain: integer, floating, logical, otherwise
				 *          a sequence of signs. This method is needed by the kinds of a record having
				 *          no system of the kinds of their own.
				 *
				 * @param text record of the value
				 * @return     inferred value of the container ABC
				 *
				 * \~
				 */
				[[nodiscard]] abc::value_t infer(const string & text) const noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC документу YAML
				 *
				 * @param value  значение контейнера ABC
				 * @param result собираемое значение записи YAML
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to the document of YAML
				 *
				 * @param value  value of the container ABC
				 * @param result assembled value of a record of YAML
				 * @param depth  depth of the traversal of the tree
				 * @return       result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feedYAML(const abc::value_t & value, yaml::Value & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись YAML
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись YAML
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of YAML
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of YAML
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeYAML(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC значению TOML
				 *
				 * @param value  значение контейнера ABC
				 * @param result собираемое значение записи TOML
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to a value of TOML
				 *
				 * @param value  value of the container ABC
				 * @param result assembled value of a record of TOML
				 * @param depth  depth of the traversal of the tree
				 * @return       result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feedTOML(const abc::value_t & value, toml::Value & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись TOML
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись TOML
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of TOML
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of TOML
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeTOML(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC значению XML
				 *
				 * @param value  значение контейнера ABC
				 * @param result собираемое значение записи XML
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to a value of XML
				 *
				 * @param value  value of the container ABC
				 * @param result assembled value of a record of XML
				 * @param depth  depth of the traversal of the tree
				 * @return       result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feedXML(const abc::value_t & value, xml::Value & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись XML
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись XML
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of XML
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of XML
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeXML(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод записи значения свойством раздела INI
				 *
				 * @param value    значение контейнера ABC
				 * @param document собираемый документ записи INI
				 * @param name     имя свойства
				 * @param section  имя раздела
				 * @return         результат записи
				 *
				 * \~english
				 * @brief Method of the writing of a value as a property of a section of INI
				 *
				 * @param value    value of the container ABC
				 * @param document assembled document of a record of INI
				 * @param name     name of the property
				 * @param section  name of the section
				 * @return         result of the writing
				 *
				 * \~
				 */
				[[nodiscard]] bool feedINI(const abc::value_t & value, ini::document_t & document, const string & name, const string & section, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись INI
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись INI
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of INI
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result assembled record of INI
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeINI(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения YAML в значение ABC
				 *
				 * @param document документ записи YAML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of YAML into a value of ABC
				 *
				 * @param document document of a record of YAML
				 * @param path     path to the value being laid
				 * @param result   laid value of the container ABC
				 * @param depth    depth of the traversal of the tree
				 * @return         result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbYAML(const yaml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи YAML в дерево ABC
				 *
				 * @param text   запись YAML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of YAML into a tree of ABC
				 *
				 * @param text   record of YAML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeYAML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения XML в значение ABC
				 *
				 * @details Своей системы видов у разметки нет: узел считается простым
				 *          значением тогда, когда потомков-узлов у него не нашлось, и
				 *          укладывается тогда собранным текстом. Признак этот берётся
				 *          перечнем звеньев, а не числом потомков: примечания и разделы
				 *          дословного текста узлами разметки не являются, и счёт лжёт.
				 *
				 * @param document документ записи XML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of XML into a value of ABC
				 *
				 * @details Markup has no system of the kinds of its own: a node is counted a simple
				 *          value when no child nodes were found, and
				 *          then it is laid by the assembled text. This sign is taken
				 *          from the list of the links rather than from the number of the children: comments and
				 *          sections of the literal text are not markup nodes, and the count lies.
				 *
				 * @param document document of a record of XML
				 * @param path     path to the value being laid
				 * @param result   laid value of the container ABC
				 * @param depth    depth of the traversal of the tree
				 * @return         result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbXML(const xml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth, const bool slot = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи XML в дерево ABC
				 *
				 * @param text   запись XML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of XML into a tree of ABC
				 *
				 * @param text   record of XML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeXML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения TOML в значение ABC
				 *
				 * @param document документ записи TOML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of TOML into a value of ABC
				 *
				 * @param document document of a record of TOML
				 * @param path     path to the value being laid
				 * @param result   laid value of the container ABC
				 * @param depth    depth of the traversal of the tree
				 * @return         result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbTOML(const toml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи TOML в дерево ABC
				 *
				 * @param text   запись TOML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of TOML into a tree of ABC
				 *
				 * @param text   record of TOML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeTOML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения INI в значение ABC
				 *
				 * @param document документ записи INI
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of INI into a value of ABC
				 *
				 * @param document document of a record of INI
				 * @param path     path to the value being laid
				 * @param result   laid value of the container ABC
				 * @param depth    depth of the traversal of the tree
				 * @return         result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbINI(const ini::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи INI в дерево ABC
				 *
				 * @details Обход ведётся общим ходом `children(путь)` наравне с прочими
				 *          четырьмя дорогами: ход этот у кодека INI есть тот же самый,
				 *          что и `keys(string)`, и обещание у него общее - перечень
				 *          звеньев пути к потомкам значения.
				 *
				 * @warning Прежде здесь стояло, будто обход идёт РОДНЫМ ходом кодека -
				 *          перечнем разделов и перечнем свойств раздела, - ибо общего
				 *          вида у него покуда нет. Утверждение это разошлось с телом:
				 *          общий вид у кодека появился под именем `children()`, тело
				 *          моста зовёт именно его, а запись заголовка осталась прежней.
				 *          Сверх того блок этот стоял ОСИРОТЕВШИМ - за ним шло описание
				 *          иного хода, а само объявление оставалось без описания вовсе.
				 *          Замерено 09.09.2026 аудитом
				 *
				 * @param text   запись INI для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of INI into a tree of ABC
				 *
				 * @details The traversal goes by the common method `children(path)` on a par with the other
				 *          four roads: this method of the codec INI is the very same one
				 *          as `keys(string)`, and its promise is common - a list of
				 *          the links of the path to the children of the value.
				 *
				 * @warning It used to stand here as if the traversal went by the NATIVE method of the codec -
				 *          a list of the sections and a list of the properties of the section, - for it had no
				 *          common form yet. This statement diverged from the body: a common form appeared
				 *          at the codec under the name `children()`, the body of the bridge calls exactly
				 *          it, while the record of the header stayed unchanged. Beyond that this block
				 *          stood ORPHANED - the description of another method followed it, while the
				 *          declaration itself was left without any description at all. Measured
				 *          09.09.2026 by an audit
				 *
				 * @param text   record of INI for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeINI(const string_view text, abc::value_t & result) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись указанном формате
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись в указанном формате
				 * @param format вид записи, в который переводится дерево
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method for converting an ABC tree into a record in a specified format
				 *
				 * @param value  tree of the values of the container ABC
				 * @param result a record assembled in the specified format
				 * @param format a type of record into which a tree is converted
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encode(const abc::value_t & value, string & result, const format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи в указанном формате в дерево ABC
				 *
				 * @param text   запись в указанном формате для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @param format вид записи, из которого переводится дерево
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief A method for converting a record in the specified format into an ABC tree
				 *
				 * @param text   entry in the specified format for translation
				 * @param result assembled tree of the values of the container ABC
				 * @param format a type of record from which a tree is translated
				 * @return       result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decode(const string_view text, abc::value_t & result, const format_t format) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода отказа последнего перевода
				 *
				 * @return код отказа последнего перевода
				 *
				 * \~english
				 * @brief Method of the extraction of the error code of the last translation
				 *
				 * @return error code of the last translation
				 *
				 * \~
				 */
				error_t error() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения настроек перевода
				 *
				 * @return настройки перевода
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the translation
				 *
				 * @return settings of the translation
				 *
				 * \~
				 */
				const settings_t & settings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек перевода
				 *
				 * @param settings настройки перевода
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the translation
				 *
				 * @param settings settings of the translation
				 *
				 * \~
				 */
				void settings(const settings_t & settings) noexcept;
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
				explicit Bridge() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Bridge() noexcept = default;
		} bridge_t;
	};
};

/**
 * Возвращаем подавленные системные макросы
 */
#include "../sys/macro/restore.hpp"
