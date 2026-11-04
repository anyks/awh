/**
 * @file args.hpp
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
 * @brief Заголовочный файл модуля параметров запуска приложения
 *
 * \~english
 * @brief Header file of the module of the parameters of the launch of an application
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
#include <map>
#include <vector>
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "lexer.hpp"
#include "common.hpp"
#include "schema.hpp"
#include "../sys/fs.hpp"
#include "../codec/bridge.hpp"
#include "../codec/abc/value.hpp"
#include "../sys/macro/global.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён параметров запуска приложения
	 *
	 * \~english
	 * @brief Namespace of the parameters of the launch of an application
	 *
	 * \~
	 */
	namespace args {
		/**
		 * \~russian
		 * @brief Параметры запуска приложения
		 *
		 * @details Собирает настройки приложения из набора запуска, переменных окружения
		 *          и текстовых потоков в единое дерево значений. Осью хранения служит владеющее
		 *          значение контейнера ABC: система видов его вмещает виды всех текстовых кодеков
		 *          разом, оттого выдача настроек любым из них не требует второго дерева
		 *
		 * @par Намеренные решения
		 *
		 *      Перечисленное ниже не является пробелом реализации: это очерченные границы
		 *      задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Старшинство источников задано порядком членов перечня.** Значение из
		 *     набора запуска перекрывает переменную окружения, та - файл настроек, а тот -
		 *     значение по умолчанию. Обратное же перекрытие не совершается вовсе: значение
		 *     младшего источника поверх старшего не ложится, в каком бы порядке источники
		 *     ни подавались. Иначе разбор файла настроек, поданный после набора запуска,
		 *     молча отменял бы поданное из набора
		 *
		 *     Приоритет проверяется также между родителем и потомком: файл с полем
		 *     `net` не заменяет `net.port` из CLI. При замене младшего скаляра потомком
		 *     скаляр освобождает место для поддерева. Источники вытесненных значений
		 *     удаляются; при сборе повторов массивом пути сохранённых потомков переносятся
		 *
		 * @li **Источник хранится у каждого значения.** Без него ответ на вопрос
		 *     «откуда взялось это значение» неотличим от догадки, а отладка настроек слепа
		 *
		 * @li **Вложенное пустое отображение является значением.** При загрузке оно
		 *     сохраняется вместе с источником и проверяется строгой схемой. Пустой
		 *     корневой документ не добавляет параметров и не очищает прежние настройки
		 *
		 * @li **Вид значения выводится из записи, и это настройка.** Записи «true»,
		 *     «17» и «1.5», поданные набором запуска, ложатся логическим значением и
		 *     числами, а не последовательностями знаков. Довод тому - совпадение с файлом
		 *     настроек: без вывода вида один и тот же ключ приходил бы числом из файла и
		 *     знаками из набора, и потребителю пришлось бы знать источник. Обратный уклад
		 *     отведён настройкой тем, кому вывод вида мешает
		 *
		 * @li **Разделитель пути в имени параметра - точка, а внутри оси - косая.**
		 *     Запись «--net.port=80» ложится по пути «net/port» оси хранения. Точка
		 *     выбрана привычною записи настроек, а перевод её в косую делается разбором,
		 *     а не потребителем
		 *
		 * \~english
		 * @brief Parameters of the launch of an application
		 *
		 * @details Assembles the settings of an application from the set of the launch, from the variables of the environment
		 *          and from the text streams into a single tree of the values. The owning value of the container ABC serves as the axis
		 *          of the storage: its system of the kinds encompasses the kinds of all the text codecs
		 *          at once, whereby the issuance of the settings by any of them requires no second tree.
		 *
		 *
		 * @par Deliberate decisions
		 *
		 *      Listed below is not a gap of the implementation: these are the outlined bounds
		 *      of the task, and each of the decisions is fixed by a verification test
		 *
		 * @li **The seniority of the sources is set by an order of the members of a list.** A value from
		 *     a launch set overrides an environment variable, that one - a settings file, and that one -
		 *     a default value. A reverse overriding, however, is not performed at all: a value
		 *     of a junior source does not lie above a senior one, in whatever order the sources
		 *     were submitted. Otherwise a parsing of a settings file, submitted after a launch set,
		 *     would silently cancel what was submitted from the set
		 *
		 *     A priority is also checked between a parent and a descendant: a file with a field
		 *     `net` does not replace `net.port` from a CLI. At a replacing of a junior scalar by a descendant
		 *     a scalar frees a place for a sub-tree. The sources of the displaced values
		 *     are removed; at a gathering of the repeats by an array the paths of the kept descendants are moved
		 *
		 * @li **A source is stored at each value.** Without it an answer to a question
		 *     `where this value came from` is indistinguishable from a guess, and a debugging of the settings is blind
		 *
		 * @li **A nested empty mapping is a value.** On a loading it
		 *     is kept together with a source and is checked by a strict schema. An empty
		 *     root document adds no parameters and does not clear the former settings
		 *
		 * @li **A kind of a value is inferred from a record, and this is a setting.** The records `true`,
		 *     `17` and `1.5`, submitted by a launch set, lie as a logical value and numbers,
		 *     and not as sequences of signs. An argument to this is a match with a settings
		 *     file: without an inference of a kind the same key came as a number from a file and
		 *     as signs from a set, and a consumer had to know a source. A reverse laying
		 *     is allotted by a setting to those, for whom an inference of a kind hinders
		 *
		 * @li **A separator of a path in a name of a parameter is a dot, and inside an axis is a slash.**
		 *     A record `--net.port=80` lies by a path `net/port` of an axis of the storage. A dot
		 *     is chosen as usual for a record of the settings, and its turning into a slash is done by a parsing,
		 *     and not by a consumer
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Args {
			public:
				/**
				 * \~russian
				 * @brief Настройки сбора параметров запуска
				 *
				 *
				 * \~english
				 * @brief Settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Признак вывода вида значения из его записи
					bool typed;
					/**
					 * \~russian
					 * Признак отказа на параметр, описанию ожидаемых неизвестный
					 *
					 * @note Строгость спрашивается ВСЯКИМ источником - набором доводов
					 *       запуска, текстовым потоком, окружением, файлом настроек и
					 *       значением по умолчанию, - ибо она есть свойство разбора, а
					 *       не одной его дороги. Закреплено проверкой
					 *       `Args.TheStrictnessIsAskedByEverySource`
					 *
					 * @note Годным считается не только имя описанного параметра, но и
					 *       всякое имя ПОД ним: описав имя `host`, потребитель вправе
					 *       подать ему поддерево файлом настроек
					 *
					 * @note При описании ПУСТОМ строгость не спрашивается вовсе:
					 *       сверять поданное не с чем, и разбор принимает всякое имя
					 *
					 * \~english
					 * Sign of a refusal of a parameter unknown to the schema of the expected ones
					 *
					 *
					 * @note A strictness is asked by EVERY source - a set of the arguments of a launch,
					 *       a text stream, an environment, a settings file and a default value, - for it
					 *       is a property of a parsing, and not of one of its roads. Fixed by a check
					 *       `Args.TheStrictnessIsAskedByEverySource`
					 *
					 * @note A fit name is counted not only a name of a described parameter, but also
					 *       every name UNDER it: having described a name `host`, a consumer has a right
					 *       to submit to it a sub-tree by a settings file
					 *
					 * @note At an EMPTY description a strictness is not asked at all:
					 *       there is nothing to compare a submitted one with, and a parsing accepts every name
					 *
					 * \~
					 */
					bool strict;
					/**
					 * \~russian
					 * Признак укладки повторно поданного параметра массивом
					 *
					 * @details Признак этот СТАРШЕ одноимённого признака у описания
					 *          параметра, и старшинство это решено 15.09.2026: описание
					 *          говорит, ДОЗВОЛЕН ли повтор вовсе, а настройка - собирается ли он
					 *          массивом. Веля не собирать, настройка запрещает сбор ВСЕМ параметрам,
					 *          включая те, чьё описание повтор дозволяет.
					 *
					 * @note Сочетание «описание дозволяет, настройка не велит» потому и
					 *       законно: повтор принимается без отказа, а побеждает последнее
					 *       поданное значение. Отказ `DUPLICATE` тут был бы ложью - он
					 *       означает «повтор запрещён», а запрещён лишь сбор массивом
					 *
					 * @warning Прежнее значение при этом УХОДИТ, и уход этот оглашается
					 *          строкою журнала: запись `--порт=1 --порт=2` при настройке без
					 *          массивов оставляет двойку, а единица прежде пропадала
					 *          БЕССЛЕДНО - ни отказа, ни строки. Молчаливую потерю здесь
					 *          оставлять нельзя, и закреплена она проверкою
					 *          `Args.TheRepeatWithoutArraysKeepsTheLastValue`
					 *
					 * \~english
					 * Sign of the laying of a repeatedly submitted parameter as an array
					 *
					 *
					 * @details This sign is SENIOR to the homonymous sign at a description
					 *          of a parameter, and this seniority was decided on 2026-09-15: a description
					 *          says whether a repeat is ALLOWED at all, and a setting - whether it is gathered
					 *          by an array. Ordering not to gather, a setting forbids a gathering to ALL parameters,
					 *          including those whose description allows a repeat.
					 *
					 * @note A combination `a description allows, a setting does not order` is therefore
					 *       lawful: a repeat is accepted without a refusal, and a last submitted
					 *       value wins. A refusal `DUPLICATE` here would be a lie - it
					 *       means `a repeat is forbidden`, while only a gathering by an array is forbidden
					 *
					 * @warning A former value at this LEAVES, and its leaving is announced
					 *          by a line of the journal: a record `--порт=1 --порт=2` at a setting without
					 *          the arrays keeps a two, and a unit before was lost
					 *          WITHOUT A TRACE - neither a refusal nor a line. A silent loss here
					 *          can not be kept, and it is fixed by a check
					 *          `Args.TheRepeatWithoutArraysKeepsTheLastValue`
					 *
					 * \~
					 */
					bool multiple;
					// Разделитель звеньев пути в имени параметра
					char delimiter;
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
				// Начало имён переменных окружения, приложению отведённых
				string _prefix;
			private:
				// Описание ожидаемых параметров запуска
				schema_t _schema;
			private:
				// Настройки сбора параметров запуска
				settings_t _settings;
			private:
				// Разборщик параметров запуска и текстовых потоков
				lexer_t _lexer;
				// Мост между контейнером ABC и текстовыми кодеками
				codec::bridge_t _bridge;
			private:
				// Дерево собранных значений настроек
				codec::abc::value_t _root;
			private:
				// Источники собранных значений настроек, ключом путь звеньями
				std::map <string, source_t> _origins;
			private:
				// Позиционные доводы в порядке их встречи в наборе запуска
				vector <string> _operands;
				// Отказы, случившиеся при последнем разборе
				vector <pair <error_t, location_t>> _errors;
			private:
				/**
				 * \~russian
				 * @brief Метод перевода имени параметра в путь оси хранения
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    путь звеньями оси хранения
				 *
				 * \~english
				 * @brief Method of the conversion of a name of a parameter into a path of the axis of the storage
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    path by the links of the axis of the storage
				 *
				 * \~
				 */
				string route(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выведения значения из его записи
				 *
				 * @details Вид выводится лишь при взведённой настройке; иначе запись
				 *          ложится последовательностью знаков как есть.
				 *
				 * @param text запись значения
				 * @return     выведенное значение оси хранения
				 *
				 * \~english
				 * @brief Method of the inference of a value from its record
				 *
				 *
				 * @details A kind is inferred only at an armed setting; otherwise a record
				 *          lies as a sequence of signs as it is.
				 *
				 * @param text record of a value
				 * @return     inferred value of the axis of the storage
				 *
				 * \~
				 */
				codec::abc::value_t derive(const string_view text) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод укладки значения в дерево настроек
				 *
				 * @details Значение младшего источника поверх старшего не ложится.
				 *          До изменения дерева проверяется возможность записи по всему пути.
				 *          Несовместимый вид родителя и превышение предела роста массива дают
				 *          `INVALID_PATH`, сохраняя прежние значения и их источники
				 *
				 * @param path    путь звеньями оси хранения
				 * @param value   значение для укладки
				 * @param source  источник значения
				 * @param mapping признак создания новых родителей отображениями при слиянии
				 * @return        результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value into the tree of the settings
				 *
				 * @details The full path is checked before the tree is changed. An incompatible
				 *          parent or an array growth limit produces `INVALID_PATH` and preserves existing
				 *          values and their sources
				 *
				 * @param path    path by the links of the axis of the storage
				 * @param value   value for the laying
				 * @param source  source of the value
				 * @param mapping create new parents as mappings while merging a configuration tree
				 * @return        result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool lay(const string & path, codec::abc::value_t && value, const source_t source, const bool mapping = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод слияния дерева значений с деревом настроек
				 *
				 * @details Отображения сливаются вглубь, а вместимые и одиночные значения
				 *          ложатся целиком: сливать их поэлементно значило бы смешивать перечень,
				 *          поданный файлом настроек, с перечнем, поданным набором запуска.
				 *
				 * @param value  сливаемое дерево значений
				 * @param path   путь звеньями оси хранения, уже пройденный слиянием
				 * @param source источник сливаемых значений
				 * @return       результат слияния
				 *
				 * \~english
				 * @brief Method of the merging of a tree of the values with the tree of the settings
				 *
				 *
				 * @details The mappings are merged in depth, and the container and single values
				 *          lie wholly: to merge them element by element would mean to mix a list,
				 *          submitted by a settings file, with a list submitted by a launch set.
				 *
				 * @param value  merged tree of the values
				 * @param path   path by the links of the axis of the storage already passed by the merging
				 * @param source source of the merged values
				 * @return       result of the merging
				 *
				 * \~
				 */
				[[nodiscard]] bool merge(const codec::abc::value_t & value, const string & path, const source_t source, const bool derive = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки разобранной лексемы по описанию ожидаемых
				 *
				 * @details Без описания лексема ложится как есть. С описанием же имя
				 *          сличается с ожидаемыми, склейка коротких имён разбирается, а потребность
				 *          значения проверяется.
				 *
				 * @param lexeme разобранная лексема
				 * @param source источник поданного значения
				 * @return       результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a parsed lexeme by the description of the expected
				 *
				 *
				 * @details Without a description a lexeme lies as it is. With a description, however, a name
				 *          is compared with the expected ones, a gluing of the short names is parsed, and a necessity
				 *          of a value is checked.
				 *
				 * @param lexeme parsed lexeme
				 * @param source source of the submitted value
				 * @return       result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool apply(const lexeme_t & lexeme, const source_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки собранных параметров запуска
				 *
				 * \~english
				 * @brief Method of the clearing of the assembled parameters of the launch
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска
				 *
				 * @param count число доводов набора запуска
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch
				 *
				 * @param count number of the arguments of the set of the launch
				 * @param items set of the arguments of the launch
				 * @return      result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const int32_t count, const char * items[]) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска широкими знаками
				 *
				 * @details Отведён MS Windows, где набор запуска приходит широкими знаками.
				 *
				 * @param count число доводов набора запуска
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch by the wide characters
				 *
				 *
				 * @details It is allotted to MS Windows, where a launch set comes by wide signs.
				 *
				 * @param count number of the arguments of the set of the launch
				 * @param items set of the arguments of the launch
				 * @return      result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const int32_t count, const wchar_t * items[]) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска
				 *
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch
				 *
				 * @param items set of the arguments of the launch
				 * @return      result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const vector <string> & items) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора текстового потока
				 *
				 * @details Отведён вводу с клавиатуры и подаче настроек одною строкою.
				 *
				 * @param text текст для разбора
				 * @return     результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a text stream
				 *
				 *
				 * @details It is allotted to an input from a keyboard and to a submission of the settings by one line.
				 *
				 * @param text text for the parsing
				 * @return     result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool text(const string_view text) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сбора переменных окружения
				 *
				 * @details Собираются переменные, имя которых начато отведённым приложению
				 *          началом. Имя «PREFIX_NET_PORT» ложится по пути «net/port» оси хранения.
				 *
				 * @return результат сбора
				 *
				 * \~english
				 * @brief Method of the assembling of the variables of the environment
				 *
				 *
				 * @details The variables are gathered, whose name is begun by a beginning allotted to an
				 *          application. A name `PREFIX_NET_PORT` lies by a path `net/port` of an axis of the storage.
				 *
				 * @return result of the assembling
				 *
				 * \~
				 */
				[[nodiscard]] bool env() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки значения по умолчанию
				 *
				 * @param key   имя параметра с разделителем звеньев
				 * @param value запись значения по умолчанию
				 * @return      результат установки
				 *
				 * \~english
				 * @brief Method of the setting of a value by default
				 *
				 * @param key   name of a parameter with the separator of the links
				 * @param value record of the value by default
				 * @return      result of the setting
				 *
				 * \~
				 */
				[[nodiscard]] bool fallback(const string_view key, const string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения описания ожидаемых параметров запуска
				 *
				 * @return описание ожидаемых параметров запуска
				 *
				 * \~english
				 * @brief Method of the extraction of the description of the expected parameters of the launch
				 *
				 * @return description of the expected parameters of the launch
				 *
				 * \~
				 */
				schema_t & schema() noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки справки о применении
				 *
				 * @return собранный текст справки
				 *
				 * \~english
				 * @brief Method of the assembling of the help of the usage
				 *
				 * @return assembled text of the help
				 *
				 * \~
				 */
				string usage() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки собранного по описанию ожидаемых
				 *
				 * @details Проверяется подача обязательных параметров, а значения по
				 *          умолчанию, описанием заданные, укладываются недостающим
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of the check of the assembled by the description of the expected
				 *
				 *
				 * @details A submission of the obligatory parameters is checked, and the default values, set by
				 *          a description, are laid for the missing ones
				 *
				 * @return result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool verify() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора записи настроек кодеком
				 *
				 * @details Разобранное сливается с деревом настроек источником файла:
				 *          значения набора запуска, текстового потока и окружения его перекрывают.
				 *
				 * @param text запись настроек для разбора
				 * @return     результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a record of the settings by a codec
				 *
				 *
				 * @details A parsed one is merged with a tree of the settings by a source of a file:
				 *          the values of a launch set, a text stream and an environment override it.
				 *
				 * @param text record of the settings for the parsing
				 * @return     result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool config(const string_view text, const codec::Bridge::format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод чтения файла настроек
				 *
				 * @param filename путь к файлу настроек
				 * @return         результат чтения
				 *
				 * \~english
				 * @brief Method of the reading of a file of the settings
				 *
				 * @param filename path to the file of the settings
				 * @return         result of the reading
				 *
				 * \~
				 */
				[[nodiscard]] bool filename(const string & filename, const codec::Bridge::format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод чтения файла настроек с выводом вида записи из его имени
				 *
				 * @details Вид записи берётся РАСШИРЕНИЕМ имени файла: `.json`, `.yaml`
				 *          либо `.yml`, `.toml`, `.ini` либо `.conf`, `.xml`. Расширение неведомое
				 *          отвечает отказом, а не догадкою по содержимому: догадка на записи,
				 *          годной двум видам разом, выбирала бы молча.
				 *
				 * @note Ход этот ДОБАВЛЕН к прежнему, а не заменяет его: потребитель,
				 *       вид записи знающий, задаёт его сам, и имя файла ему не указ
				 *
				 * @param filename путь к файлу настроек
				 * @return         результат чтения
				 *
				 * \~english
				 * @brief Method of the reading of a file of the settings with the deduction of the kind of the record from its name
				 *
				 *
				 * @details A kind of a record is taken by an EXTENSION of a file name: `.json`, `.yaml`
				 *          or `.yml`, `.toml`, `.ini` or `.conf`, `.xml`. An unknown extension
				 *          answers by a refusal, and not by a guess on a contents: a guess on a record,
				 *          fit for two kinds at once, would choose silently.
				 *
				 * @note This move is ADDED to the former, and does not replace it: a consumer,
				 *       knowing a kind of a record, sets it by himself, and a file name is not an order to him
				 *
				 * @param filename path to the file of the settings
				 * @return         result of the reading
				 *
				 * \~
				 */
				[[nodiscard]] bool filename(const string & filename) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи дерева настроек записью кодека
				 *
				 * @param result собранная запись настроек
				 * @return       результат выдачи
				 *
				 * \~english
				 * @brief Method of the issuance of the tree of the settings by a record of a codec
				 *
				 * @param result assembled record of the settings
				 * @return       result of the issuance
				 *
				 * \~
				 */
				[[nodiscard]] bool dump(string & result, const codec::Bridge::format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод записи дерева настроек в файл
				 *
				 * @param filename путь к файлу настроек
				 * @return         результат записи
				 *
				 * \~english
				 * @brief Method of the writing of the tree of the settings into a file
				 *
				 * @param filename path to the file of the settings
				 * @return         result of the writing
				 *
				 * \~
				 */
				[[nodiscard]] bool save(const string & filename, const codec::Bridge::format_t format) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения моста между контейнером ABC и текстовыми кодеками
				 *
				 * @details Выдан наружу настройкам перевода: правило обращения с видами,
				 *          записи кодека неведомыми, и вид оформления задаются им.
				 *
				 * @return мост между контейнером ABC и текстовыми кодеками
				 *
				 * \~english
				 * @brief Method of the extraction of the bridge between the container ABC and the text codecs
				 *
				 *
				 * @details It is issued outward to the settings of a translation: a rule of the treatment of the kinds,
				 *          unknown to a record of a codec, and a kind of a formatting are set by it.
				 *
				 * @return bridge between the container ABC and the text codecs
				 *
				 * \~
				 */
				codec::bridge_t & bridge() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    результат проверки
				 *
				 * \~english
				 * @brief Method of the check of the presence of a parameter
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool has(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения источника значения параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    источник значения параметра
				 *
				 * \~english
				 * @brief Method of the extraction of the source of a value of a parameter
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    source of the value of the parameter
				 *
				 * \~
				 */
				source_t source(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения числа значений вместимого параметра
				 *
				 * @details Значение, вместимым не являющееся, считается ОДНИМ значением:
				 *          параметр, поданный единожды, и параметр, поданный дважды,
				 *          потребителю вместимого различаться не должны. Ноль означает,
				 *          что параметр не уложен вовсе.
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    число значений вместимого
				 *
				 * \~english
				 * @brief Method of the extraction of the number of the values of a container parameter
				 *
				 * @details A value that is not a container is counted as ONE value: a parameter submitted
				 *          once and a parameter submitted twice must not differ for the consumer of a container.
				 *          A zero means that the parameter is not laid at all.
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    number of the values of the container
				 *
				 * \~
				 */
				size_t size(const string_view key) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода извлечения значения параметра
				 *
				 * @tparam T тип извлекаемого значения
				 *
				 * \~english
				 * @brief Template of the method of the extraction of a value of a parameter
				 *
				 * @tparam T type of the extracted value
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод извлечения значения параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    извлечённое значение параметра
				 *
				 * \~english
				 * @brief Method for retrieving a parameter value
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    extracted value of the parameter
				 *
				 * \~
				 */
				T get(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Шаблон метода извлечения значений вместимого параметра
				 *
				 * @tparam T тип извлекаемых значений
				 *
				 * \~english
				 * @brief Template of the method of the extraction of the values of a container parameter
				 *
				 * @tparam T type of the extracted values
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Метод извлечения значений вместимого параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    извлечённые значения вместимого
				 *
				 * \~english
				 * @brief Method of the extraction of the values of a container parameter
				 *
				 * @param key name of a parameter with the separator of the links
				 * @return    extracted values of the container
				 *
				 * \~
				 */
				vector <T> arr(const string_view key) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения позиционных доводов набора запуска
				 *
				 * @return позиционные доводы в порядке их встречи
				 *
				 * \~english
				 * @brief Method of the extraction of the positional arguments of the set of the launch
				 *
				 * @return positional arguments in the order of their occurrence
				 *
				 * \~
				 */
				const vector <string> & operands() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения дерева собранных значений настроек
				 *
				 * @return дерево собранных значений настроек
				 *
				 * \~english
				 * @brief Method of the extraction of the tree of the assembled values of the settings
				 *
				 * @return tree of the assembled values of the settings
				 *
				 * \~
				 */
				const codec::abc::value_t & root() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения отказов последнего разбора
				 *
				 * @details Отказы КОПЯТСЯ со всех ходов сбора - разбора набора запуска, разбора
				 *          текста, сбора окружения, чтения и записи файла настроек, - и живут до
				 *          очистки ходом `clear()`. Сделано это ради того, что настройки берутся
				 *          из нескольких источников подряд, а судит их потребитель однажды, в
				 *          конце сбора: очищай их всякий разбор, отказ чтения файла настроек,
				 *          случившийся раньше, стирался бы молча.
				 *
				 * @return отказы, накопленные с последней очистки
				 *
				 * \~english
				 * @brief Method of the extraction of the refusals of the last parsing
				 *
				 * @details The refusals ACCUMULATE from all the functions of the gathering - the parsing of
				 *          the set of the launch, the parsing of a text, the gathering of the environment, the reading
				 *          and the writing of the settings file - and live until the clearing by the `clear()` function.
				 *
				 * @return refusals accumulated since the last clearing
				 *
				 * \~
				 */
				const vector <pair <error_t, location_t>> & errors() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения начала имён переменных окружения
				 *
				 * @return начало имён переменных окружения
				 *
				 * \~english
				 * @brief Method of the extraction of the beginning of the names of the variables of the environment
				 *
				 * @return beginning of the names of the variables of the environment
				 *
				 * \~
				 */
				const string & prefix() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки начала имён переменных окружения
				 *
				 * @param prefix начало имён переменных окружения
				 *
				 * \~english
				 * @brief Method of the setting of the beginning of the names of the variables of the environment
				 *
				 * @param prefix beginning of the names of the variables of the environment
				 *
				 * \~
				 */
				void prefix(const string_view prefix) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения настроек сбора параметров запуска
				 *
				 * @return настройки сбора параметров запуска
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the assembling of the parameters of the launch
				 *
				 * @return settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				const settings_t & settings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек сбора параметров запуска
				 *
				 * @param settings настройки сбора параметров запуска
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the assembling of the parameters of the launch
				 *
				 * @param settings settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				void settings(const settings_t & settings) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения настроек разбора параметров
				 *
				 * @return настройки разбора параметров
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the parsing of the parameters
				 *
				 * @return settings of the parsing of the parameters
				 *
				 * \~
				 */
				const lexer_t::settings_t & lexing() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек разбора параметров
				 *
				 * @param settings настройки разбора параметров
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the parsing of the parameters
				 *
				 * @param settings settings of the parsing of the parameters
				 *
				 * \~
				 */
				void lexing(const lexer_t::settings_t & settings) noexcept;
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
				explicit Args() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Args() noexcept = default;
		} args_t;
	}
}
