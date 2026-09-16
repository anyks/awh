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
 * Подключаем заголовочные файлы проекта
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
 */
#include "../sys/macro/suppress.hpp"
#include "numeric.hpp"
#include "abc/value.hpp"
#include "json/json.hpp"
#include "yaml/yaml.hpp"
#include "xml/xml.hpp"
#include "toml/toml.hpp"
#include "ini/ini.hpp"

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
		 * кодеков и обратно. Осью перевода служит именно ABC: система видов его вмещает
		 * виды всех текстовых кодеков разом, оттого перевод между двумя текстовыми
		 * кодеками идёт через одно промежуточное дерево, а не через пятнадцать пар
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Сужение выдаётся отказом, а не молчаливой порчей.** Виды ABC, записи
		 * кодека неведомые, обращаются по правилу, настройками заданному: либо в
		 * последовательность знаков, либо в отказ с кодом. Молчаливая же порча
		 * оставляла бы потребителя с записью, которая разбирается, но означает иное
		 *
		 * @li **Перевод ведётся обходом дерева, а не разбором записи.** Дерево ABC
		 * подаётся писателю кодека событиями, и второго представления по дороге не
		 * заводится вовсе
		 *
		 * @li **Дословного совпадения записи перевод не обещает - обещает значение.**
		 * Запись «1e2» выдаётся как «100.0», «1.50» как «1.5», лишнее экранирование
		 * снимается, а суррогатная пара обращается самим знаком. Это записанный
		 * договор кодеков, а не изъян моста: разбор хранит число, а не знаки его.
		 * Обратимость закреплена сличением ДЕРЕВЬЕВ, а не текстов
		 *
		 * @li **Число сверх родных видов выдаётся последовательностью знаков.** Вид
		 * `EXTENDED` записи JSON укладывается в ABC записью числа знаками, ибо
		 * перевод числа в родной вид терял бы разряды. Запись числа берётся ходом
		 * `raw()`, а НЕ `text()`: последний выдаёт содержимое лишь у строковых узлов.
		 * Причина знаков лежит не у ABC - ход `Value::decimal` вид этот собрать
		 * позволяет, - а у самого JSON: величину числа он держит записью знаков, а
		 * не октетами, и накормить `decimal` мосту нечем, не разбирая десятичную
		 * запись произвольной длины самому
		 *
		 * @li **Пустой узел разметки укладывается по настройке.** Узел `<br/>` выражает
		 * то, чего у прочих форматов нет вовсе - наличие без значения, - и согласия в
		 * мире нет: Parker кладёт пустое значение, xml2js пустую запись, Badgerfish
		 * пустое отображение, средства настройки логическую истину. Правило вынесено
		 * настройкою `empty`, умолчанием взята истина: у настроек узел этот значит
		 * «включено»
		 *
		 * @li **Перечень безымянный несёт пометку, а не обнос по договорённости.**
		 * Перечень у разметки выражается ПОВТОРОМ одноимённых узлов, и повтор годен
		 * лишь при двух звеньях и более, да и то полем отображения. Перечень верхнего
		 * уровня, перечень внутри перечня и перечень из одного звена повтора не дают
		 * вовсе - им ставится свойство `array`, обратным чтением снимаемое. Догадка по
		 * виду записи здесь негодна: отображение с полем `item` неотличимо от перечня
		 * обносов, и всякая догадка портила бы одно ради другого
		 *
		 * @li **Имя корня безымянному дереву даётся настройкою.** У документа разметки
		 * ровно один корень, и безымянным он быть не может; перечень же и дерево ABC
		 * корня именованного не имеют. Оттого круг перевода замыкается СО ВТОРОГО
		 * прохода: первый даёт имя, дальнейшие неподвижны
		 *
		 * @li **Имя поля, разметке негодное, правится приставкой.** Имя узла с ведущей
		 * цифрой стандарт XML 1.0 запрещает, а у отображения оно законно и приходит
		 * из записи JSON: поле `3` получает приставку `Item`. Круг на нём НЕ
		 * замыкается - обратное чтение отдаёт `Item3`, - и замкнуть его нечем
		 *
		 * @li **Содержимое смешанное теряет текст узла.** Запись `<a>текст<b/></a>`
		 * укладывается отображением с одним лишь полем `b`: у отображения текст лечь
		 * может только полем, а поле стало бы неотличимо от потомка с тем же именем.
		 * Правило то же, что у эталонного моста
		 *
		 * \~english
		 * @brief Bridge between the container ABC and the text codecs
		 * @details Translates a tree of the values of the container ABC into a record of any of the text
		 * codecs and back. Precisely ABC serves as the axis of the translation: its system of the kinds encompasses
		 * the kinds of all the text codecs at once, whereby a translation between two text
		 * codecs goes through one intermediate tree rather than through fifteen pairs
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Bridge {
			public:
				/**
				 * \~russian
				 * @brief Коды отказов перевода
				 *
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
					 * Устройство записи кодека дерева не вмещает
					 *
					 * @details Код этот отделён от `UNSUPPORTED` намеренно: тот
					 * означает «вид значения записи неведом», и чинится он сменою
					 * ВИДА, а этот означает «устройство записи такого не вмещает», и
					 * чинится он сменою УСТРОЙСТВА. Запись INI, к примеру, глубже
					 * раздела со свойством не идёт, и перечень отображений в неё не
					 * ложится ни при каком виде значений
					 *
					 * @note Различение это не украшение: потребитель, увидевший один
					 *       код на обе беды, не знает, что ему править - значение либо
					 *       дерево
					 */
					STRUCTURE   = 0x05,
					/**
					 * \~russian
					 * Настройка перевода негодна
					 *
					 * @details Код этот отделён от `WRITING` намеренно: тот означает
					 * «кодек записать не смог», и искать причину потребитель идёт в
					 * ДЕРЕВЕ, а этот означает «негодна сама настройка», и чинится он
					 * сменою НАСТРОЙКИ. Имя корня и имя звена перечня становятся у
					 * разметки именами узлов, а имя узла бывает не всяким: пустое
					 * имя либо имя со знаком, стандартом не допущенным, записи
					 * негодно вовсе
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
					 * «the codec could not write», and the consumer goes looking for the cause in the
					 * TREE, while this one means «the setting itself is invalid», and it is fixed
					 * by changing the SETTING
					 *
					 * \~
					 */
					SETTINGS    = 0x06
				};
				/**
				 * \~russian
				 * @brief Вид записи, с которым мост переводит дерево
				 *
				 * @details Кодек CSV в перечень не входит намеренно: содержимое его
				 *          таблично, а не древовидно, и настройками оно не бывает
				 *
				 * \~english
				 * @brief Kind of a record with which the bridge translates a tree
				 * @details The codec CSV is deliberately not in the list: its content is
				 *          tabular, not tree-shaped, and it is never settings
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
					 * Отвечать отказом
					 *
					 * @note Код отказа берётся ПО ПРИЧИНЕ, а не один и тот же:
					 *       `UNSUPPORTED`, когда записи неведом сам вид значения, и
					 *       `STRUCTURE`, когда вид ей ведом, а устройство записи
					 *       значения такого не вмещает - перечень пустой у разметки и
					 *       у INI, к примеру, выразить нечем, тогда как перечень
					 *       непустой те же записи выражают повтором. Различение это
					 *       описано у самих кодов, и правило сужения его не отменяет
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
				 * то, чего у прочих форматов нет вовсе - НАЛИЧИЕ без значения. Согласия
				 * в мире нет: Parker кладёт `null`, xmltodict - `None`, xml2js и
				 * Newtonsoft - пустую запись, Badgerfish - пустое отображение, а средства
				 * настройки - логическую истину. Оттого правило это и вынесено настройкой
				 *
				 * @note Умолчанием взята ИСТИНА: у настроек узел этот означает
				 *       «включено», и владелец назвал такое прочтение привычным
				 *
				 *
				 * \~english
				 * @brief Rule for laying out an empty markup node
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
				 * @brief Настройки перевода
				 *
				 *
				 * \~english
				 * @brief Settings of the translation
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Правило обращения с видами, записи кодека неведомыми
					narrow_t narrow;
					/**
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
					 */
					json::format_t format;
					// Предельная глубина дерева перевода
					uint32_t depth;
					/**
					 * Признак вывода вида значения из его записи
					 *
					 * @warning Настройка эта нужна видам записи, своей системы видов не
					 *          имеющим - разметке прежде всего: у неё всякое содержимое
					 *          есть текст, и `<port>8080</port>` без вывода вида лёг бы
					 *          строкою, а не числом. Вывод ведётся цепью: целое, дробное,
					 *          логическое, иначе строка
					 */
					bool typed;
					/**
					 * Имя поля, под которым ложится текст узла с атрибутами
					 *
					 * @warning Имя это нужно оттого, что узел разметки несёт РАЗОМ и
					 *          атрибуты, и собственный текст, а отображение выразить это
					 *          может лишь полем. При столкновении с одноимённым атрибутом
					 *          имя получает приставку `_` столько раз, сколько нужно
					 */
					string text;
					/**
					 * Имя корневого узла собираемой записи разметки
					 *
					 * @warning Настройка эта нужна одному лишь виду XML и зашита быть не
					 *          может: стандарт XML 1.0 требует у документа РОВНО ОДИН
					 *          корневой элемент, а у дерева контейнера ABC корень
					 *          безымянен. Имя взять неоткуда, кроме как у того, кто
					 *          пишет, и молчаливый выбор моста был бы выдумкой
					 */
					string root;
					/**
					 * Имя узла, обносящего перечень, во вместилище вложенный
					 *
					 * @warning Имя это нужно оттого, что перечень у разметки ИМЕНИ НЕ
					 *          ИМЕЕТ вовсе: перечень выражается повтором одноимённых
					 *          узлов, и перечень внутри перечня выразить нечем -
					 *          одноимённые узлы легли бы вперемешку и границы вложенного
					 *          перечня пропали бы. Оттого всякое звено вложенного перечня
					 *          обносится узлом с этим именем
					 */
					string item;
					// Правило укладки пустого узла разметки
					empty_t empty;
					/**
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
					 */
					string array;
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
					Settings() noexcept : narrow(narrow_t::TEXT), format(json::format_t::PRETTY), depth(512), typed(true), text{"value"}, root{"config"}, item{"item"}, empty(empty_t::BOOLEAN), array{"array"} {}
				} settings_t;
			private:
				// Настройки перевода
				settings_t _settings;
			private:
				// Код отказа последнего перевода
				error_t _error;
			private:
				/**
				 * \~russian
				 * Признак пропуска значения правилом сужения
				 *
				 * @details Правило `SKIP` велит пропускать значение ВОВСЕ, а ходы подачи
				 * отвечают лишь успехом либо отказом, и «пропущено» от «подано» ими не
				 * отличалось: подающий выходил успехом, ничего не тронув, а вкладывающий
				 * клал НЕТРОНУТОЕ значение полем. Оттого пропуск оборачивался укладкою
				 * пустого значения у четырёх дорог и отказом записи целиком у пятой -
				 * настройка не исполняла обещанного НИ НА ОДНОЙ
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
				 * \~
				 */
				bool _skipped;
			private:
				/**
				 * \~russian
				 * @brief Метод упреждающего спроса пропуска значения правилом сужения
				 *
				 * @details Ход этот нужен ОДНОЙ лишь дороге JSON: писатель её поточный,
				 * имя поля уходит в запись ДО значения, и пропустить значение после записи
				 * имени нельзя вовсе. Прочие четыре дороги собирают владеющее значение и
				 * кладут его полем СЛЕДОМ, и им довольно признака `_skipped`
				 *
				 * @param value значение контейнера ABC
				 * @param depth глубина обхода дерева
				 * @return      признак пропуска значения правилом сужения
				 *
				 * \~english
				 * @brief Method of the look-ahead inquiry of the skipping of a value by the narrowing rule
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
				 * его есть отказ по НЕВЫРАЗИМОСТИ: разбирать его надлежит правилом сужения,
				 * а не выдачей наружу кодом `WRITING`
				 *
				 * @param name имя свойства, записи не принятого
				 * @return     результат разбора: истина, когда запись продолжима
				 *
				 * \~english
				 * @brief Method of the treatment of a refusal of the writer of the record INI by the narrowing rule
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
				 * ею невыразимое, ведётся правилом сужения. Спрос стоит у моста оттого, что
				 * кодек отвечает отказом лишь при посадке собранного узла документом - то
				 * есть после того, как всё дерево уже подано, и поле, отказ вызвавшее, уже
				 * не отличить
				 *
				 * @param value  значение контейнера ABC
				 * @param result собранная запись значения
				 * @return       результат сборки: истина, когда запись продолжима
				 *
				 * \~english
				 * @brief Method of the assembly of a record of a value admissible in the markup
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
				 * @param value value of the container ABC
				 * @param writer writer of a record of JSON
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
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
				 * @param value value of a document of JSON
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorb(const json::Document::value_t & value, abc::value_t & result, const uint32_t depth) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись самого контейнера ABC
				 *
				 * @details Дорога эта не переводит ничего: запись контейнера собирается
				 * им самим. Мост же ставит её вровень с прочими видами, дабы потребитель,
				 * вид записи выбирающий настройкой, не знал о ABC особого хода
				 *
				 * @warning Запись эта ДВОИЧНА и укладывается в строку октетами как есть
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of the container ABC itself
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод спроса о виде, всякой текстовой записи родном
				 *
				 * @details Родными считаются виды, какие всякий текстовый кодек выражает
				 * сам: пустота, логическое значение, последовательность знаков, число
				 * целое и дробное да отметка времени, числом записываемая. Прочие -
				 * двоичные данные, опознаватель, десятичное с точным разрядом, целое
				 * сверх родных видов и открытое расширение - записи неведомы, и обращение
				 * с ними решает настройка сужения
				 *
				 * @param value значение контейнера ABC
				 * @return      признак родного вида
				 *
				 * \~english
				 * @brief Method of the question about a kind native to every text record
				 * @param value value of the container ABC
				 * @return sign of a native kind
				 *
				 * \~
				 */
				[[nodiscard]] bool native(const abc::value_t & value) const noexcept;
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
				 * @param text record of the container ABC for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of JSON
				 * @return result of the translation
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
				 * @param text record of JSON for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 *          Ход возвращает звену исходное имя
				 *
				 * @param link звено пути с отменяющими записями
				 * @return     исходное имя потомка
				 *
				 * \~english
				 * @brief Method of the removal of the escaping from a link of a path
				 * @param link link of a path with the escaping
				 * @return original name of the child
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
				 *          несущее, разошлось бы у кодека на два звена пути
				 *
				 * @param name исходное имя потомка
				 * @return     звено пути с отменяющими записями
				 *
				 * \~english
				 * @brief Method of the applying of the escaping to a name of a child
				 * @param name original name of the child
				 * @return link of a path with the escaping
				 *
				 * \~
				 */
				[[nodiscard]] string escape(const string & name) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выдачи записи простого значения ABC
				 *
				 * @details Ход этот нужен видам записи, у которых своей системы видов
				 *          нет: значение выдаётся ТЕКСТОМ, каким оно записывается
				 *
				 * @param value значение контейнера ABC
				 * @return      запись простого значения
				 *
				 * \~english
				 * @brief Method of getting the record of a simple value of ABC
				 * @param value value of the container ABC
				 * @return record of the simple value
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
				 *          системы видов не имеющим
				 *
				 * @param text запись значения
				 * @return     выведенное значение контейнера ABC
				 *
				 * \~english
				 * @brief Method of the inference of a value of ABC from its record
				 * @param text record of the value
				 * @return inferred value of the container ABC
				 *
				 * \~
				 */
				[[nodiscard]] abc::value_t infer(const string & text) const noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC документу YAML
				 *
				 * @param value    значение контейнера ABC
								 * @param result   собираемое значение записи YAML
				 * @param depth    глубина обхода дерева
				 * @return         результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to the document of YAML
				 * @param value value of the container ABC
				 * @param document assembled document of a record of YAML
				 * @param path path to the value being submitted
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of YAML
				 * @return result of the translation
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
				 * @param value value of the container ABC
				 * @param result assembled value of a record of TOML
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of TOML
				 * @return result of the translation
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
				 * @param value value of the container ABC
				 * @param result assembled value of a record of XML
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of XML
				 * @return result of the translation
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
				 * @param value value of the container ABC
				 * @param document assembled document of a record of INI
				 * @param name name of the property
				 * @param section name of the section
				 * @return result of the writing
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of INI
				 * @return result of the translation
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
				 * @param document document of a record of YAML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
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
				 * @param text record of YAML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 *          дословного текста узлами разметки не являются, и счёт лжёт
				 *
				 * @param document документ записи XML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of XML into a value of ABC
				 * @param document document of a record of XML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
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
				 * @param text record of XML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 * @param document document of a record of TOML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
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
				 * @param text record of TOML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 * @param document document of a record of INI
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
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
				 *          звеньев пути к потомкам значения
				 *
				 * @param text   запись INI для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
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
				 * \~english
				 * @brief Method of the translation of a record of INI into a tree of ABC
				 * @param text record of INI for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeINI(const string_view text, abc::value_t & result) noexcept;
			public:
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
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of JSON
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encode(const abc::value_t & value, string & result, const format_t format) noexcept;
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
				 * @param text record of JSON for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
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
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Bridge() noexcept {}
		} bridge_t;
	}
}

/**
 * Возвращаем подавленные системные макросы
 */
#include "../sys/macro/restore.hpp"
