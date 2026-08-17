/**
 * @file writer.hpp
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
 * @brief Заголовочный файл записи текста YAML — сборка блочных и поточных построений
 *        с оградою значений по действующей схеме
 *
 * \~english
 * @brief Header file of the writing of a YAML text — the assembling of the block and of the flow
 *        constructions with the quoting of the values by the acting schema
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_WRITER__
#define __AWH_CODEC_YAML_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
			 * @brief Запись текста YAML
			 *
			 * @details Текст собирается вызовами по мере обхода записываемого: открытие
			 * построения, имя пары, значение, закрытие построения. Отступы, ограда значений
			 * и разделители расставляются сборкой сами
			 *
			 * @note Ширина отступа задаётся пробелами, и нуля она не принимает: знак
			 *       горизонтальной подачи описанием в отступе запрещён прямо, и записать им
			 *       отступ означало бы собрать текст, никаким разбором не читаемый. Тем
			 *       запись эта от записи JSON и разнится - там ноль означает подачу
			 *
			 * \~english
			 * @brief Writing of a YAML text
			 * @details A text is assembled by the calls as the traversal of what is being written goes on:
			 * the opening of a construction, the name of a pair, a value, the closing of a construction.
			 * The indentations, the quoting of the values and the separators are placed by the assembling itself
			 * @note The width of the indentation is set by the spaces and it does not accept a zero:
			 *       a tabulation character is directly forbidden in an indentation by the specification
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * \~russian
					 * @brief Настройки записи текста
					 *
					 * \~english
					 * @brief Settings of the writing of a text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Схема, по которой решается нужда значения в ограде
						schema_t schema;
						// Построение, каким записываются вместилища по умолчанию
						layout_t layout;
						/**
						 * Ограда, какою обносятся строковые значения
						 *
						 * @note Значение `PLAIN` означает не запись без ограды всегда, а решение
						 *       по содержимому: ограда ставится там, где без неё запись прочлась бы
						 *       иначе - числом, признаком либо построением
						 */
						style_t quoting;
						// Ширина отступа в пробелах, ноль не принимается
						uint8_t indent;
						/**
						 * Признак отступа перечня, значением пары стоящего
						 *
						 * @note Оба написания описанием дозволены и читаются одинаково, а
						 *       расходятся лишь видом. Умолчанием взято написание без отступа -
						 *       его пишет большинство и его же выдаёт большинство записывающих
						 */
						bool sequenceIndent;
						// Признак записи черты начала документа
						bool explicitStart;
						// Признак записи черты конца документа
						bool explicitEnd;
						// Признак записи директивы наречия текста
						bool version;
						/**
						 * \~russian
						 * Правило обращения с негодной последовательностью UTF-8
						 *
						 * @details Правило это касается лишь записи содержимого, потребителем
						 * поданного: значения, имени поля, якоря, метки и замечания. Записи,
						 * потоком собираемые самостоятельно, битых байтов нести не могут вовсе
						 *
						 * \~english
						 * Rule of the treatment of a malformed UTF-8 sequence
						 * @details This rule concerns only the writing of the content supplied by the consumer:
						 * a value, a name of a field, an anchor, a tag and a comment. The records assembled
						 * by the stream itself cannot carry the broken bytes at all
						 *
						 * \~
						 */
						malformed_t malformed;
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
					 * @brief Открытое вместилище записи
					 *
					 * \~english
					 * @brief Opened container of the writing
					 *
					 * \~
					 */
					typedef struct Level {
						// Вид открытого вместилища
						kind_t kind;
						// Построение, каким записывается вместилище
						layout_t layout;
						// Отступ, на котором записывается содержимое вместилища
						uint32_t indent;
						// Признак того, что вместилище ещё не получило ни одного значения
						bool empty;
						/**
						 * Признак того, что первая запись вместилища строку продолжает
						 *
						 * @note Вместилище, значением перечня стоящее, открывается сразу за
						 *       чертой значения, и первая запись его стоит в той же строке:
						 *       `- имя: значение` есть отображение внутри перечня
						 */
						bool attached;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param kind     вид открытого вместилища
						 * @param layout   построение, каким записывается вместилище
						 * @param indent   отступ содержимого вместилища
						 * @param attached признак продолжения строки первой записью
						 *
						 * \~english
						 * @brief Constructor
						 * @param kind kind of the opened container
						 * @param layout layout by which the container is written
						 * @param indent indentation of the content of the container
						 * @param attached sign of the continuation of the line by the first record
						 *
						 * \~
						 */
						Level(const kind_t kind, const layout_t layout, const uint32_t indent, const bool attached) noexcept :
						 kind(kind), layout(layout), indent(indent), empty(true), attached(attached) {}
					} level_t;
				private:
					// Настройки записи текста
					settings_t _settings;
					// Собираемый текст
					/**
					 * \~russian
					 * Признак того, что запись отвергнута негодной кодировкой
					 *
					 * @details Признак этот заводится ради изъятия: текст, оборванный на
					 * половине из-за отказа, негоден **вовсе**, и выдавать его молча поток
					 * не вправе. Изъятие такого текста отдаёт пустоту
					 *
					 * @note Ставится он лишь отказом по кодировке, а не всяким отказом:
					 * отказы прочие потребитель разбирает сам, и запись блочного значения
					 * внутри скобок, к примеру, законно отступает к иному оформлению
					 *
					 * \~english
					 * Sign that the writing is refused because of a malformed encoding
					 * @details This sign is created for the sake of the withdrawal: a text cut off in the
					 * middle because of a refusal is **entirely** unsuitable, and the stream has no right to give it away
					 * silently. A withdrawal of such a text gives away emptiness
					 * @note It is set only by a refusal by the encoding rather than by every refusal:
					 * the other refusals are handled by the consumer itself, and the writing of a block value
					 * inside the brackets, for instance, lawfully retreats to another formatting
					 *
					 * \~
					 */
					bool _refused;
				private:
					string _result;
					// Стопа открытых вместилищ записи
					vector <level_t> _levels;
					// Имя метки, узлу предпосланной, ожидающее узла своего
					string _anchor;
					// Метка типа, узлу предпосланная, ожидающая узла своего
					string _tag;
					// Признак того, что имя пары записано, а значение её ещё нет
					bool _keyed;
					// Отступ, следующему вместилищу назначенный, ноль - по ширине настроек
					uint32_t _margin;
					// Признак того, что записанное строку не закрыло
					bool _hanging;
					/**
					 * Признак того, что записывается содержимое блочного значения
					 *
					 * @note Пробелы в конце строки снимаются закрытием её: они содержимым не
					 *       являются, и обратное чтение их не увидит. Строки же блочного значения
					 *       суть содержимое само, и снимать в них нечего
					 */
					bool _verbatim;
					/**
					 * Признак того, что открытая строка перенесена дословно
					 *
					 * @note Строка эта усечению не подлежит: перенесена она исходными байтами, и
					 *       пробелы в конце её суть те же байты. Признак снимается закрытием
					 *       строки - в отличие от признака блочного значения, живущего всю запись
					 *       содержимого его
					 */
					bool _transferred;
					// Признак того, что документ уже открыт
					bool _opened;
					// Количество байтов, изъятых из сборщика за всё время работы
					uint64_t _taken;
				private:
					/**
					 * \~russian
					 * @brief Метод закрытия открытой строки собранного текста
					 *
					 * \~english
					 * @brief Method of the closing of an opened line of the assembled text
					 *
					 * \~
					 */
					void line() noexcept;
					/**
					 * \~russian
					 * @brief Метод добавления разделителя записей к собранному тексту
					 *
					 * @details Разделитель ставится лишь там, где его ещё нет: запись, за
					 * чертой значения стоящая, пробелом уже отделена, а запись за именем пары -
					 * ещё нет. Сличать это в каждом месте порознь значило бы развести места
					 * между собою
					 *
					 * \~english
					 * @brief Method of the addition of a separator of the records to the assembled text
					 * @details A separator is placed only where it is not yet: a record standing after
					 * the dash of a value is already separated by a space, while a record after the name of a pair — not yet
					 *
					 * \~
					 */
					void spaced() noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки записи на своё место в собираемом тексте
					 *
					 * @details Записывается отступ вместилища и черта значения перечня - в
					 * объёме, какого требует место записи
					 *
					 * @return признак допустимости записи в этом месте
					 *
					 * \~english
					 * @brief Method of the placing of a record at its place in the text being assembled
					 * @details The indentation of the container and the dash of a value of a sequence are written —
					 * in the volume required by the place of the record
					 * @return sign of the permissibility of the record at this place
					 *
					 * \~
					 */
					bool enter() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи свойств узла, накопленных прежде него
					 *
					 * \~english
					 * @brief Method of the writing of the properties of a node accumulated before it
					 *
					 * \~
					 */
					void properties() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи скалярного значения выбранной оградою
					 *
					 * @param text  записываемое содержимое значения
					 * @param style ограда, какою обносится значение
					 *
					 * \~english
					 * @brief Method of the writing of a scalar value by a chosen quoting
					 * @param text content of the value being written
					 * @param style quoting by which the value is enclosed
					 *
					 * \~
					 */
					void quoted(const string & text, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения записи к годной кодировке UTF-8
					 *
					 * @details Приведение ведётся тем же телом разбора, каким его ведёт чтение:
					 * второе такое тело разошлось бы с первым, и запись выдавала бы то, чего
					 * чтение не принимает
					 *
					 * @param text     приводимая запись
					 * @param result   переменная, куда помещается приведённая запись
					 * @return         признак пригодности записи к записи
					 *
					 * \~english
					 * @brief Method of the bringing of a record to a valid UTF-8 encoding
					 * @details The bringing is conducted by the same parsing body by which the reading conducts it:
					 * a second such body would diverge from the first one, and the writing would give away what
					 * the reading does not accept
					 * @param text record being brought
					 * @param result variable where the brought record is placed
					 * @return sign of the suitability of the record for the writing
					 *
					 * \~
					 */
					bool sanitize(const string & text, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия вместилища заданного вида
					 *
					 * @param kind   вид открываемого вместилища
					 * @param layout построение, каким записывается вместилище
					 * @return       признак успешного открытия вместилища
					 *
					 * \~english
					 * @brief Method of the opening of a container of a given kind
					 * @param kind kind of the container being opened
					 * @param layout layout by which the container is written
					 * @return sign of the successful opening of the container
					 *
					 * \~
					 */
					bool expand(const kind_t kind, const layout_t layout) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод открытия отображения пар
					 *
					 * @return признак успешного открытия отображения
					 *
					 * \~english
					 * @brief Method of the opening of a mapping of the pairs
					 * @return sign of the successful opening of the mapping
					 *
					 * \~
					 */
					bool mapping() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия отображения пар заданным построением
					 *
					 * @param layout построение, каким записывается отображение
					 * @return       признак успешного открытия отображения
					 *
					 * \~english
					 * @brief Method of the opening of a mapping of the pairs by a given layout
					 * @param layout layout by which the mapping is written
					 * @return sign of the successful opening of the mapping
					 *
					 * \~
					 */
					bool mapping(const layout_t layout) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений
					 *
					 * @return признак успешного открытия перечня
					 *
					 * \~english
					 * @brief Method of the opening of a sequence of the values
					 * @return sign of the successful opening of the sequence
					 *
					 * \~
					 */
					bool sequence() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия перечня значений заданным построением
					 *
					 * @param layout построение, каким записывается перечень
					 * @return       признак успешного открытия перечня
					 *
					 * \~english
					 * @brief Method of the opening of a sequence of the values by a given layout
					 * @param layout layout by which the sequence is written
					 * @return sign of the successful opening of the sequence
					 *
					 * \~
					 */
					bool sequence(const layout_t layout) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого вместилища
					 *
					 * @details Вместилище, значений так и не получившее, записывается скобками:
					 * блочное построение пустоту выразить не в силах, и `{}` есть единственная
					 * запись пустого отображения
					 *
					 * @return признак успешного закрытия вместилища
					 *
					 * \~english
					 * @brief Method of the closing of an opened container
					 * @details A container which has not received any values is written by the brackets:
					 * a block layout is not able to express an emptiness, and `{}` is the only
					 * notation of an empty mapping
					 * @return sign of the successful closing of the container
					 *
					 * \~
					 */
					bool close() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи имени пары отображения
					 *
					 * @param name записываемое имя пары отображения
					 * @return     признак успешной записи имени пары
					 *
					 * \~english
					 * @brief Method of the writing of the name of a pair of a mapping
					 * @param name name of the pair of the mapping being written
					 * @return sign of the successful writing of the name of the pair
					 *
					 * \~
					 */
					bool key(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи метки, следующему узлу предпосылаемой
					 *
					 * @param name записываемое имя метки
					 * @return     признак успешной записи метки
					 *
					 * \~english
					 * @brief Method of the writing of an anchor placed before the next node
					 * @param name name of the anchor being written
					 * @return sign of the successful writing of the anchor
					 *
					 * \~
					 */
					bool anchor(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи метки типа, следующему узлу предпосылаемой
					 *
					 * @details Метка описания, полным видом переданная, записывается
					 * сокращением: запись `tag:yaml.org,2002:str` выйдет как `!!str`, ибо
					 * сокращение это описанием закреплено и понятно всякому читающему
					 *
					 * @param name записываемая метка типа
					 * @return     признак успешной записи метки типа
					 *
					 * \~english
					 * @brief Method of the writing of a tag placed before the next node
					 * @details A tag of the specification passed by the full notation is written
					 * by the shorthand: the notation `tag:yaml.org,2002:str` will come out as `!!str`
					 * @param name tag being written
					 * @return sign of the successful writing of the tag
					 *
					 * \~
					 */
					bool tag(const string & name) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи ссылки на объявленную метку
					 *
					 * @param name имя метки, на которую указывает ссылка
					 * @return     признак успешной записи ссылки
					 *
					 * \~english
					 * @brief Method of the writing of an alias to a declared anchor
					 * @param name name of the anchor to which the alias points
					 * @return sign of the successful writing of the alias
					 *
					 * \~
					 */
					bool alias(const string & name) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи пустого значения
					 *
					 * @return признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of an empty value
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool null() noexcept;
					/**
					 * \~russian
					 * @brief Метод записи логического значения
					 *
					 * @param value записываемое логическое значение
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a boolean value
					 * @param value boolean value being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const bool value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value записываемое строковое значение
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value string value being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const string & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения заданной оградою
					 *
					 * @param value записываемое строковое значение
					 * @param style ограда, какою обносится значение
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a string value by a given quoting
					 * @param value string value being written
					 * @param style quoting by which the value is enclosed
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const string & value, const style_t style) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи строкового значения
					 *
					 * @param value записываемое строковое значение
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a string value
					 * @param value string value being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const char * value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа со знаком
					 *
					 * @param value записываемое целое число
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a signed integer
					 * @param value integer being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const int64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи целого числа без знака
					 *
					 * @param value записываемое целое число
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of an unsigned integer
					 * @param value integer being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const uint64_t value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи дробного числа
					 *
					 * @details Бесконечность и нечисло записываются словами `.inf` и `.nan`:
					 * описание знает их прямо, и отказывать в них незачем - тем запись эта от
					 * записи JSON и разнится, ибо там их нет вовсе
					 *
					 * @param value записываемое дробное число
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a floating point number
					 * @details The infinity and the not-a-number are written by the words `.inf` and `.nan`:
					 * the specification knows them directly, and there is no point in refusing them
					 * @param value floating point number being written
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool value(const double value) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи блочного значения
					 *
					 * @details Блочным значением записывается многострочный текст: ограда
					 * двойная передала бы его отменяющими последовательностями, и читающий
					 * человек в записи той ничего бы не разобрал
					 *
					 * @param text  записываемое содержимое значения
					 * @param style вид блочного значения, дословный либо со свёрткой
					 * @param chomp правило усечения переводов строк в конце значения
					 * @return      признак успешной записи значения
					 *
					 * \~english
					 * @brief Method of the writing of a block scalar
					 * @details A multiline text is written by a block scalar: a double quoting
					 * would pass it by the escape sequences, and a reading human would make out nothing in that notation
					 * @param text content of the value being written
					 * @param style kind of the block scalar, literal or folded
					 * @param chomp rule of the chomping of the line breaks at the end of the value
					 * @return sign of the successful writing of the value
					 *
					 * \~
					 */
					bool block(const string & text, const style_t style, const chomp_t chomp) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи значения дословно, как оно передано
					 *
					 * @details Записанное не проверяется ничем: способ этот отведён под
					 * перенос кусков исходного текста, разбором уже прочитанных, и проверять
					 * их вторично незачем
					 *
					 * @param value записываемое содержимое
					 * @return      признак успешной записи содержимого
					 *
					 * \~english
					 * @brief Method of the writing of a value verbatim as it is passed
					 * @details What is written is not checked by anything: this way is reserved for
					 * the transfer of the pieces of a source text already read by the parsing
					 * @param value content being written
					 * @return sign of the successful writing of the content
					 *
					 * \~
					 */
					bool raw(const string & value) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи примечания собственной строкой
					 *
					 * @param text записываемое содержимое примечания
					 * @return     признак успешной записи примечания
					 *
					 * \~english
					 * @brief Method of the writing of a comment by its own line
					 * @param text content of the comment being written
					 * @return sign of the successful writing of the comment
					 *
					 * \~
					 */
					bool comment(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод записи примечания в конце записанной строки
					 *
					 * @param text записываемое содержимое примечания
					 * @return     признак успешной записи примечания
					 *
					 * \~english
					 * @brief Method of the writing of a comment at the end of a written line
					 * @param text content of the comment being written
					 * @return sign of the successful writing of the comment
					 *
					 * \~
					 */
					bool trailing(const string & text) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия очередного документа потока
					 *
					 * @details Документ второй и всякий следующий открывается чертою всегда:
					 * иначе поток слился бы в один документ
					 *
					 * @return признак успешного открытия документа
					 *
					 * \~english
					 * @brief Method of the opening of the next document of a stream
					 * @details The second document and every next one is always opened by the dash:
					 * otherwise the stream would merge into one document
					 * @return sign of the successful opening of the document
					 *
					 * \~
					 */
					bool document() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения записи текста
					 *
					 * @return признак успешного завершения записи
					 *
					 * \~english
					 * @brief Method of the termination of the writing of a text
					 * @return sign of the successful termination of the writing
					 *
					 * \~
					 */
					bool finish() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения собранного текста
					 *
					 * @return собранный текст
					 *
					 * \~english
					 * @brief Method of the obtaining of the assembled text
					 * @return assembled text
					 *
					 * \~
					 */
					const string & text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия собранного текста из сборщика
					 *
					 * @return изъятый из сборщика текст
					 *
					 * \~english
					 * @brief Method of the taking of the assembled text out of the assembler
					 * @return text taken out of the assembler
					 *
					 * \~
					 */
					string take() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения размера собранного текста
					 *
					 * @return размер собранного текста в байтах
					 *
					 * \~english
					 * @brief Method of the obtaining of the size of the assembled text
					 * @return size of the assembled text in the bytes
					 *
					 * \~
					 */
					size_t size() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения глубины вложенности записи
					 *
					 * @return глубина вложенности открытых вместилищ
					 *
					 * \~english
					 * @brief Method of the obtaining of the depth of the nesting of the writing
					 * @return depth of the nesting of the opened containers
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения отступа, на каком ложится следующая запись
					 *
					 * @return отступ содержимого открытого вместилища в пробелах
					 *
					 * \~english
					 * @brief Method of the obtaining of the indentation at which the next record is laid
					 * @return indentation of the content of the opened container in the spaces
					 *
					 * \~
					 */
					uint32_t indent() const noexcept;
					/**
					 * \~russian
					 * @brief Метод назначения отступа следующему вместилищу
					 *
					 * @details Отступ этот отменяет ширину, настройками заданную, для одного лишь
					 * вместилища, открываемого следующим - ровно как метка да метка типа ждут
					 * своего узла. Служит это перезаписи с удержанием исходного текста: вместилище,
					 * правкой тронутое, собирается заново, а дети его нетронутые переносятся
					 * дословно, и отступ им надобен тот же, на каком они стояли
					 *
					 * @note Значение нулевое отменяет назначение и возвращает ширину настроек
					 *
					 * @param indent назначаемый отступ содержимого вместилища
					 *
					 * \~english
					 * @brief Method of the appointment of an indentation to the next container
					 * @details This indentation overrides the width set by the settings for the one container
					 * being opened next
					 * @note A zero value cancels the appointment and returns the width of the settings
					 * @param indent indentation of the content of the container being appointed
					 *
					 * \~
					 */
					void margin(const uint32_t indent) noexcept;
					/**
					 * \~russian
					 * @brief Метод дословной записи готовых строк текста
					 *
					 * @details Строки переносятся в собираемый текст как есть, без ограды и разбора:
					 * переносящий сам отвечает за то, что они лягут на своё место. Служит это
					 * дословной перезаписи поддеревьев, правкой не тронутых, - и тем сохраняет
					 * примечания, пустые строки да ограду значений, дереву неведомые
					 *
					 * @details Отступ переносится целиком, а не построчно: разность между отступом
					 * исходным и отступом записи прибавляется ко всякой строке, и вложенность строк
					 * между собою тем сохраняется. Разность нулевая - а таков случай перезаписи
					 * без правки - оставляет байты нетронутыми вовсе
					 *
					 * @warning Запись отвергается там, где дословные строки лечь не могут: внутри
					 *          поточного построения, за именем пары, ожидающим значения, и первою
					 *          записью вместилища, строку объемлющей записи продолжающего
					 *
					 * @param text   переносимые строки текста
					 * @param indent отступ, на каком строки стояли в исходном тексте
					 * @return       признак успешного переноса строк
					 *
					 * \~english
					 * @brief Method of the verbatim writing of the ready lines of a text
					 * @details The lines are transferred into the assembled text as they are, without the quoting, the indentation
					 * and the parsing: the one transferring is himself responsible for them falling into their place
					 * @warning The writing is refused where the verbatim lines cannot fall
					 * @param text lines of the text being transferred
					 * @param indent indentation at which the lines stood in the source text
					 * @return sign of the successful transfer of the lines
					 *
					 * \~
					 */
					bool verbatim(const string_view text, const uint32_t indent) noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния записи текста
					 *
					 * \~english
					 * @brief Method of the reset of the state of the writing of a text
					 *
					 * \~
					 */
					void clear() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек записи текста
					 *
					 * @return настройки записи текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the settings of the writing of a text
					 * @return settings of the writing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи текста
					 *
					 * @param settings устанавливаемые настройки записи
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the writing of a text
					 * @param settings settings of the writing being set
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
					Writer() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки записи текста
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the writing of a text
					 *
					 * \~
					 */
					explicit Writer(const settings_t & settings) noexcept;
			} writer_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_WRITER__
