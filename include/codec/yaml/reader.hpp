/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения текста YAML — разбор блочных построений
 *        стопою отступов с выдачей событий по мере чтения
 *
 * \~english
 * @brief Header file of the streaming reading of a YAML text — the parsing of the block constructions
 *        by a stack of the indentations with the issuance of the events as the reading goes on
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_YAML_READER__
#define __AWH_CODEC_YAML_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

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
			 * @brief Состояния потокового чтения текста
			 *
			 * \~english
			 * @brief States of the streaming reading of a text
			 *
			 * \~
			 */
			enum class state_t : uint8_t {
				READY    = 0x00, // Чтение заведено, текст ещё не подавался
				PARSING  = 0x01, // Текст разбирается, продолжения ожидается
				FINISHED = 0x02, // Текст разобран до конца
				FAILED   = 0x03  // Разбор прекращён отказом
			};

			/**
			 * \~russian
			 * @brief Значение, выданное событием чтения
			 *
			 * @details Поле содержимого ссылается на память, принадлежащую хранилищу
			 * чтения, и живёт до получения следующего события
			 *
			 * \~english
			 * @brief Value issued by an event of the reading
			 * @details The field of the content refers to the memory belonging to the storage
			 * of the reading and lives until the obtaining of the next event
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Content {
				// Вид значения, разрешённый действующей схемой
				type_t type;
				// Вид записи значения в исходном тексте
				style_t style;
				// Содержимое значения, приведённое к окончательному виду
				string_view text;
				/**
				 * Имя метки, значению предпосланной, пусто при отсутствии её
				 *
				 * @note Событие ссылки несёт имя её полем содержимого, а не полем этим: ссылка
				 *       сама значением не является, и содержимое её есть имя метки, на которую
				 *       она указывает
				 */
				string_view anchor;
				/**
				 * Метка типа, значению предпосланная, приведённая к полному виду
				 *
				 * @note Сокращение раскрывается объявлением своим: `!!str` выдаётся записью
				 *       `tag:yaml.org,2002:str`, а `!свой` - записью `!свой`. Потребителю
				 *       незачем знать, каким сокращением метка записана была
				 */
				string_view tag;
				// Положение значения в исходном тексте
				location_t location;
				/**
				 * Признак того, что значение собрано внутри поточного построения
				 *
				 * @note Записи поточного построения строкою не отделены: их стоит на строке
				 *       сколько угодно, и привязать значение к отрезку строк нельзя. Держащему
				 *       документ целиком признак этот велит не переносить такое значение
				 *       дословно, а собирать заново
				 */
				bool flow;
				/**
				 * Схема, действовавшая при выдаче события
				 *
				 * @note Схема эта берётся событием, а не чтением целиком: директива `%YAML 1.1`
				 *       переводит на наречие 1.1 один свой документ, а поток вправе нести
				 *       документы обоих наречий. Спроси схему у чтения по concу разбора - и
				 *       получишь наречие документа последнего, каким бы ни был первый
				 */
				schema_t schema;
				/**
				 * Признак того, что документ наречие своё директивой объявил
				 *
				 * @note Признак этот принадлежит документу, а не потоку: директива есть
				 *       принадлежность документа и стоит перед каждою чертою начала своею.
				 *       Держащему документ целиком он велит вернуть директиву перезаписью
				 *       ровно тем документам, у каких она стояла
				 */
				bool versioned;
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
				Content() noexcept :
				 type(type_t::UNDEFINED), style(style_t::PLAIN), flow(false), schema(schema_t::CORE),
				 versioned(false) {}
			} content_t;

			/**
			 * \~russian
			 * @brief Потоковое чтение текста YAML
			 *
			 * @details Текст подаётся кусками и разбирается построчно: событие выдаётся
			 * лишь тогда, когда строка прочитана целиком и прирасти уже не может. Оттого
			 * выдача не зависит от того, как текст нарезан на куски при подаче
			 *
			 * @warning Заводится чтение по частям, и построения, ещё не заведённые -
			 *          составные имена пар, - отвечают отказом, а не молчаливым разбором
			 *          наугад: молчаливый разбор выдал бы дерево, исходному тексту не
			 *          отвечающее. Имя, само построением являющееся, нуждается в дереве, и
			 *          заводится оно вместе с держащим документ целиком
			 *
			 * @note Ссылки чтением не раскрываются: событие ссылки выдаётся как есть, а
			 *       раскрытие её есть забота держащего документ целиком. Потоковое чтение
			 *       памяти под дерево не отводит вовсе, и раскрывать ему нечего, а предел
			 *       раскрытия `MAX_EXPANSION` оттого стережёт не здесь, а там
			 *
			 * \~english
			 * @brief Streaming reading of a YAML text
			 * @details A text is fed by the chunks and is parsed line by line: an event is issued
			 * only when a line is read in full and can no longer grow. Whereby the issuance
			 * does not depend on how the text is cut into the chunks at the feeding
			 * @warning The reading is being created by the parts, and the constructions not yet created —
			 *          the complex keys of the pairs — answer with a refusal rather than with a silent parsing
			 *          at random: a silent parsing would issue a tree not corresponding to the source text
			 * @note The aliases are not expanded by the reading: an event of an alias is issued as it is,
			 *       and the expansion of it is the concern of the one holding a document in full
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора текста
					 *
					 * \~english
					 * @brief Settings of the parsing of a text
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
						// Признак выдачи примечаний отдельным событием
						bool emitComments;
						// Признак выдачи пустых строк отдельным событием
						bool emitBlanks;
						// Наибольшая допустимая глубина вложенности, ноль - предел по умолчанию
						uint32_t depth;
						// Наибольшая допустимая длина скалярного значения, ноль - предел по умолчанию
						uint32_t scalar;
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
						 schema(schema_t::CORE), encoding(encoding_t::NONE), duplicates(duplicate_t::ERROR),
						 emitComments(false), emitBlanks(false), depth(0), scalar(0) {}
					} settings_t;
				private:
					/**
					 * \~russian
					 * @brief Виды открытых уровней вложенности
					 *
					 * \~english
					 * @brief Kinds of the opened levels of the nesting
					 *
					 * \~
					 */
					enum class nesting_t : uint8_t {
						MAPPING  = 0x00, // Уровень отображения пар
						SEQUENCE = 0x01  // Уровень перечня значений
					};
					/**
					 * \~russian
					 * @brief Состояния разбора поточного построения
					 *
					 * \~english
					 * @brief States of the parsing of a flow construction
					 *
					 * \~
					 */
					enum class flow_t : uint8_t {
						ENTRY = 0x00, // Ожидается очередное значение либо закрывающая скобка
						AFTER = 0x01  // Ожидается запятая, двоеточие либо закрывающая скобка
					};
					/**
					 * \~russian
					 * @brief Открытое поточное построение
					 *
					 * \~english
					 * @brief Opened flow construction
					 *
					 * \~
					 */
					typedef struct Bracket {
						// Вид открытого поточного построения
						nesting_t kind;
						// Признак того, что построение уже несёт значения
						bool filled;
						/**
						 * Признак того, что разбирается значение пары, а не имя её
						 *
						 * @note Отображение `{a, b}` описанием дозволено: значения пар пусты. Отличить
						 *       имя без значения от значения можно лишь признаком этим - двоеточие,
						 *       имя от значения отделяющее, стоит уже позади
						 */
						bool valued;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param kind вид открытого поточного построения
						 *
						 * \~english
						 * @brief Constructor
						 * @param kind kind of the opened flow construction
						 *
						 * \~
						 */
						Bracket(const nesting_t kind) noexcept : kind(kind), filled(false), valued(false) {}
					} bracket_t;
					/**
					 * \~russian
					 * @brief Открытый уровень вложенности
					 *
					 * \~english
					 * @brief Opened level of the nesting
					 *
					 * \~
					 */
					typedef struct Level {
						// Вид открытого уровня вложенности
						nesting_t kind;
						// Отступ, на котором уровень открыт
						uint32_t indent;
						/**
						 * Признак того, что уровень открыт значением пары, стоящим на отступе имени её
						 *
						 * @note Перечень, стоящий на отступе имени своей пары, описанием дозволен, и
						 *       закрытием по отступу он не снимается: снимает его следующая пара того
						 *       же отображения. Уровень же, на том отступе открытый сам по себе, есть
						 *       смешение перечня с отображением, и его надлежит отвергнуть
						 */
						bool implied;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param kind    вид открытого уровня вложенности
						 * @param indent  отступ, на котором уровень открыт
						 * @param implied признак открытия уровня значением пары на отступе имени её
						 *
						 * \~english
						 * @brief Constructor
						 * @param kind kind of the opened level of the nesting
						 * @param indent indentation at which the level is opened
						 *
						 * \~
						 */
						Level(const nesting_t kind, const uint32_t indent, const bool implied) noexcept :
						 kind(kind), indent(indent), implied(implied) {}
					} level_t;
					/**
					 * \~russian
					 * @brief Событие разбора, собранное для выдачи
					 *
					 * \~english
					 * @brief Event of the parsing assembled for the issuance
					 *
					 * \~
					 */
					typedef struct Piece {
						// Смещение куска в хранилище знаков
						size_t offset;
						// Длина куска в хранилище знаков
						size_t length;
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
						Piece() noexcept : offset(0), length(0) {}
					} piece_t;
					/**
					 * \~russian
					 * @brief Событие разбора, собранное для выдачи
					 *
					 * \~english
					 * @brief Event of the parsing assembled for the issuance
					 *
					 * \~
					 */
					typedef struct Item {
						// Вид собранного события
						event_t event;
						// Вид значения, разрешённый действующей схемой
						type_t type;
						// Вид записи значения в исходном тексте
						style_t style;
						// Смещение содержимого события в хранилище знаков
						size_t offset;
						// Длина содержимого события в хранилище знаков
						size_t length;
						// Имя метки, событию предпосланной, в хранилище знаков
						piece_t anchor;
						// Метка типа, событию предпосланная, в хранилище знаков
						piece_t tag;
						// Положение события в исходном тексте
						location_t location;
						// Признак того, что событие собрано внутри поточного построения
						bool flow;
						// Схема, действовавшая при выдаче события
						schema_t schema;
						// Признак того, что документ наречие своё директивой объявил
						bool versioned;
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
						Item() noexcept :
						 event(event_t::NONE), type(type_t::UNDEFINED), style(style_t::PLAIN),
						 offset(0), length(0), flow(false), schema(schema_t::CORE), versioned(false) {}
					} item_t;
				private:
					// Настройки разбора текста
					settings_t _settings;
					// Объект приведения кодировки исходного текста
					decoder_t _decoder;
					// Состояние потокового чтения текста
					state_t _state;
					// Код ошибки разбора текста
					error_t _error;
					// Положение отказа разбора в исходном тексте
					location_t _location;
					// Накопитель поданного текста, приведённого к UTF-8
					string _buffer;
					// Смещение начала неразобранного текста в накопителе
					size_t _offset;
					// Хранилище содержимого собранных событий
					string _storage;
					// Очередь собранных событий разбора, готовых к выдаче
					/**
					 * Очередь событий, выдачи ожидающих
					 *
					 * @details Держится она перечнем с указателем на выданное, а не двусторонней
					 * очередью: очередь освобождает опустевший кусок памяти и заводит его заново
					 * приходом следующего события, отчего разбор крупного текста обходился в
					 * десятки тысяч выделений. Перечень же память свою переиспользует, и
					 * выделений на весь разбор приходится десяток
					 */
					vector <item_t> _events;
					// Номер очередного события очереди, выдачи ожидающего
					size_t _reading;
					/**
					 * События разбираемой строки, выдачи ещё не ожидающие
					 *
					 * @note Строка собирается целиком и лишь затем переносится в очередь выдачи:
					 *       отказ посреди строки не вправе оставить потребителя с началом
					 *       построения без конца его, а исход разбора не вправе зависеть от того,
					 *       на каком куске отказ случился
					 */
					vector <item_t> _staged;
					// Событие, выданное последним
					item_t _current;
					// Значение, выданное последним событием
					content_t _content;
					// Стопа открытых уровней вложенности
					vector <level_t> _levels;
					// Номер разбираемой строки, считая с единицы
					uint32_t _line;
					// Смещение от начала текста в байтах
					uint64_t _position;
					// Признак того, что начало потока уже выдано
					bool _started;
					// Признак того, что документ открыт
					bool _opened;
					// Признак того, что открытый документ уже нёс содержимое
					bool _filled;
					// Признак того, что собирается блочное значение
					bool _blocking;
					// Вид собираемого блочного значения
					style_t _block;
					// Правило усечения переводов строк собираемого блочного значения
					chomp_t _chomp;
					// Отступ, заданный заголовком блочного значения, ноль - отступ по первой строке
					uint8_t _marked;
					// Отступ строки, заголовок блочного значения несущей
					uint32_t _outer;
					// Отступ разбираемой строки
					uint32_t _margin;
					/**
					 * \~russian
					 * Количество строк, к разбираемой присоединённых склейкой
					 *
					 * @details Значение огранённое вправе стоять в несколько строк, и строки те
					 * сводятся в одну логическую прежде разбора. Счёт строк ведётся по строкам
					 * телесным, а не логическим, оттого присоединённые и учитываются отдельно
					 *
					 * @note Место события, за таким значением стоящего, называет строкою ту, в
					 *       какой значение началось: логическая строка одна, и столбец в ней
					 *       считается от начала её
					 *
					 * \~english
					 * Number of the lines joined to the one being parsed
					 * @details A quoted value is entitled to stand in several lines, and those lines are
					 * brought into a single logical one before the parsing. The count of the lines is conducted over
					 * the physical lines rather than over the logical ones, hence the joined ones are counted separately
					 * @note The place of an event standing after such a value names the line in which the value
					 *       began: the logical line is one, and the column in it is counted from its beginning
					 *
					 * \~
					 */
					uint32_t _joined;
					// Признак того, что логическая строка несёт огранённое значение в несколько строк
					bool _stretched;
					// Наименьший отступ строк, склейкой присоединённых
					uint32_t _shallow;
					/**
					 * \~russian
					 * Признак того, что имя пары объявлено вопросом составного имени
					 *
					 * @details Составное имя объявляется вопросом, а значение его - двоеточием
					 * строкою ниже, на том же отступе. Признак этот и отличает двоеточие
					 * значения от двоеточия пустого имени: без него написание `: значение`
					 * читалось бы парою с именем пустым
					 *
					 * @note Признак держится до прихода значения либо до строки, ожидание
					 *       обрывающей: имя без значения своего пустоту получает, ровно как её
					 *       получает имя, двоеточием объявленное
					 *
					 * \~english
					 * Sign that the name of a pair is declared by the question of an explicit key
					 * @details An explicit key is declared by a question, and its value — by a colon a line below,
					 * at the same indentation. This sign distinguishes the colon of a value from the colon of
					 * an empty name: without it the record `: value` would be read as a pair with an empty name
					 * @note The sign is held until the arrival of the value or until a line breaking the expectation:
					 *       a name without its value receives an emptiness, exactly as does a name declared by a colon
					 *
					 * \~
					 */
					bool _asked;
					// Отступ, на котором стоит вопрос составного имени
					uint32_t _questioned;
					/**
					 * \~russian
					 * Признак того, что документ открыт чертою начала своего
					 *
					 * @details Документ, чертою открытый и содержимого не получивший, узел свой
					 * всё же имеет: описание берёт его правилом `e-node` - пустым узлом. Документ
					 * же, черты не имеющий, без содержимого не заводится вовсе
					 *
					 * \~english
					 * Sign that a document is opened by the dashes of its beginning
					 * @details A document opened by the dashes and which has not received a content still has
					 * its node: the description takes it by the `e-node` rule — by an empty node. A document
					 * without the dashes, on the contrary, is not created at all without a content
					 *
					 * \~
					 */
					bool _dashed;
				private:
					/**
					 * \~russian
					 * Положение, на котором ожидается значение записи либо пары
					 *
					 * @details Пустое значение выдаётся не там, где оно ожидалось, а там, где
					 * ожидание оборвалось - строкою ниже, чертою следующей записи. Место
					 * события есть место значения, и без этого положения пустота записи `-`
					 * получала бы место соседа
					 *
					 * @note Наружу это торчит удержанием исходного текста: начала записей
					 * узлов сдвигались на строку, и перезапись правки теряла запись перечня
					 *
					 * \~english
					 * Position at which a value of an entry or of a pair is expected
					 * @details An empty value is issued not where it was expected but where
					 * the expectation broke off — a line below, at the dash of the next entry. The place
					 * of the event is the place of the value, and without this position the emptiness of the entry `-`
					 * would receive the place of its neighbour
					 * @note Outwards this sticks out through the retention of the source text: the beginnings of the records
					 * of the nodes shifted by a line, and a rewriting of an edit lost an entry of the sequence
					 *
					 * \~
					 */
					location_t _awaiting;
					// Отступ содержимого собираемого блочного значения
					uint32_t _inner;
					// Положение заголовка блочного значения в строке
					size_t _opening;
					// Собираемое содержимое блочного значения
					string _block_text;
					// Количество пустых строк, содержимого ещё не дождавшихся
					size_t _breaks;
					/**
					 * Наибольший отступ пустых строк, содержимому блочного значения предпосланных
					 *
					 * @note Стандарт запрещает предпосланной пустой строке стоять глубже первой
					 *       непустой строки содержимого: отступ содержимого берётся именно по
					 *       ней, и строка, её глубже, оказалась бы содержимым, отступ которого
					 *       заголовком не задан и первою строкой не подтверждён
					 */
					uint32_t _padding;
					/**
					 * Признак того, что последняя присоединённая строка стояла глубже отступа содержимого
					 *
					 * @note Свёртка строк применяется лишь тогда, когда обе строки, перевод
					 *       разделяющий, стоят на отступе содержимого: строка, стоящая глубже,
					 *       свёртке не подлежит ни собою, ни соседкой своей, и переводы вокруг
					 *       неё сохраняются как есть
					 */
					bool _deepened;
					// Признак того, что ожидается значение пары, объявленной прежде
					bool _expected;
					/**
					 * Признак того, что строка подаёт значение пары, объявленной прежде
					 *
					 * @note Признак ожидания гасится разбором строки прежде разбора содержимого
					 *       её, и содержимому нечем отличить значение пары, строкою ниже имени
					 *       её стоящее, от записи, глубже уже завершённой пары стоящей.
					 *       Написание первое описанием дозволено, второе - запрещено
					 */
					bool _awaited;
					/**
					 * Признак того, что разбирается значение пары, в той же строке стоящее
					 *
					 * @note Описание дозволяет строке блочного построения нести одно имя пары:
					 *       запись `a: b: c` двусмысленна, и разбор её наугад выдал бы дерево,
					 *       исходному тексту не отвечающее
					 */
					bool _valued;
					/**
					 * Признак разбора содержимого на строке черты начала документа
					 *
					 * @note Блочное построение строки этой не занимает: отступ его
					 *       отсчитывался бы от черты, а не от начала строки
					 */
					bool _headed;
					/**
					 * Признак разбора свойств узла в разбираемой строке
					 *
					 * @note Свойства блочного построения отделяются от него переводом
					 *       строки, и признак этот отличает написание `&метка - запись`
					 *       от написания той же метки строкою выше черты
					 */
					bool _propped;
					/**
					 * Признак подачи в отступе вложенного построения
					 *
					 * @note Отступ вложенного построения задан правилом `s-indent`
					 *       описания, а тот пробелами набирается: подача отступом не
					 *       является, и вложенному построению отступа не даёт
					 */
					bool _tabbed;
					/**
					 * Признак того, что блочное значение стоит содержимым документа целиком
					 *
					 * @note Построений над ним нет, и содержимое его вправе стоять с
					 *       начала строки: описание берёт там отступ мельче нулевого
					 */
					bool _rooted;
					// Признак того, что отступ содержимого блочного значения определён
					bool _detected;
					/**
					 * Признак того, что ожидаемое значение принадлежит записи перечня, а не паре
					 *
					 * @note Различие это в одном: черта на отступе ожидания есть для пары
					 *       значение её, а для записи перечня - запись следующая, и пустоту
					 *       прежней записи надлежит выдать прежде неё
					 */
					bool _entered;
					// Отступ, на котором ожидается значение пары либо записи перечня
					uint32_t _pending;
					/**
					 * Схема разрешения видов, действующая над разбираемым документом
					 *
					 * @note Схема эта берётся из настроек разбора, а директива `%YAML 1.1`
					 *       правит её на схему наречия 1.1 - но лишь тогда, когда потребитель
					 *       схему свою не назначил: назначенное потребителем прямо текст
					 *       перебивать не вправе
					 */
					schema_t _schema;
					// Имя метки, узлу предпосланной, ожидающее узла своего
					string _anchor;
					// Метка типа, узлу предпосланная, ожидающая узла своего
					string _tag;
					// Имена меток, объявленных разбираемым документом
					unordered_set <string> _anchors;
					// Сокращения меток типов, объявленные директивами документа
					unordered_map <string, string> _handles;
					/**
					 * Признак того, что собирается простое значение, могущее прирасти
					 *
					 * @note Простое значение, добежавшее до конца строки, вправе продолжиться
					 *       строкою ниже, и оттого выдача его откладывается до строки той:
					 *       выдать его сразу значило бы выдать половину значения
					 */
					bool _plaining;
					// Собираемое содержимое простого значения
					string _plain;
					// Количество пустых строк, содержимого ещё не дождавшихся
					size_t _folds;
					// Отступ, который обязано превышать продолжение простого значения
					uint32_t _required;
					// Положение начала простого значения в исходном тексте
					location_t _origin;
					// Стопа открытых поточных построений
					vector <bracket_t> _flow;
					// Состояние разбора поточного построения
					flow_t _phase;
					// Признак того, что документу предпосланы директивы
					bool _directed;
					// Признак того, что наречие документа объявлено директивой
					bool _versioned;
				private:
					/**
					 * \~russian
					 * Признак того, что наречие объявлялось директивой хоть раз за текст
					 *
					 * @details Признак `_versioned` сбрасывается закрытием всякого документа, и
					 * после разбора он всегда ложен. Перезаписи же знать надобно, стояла ли
					 * директива в тексте вообще, - оттого признак этот и заведён отдельно
					 *
					 * \~english
					 * Sign that the dialect was declared by a directive at least once over the text
					 * @details The `_versioned` sign is reset by the closing of every document, and
					 * after the parsing it is always false. The rewriting, however, needs to know whether
					 * a directive stood in the text at all — that is why this sign is created separately
					 *
					 * \~
					 */
					bool _declared;
				private:
					// Схема, директивой наречия назначенная
					schema_t _dialect;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа разбора
					 *
					 * @param error  код ошибки разбора
					 * @param column положение отказа в разбираемой строке
					 * @return       признак прекращения разбора
					 *
					 * \~english
					 * @brief Method of the declaration of a refusal of the parsing
					 * @param error error code of the parsing
					 * @param column position of the refusal in the line being parsed
					 * @return sign of the termination of the parsing
					 *
					 * \~
					 */
					bool fail(const error_t error, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса собранных событий строки в очередь выдачи
					 *
					 * \~english
					 * @brief Method of the transfer of the assembled events of a line into the queue of the issuance
					 *
					 * \~
					 */
					void commit() noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки события примечания
					 *
					 * @details Содержимое примечания выдаётся без пробельной обвязки с обеих
					 * сторон - тем же порядком, каким выдаёт его контейнер TOML: расхождение
					 * между кодеками в столь мелком вопросе всплыло бы у потребителя, читающего
					 * оба
					 *
					 * @param line     разбираемая строка
					 * @param position положение знака примечания в строке
					 *
					 * \~english
					 * @brief Method of the placing of an event of a comment
					 * @details The content of a comment is issued without the whitespace framing on both
					 * sides — by the same order by which the TOML container issues it: a divergence
					 * between the codecs in such a small question would surface at a consumer reading
					 * both of them
					 * @param line line being parsed
					 * @param position position of the comment character in the line
					 *
					 * \~
					 */
					void remark(const string_view line, const size_t position) noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки собранного события в очередь выдачи
					 *
					 * @param event  вид собранного события
					 * @param column положение события в разбираемой строке
					 * @return       ссылка на поставленное событие
					 *
					 * \~english
					 * @brief Method of the placing of an assembled event into the queue of the issuance
					 * @param event kind of the assembled event
					 * @param column position of the event in the line being parsed
					 * @return reference to the placed event
					 *
					 * \~
					 */
					item_t & emit(const event_t event, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки события скалярного значения
					 *
					 * @param text   содержимое скалярного значения
					 * @param style  вид записи значения в исходном тексте
					 * @param column положение значения в разбираемой строке
					 * @return       признак успешной постановки события
					 *
					 * \~english
					 * @brief Method of the placing of an event of a scalar value
					 * @param text content of the scalar value
					 * @param style kind of the notation of the value in the source text
					 * @param column position of the value in the line being parsed
					 * @return sign of the successful placing of the event
					 *
					 * \~
					 */
					bool scalar(const string & text, const style_t style, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытых уровней глубже заданного отступа
					 *
					 * @param indent отступ, до которого закрываются уровни
					 * @param column положение закрытия в разбираемой строке
					 * @return       признак успешного закрытия уровней
					 *
					 * \~english
					 * @brief Method of the closing of the opened levels deeper than a given indentation
					 * @param indent indentation down to which the levels are closed
					 * @param column position of the closing in the line being parsed
					 * @return sign of the successful closing of the levels
					 *
					 * \~
					 */
					bool collapse(const uint32_t indent, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия уровня вложенности заданного вида
					 *
					 * @param kind    вид открываемого уровня вложенности
					 * @param indent  отступ, на котором уровень открывается
					 * @param implied признак открытия уровня значением пары на отступе имени её
					 * @param column  положение открытия в разбираемой строке
					 * @return       признак успешного открытия уровня
					 *
					 * \~english
					 * @brief Method of the opening of a level of the nesting of a given kind
					 * @param kind kind of the level of the nesting being opened
					 * @param indent indentation at which the level is opened
					 * @param column position of the opening in the line being parsed
					 * @return sign of the successful opening of the level
					 *
					 * \~
					 */
					bool expand(const nesting_t kind, const uint32_t indent, const bool implied, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод сличения метки типа с видом открываемого построения
					 *
					 * @details Сличение это едино для построений блочных и поточных: метка `!!seq`
					 * над отображением есть расхождение объявленного с записанным где угодно, и
					 * два свода правил разошлись бы у потребителя, читающего оба написания
					 *
					 * @param kind   вид открываемого построения
					 * @param column положение открытия в разбираемой строке
					 * @return       признак соответствия метки типа виду построения
					 *
					 * \~english
					 * @brief Method of the matching of a tag against a kind of a construction being opened
					 * @param kind kind of the construction being opened
					 * @param column position of the opening in the line being parsed
					 * @return sign of the correspondence of the tag to the kind of the construction
					 *
					 * \~
					 */
					bool matched(const nesting_t kind, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия открытого документа
					 *
					 * @param column положение закрытия в разбираемой строке
					 * @return       признак успешного закрытия документа
					 *
					 * \~english
					 * @brief Method of the closing of an opened document
					 * @param column position of the closing in the line being parsed
					 * @return sign of the successful closing of the document
					 *
					 * \~
					 */
					bool finish(const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод снятия ограды со скалярного значения
					 *
					 * @details Ограда снимается вместе с отменяющими последовательностями
					 * двойной ограды и удвоением кавычки одинарной
					 *
					 * @param text   разбираемая запись значения вместе с оградою
					 * @param style  вид ограды разбираемого значения
					 * @param column положение значения в разбираемой строке
					 * @param result строка, куда помещается содержимое значения
					 * @return       признак успешного снятия ограды
					 *
					 * \~english
					 * @brief Method of the removal of the quoting from a scalar value
					 * @details The quoting is removed together with the escape sequences of a double
					 * quoting and the doubling of the quote of a single one
					 * @param text record of the value being parsed together with the quoting
					 * @param style kind of the quoting of the value being parsed
					 * @param column position of the value in the line being parsed
					 * @param result string into which the content of the value is placed
					 * @return sign of the successful removal of the quoting
					 *
					 * \~
					 */
					bool unquote(const string_view text, const style_t style, const size_t column, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска конца скалярного значения в разбираемой строке
					 *
					 * @details Правила окончания сведены в одно тело: простое значение
					 * оканчивается разделителем пары, знаком примечания за пробельным знаком
					 * либо концом строки, а обнесённое оградою - закрывающей оградой
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала значения в строке
					 * @param style  вид записи значения, определяемый первым знаком
					 * @param length длина записи значения вместе с оградою
					 * @return       признак того, что значение прочитано целиком
					 *
					 * \~english
					 * @brief Method of the search for the end of a scalar value in the line being parsed
					 * @details The rules of the termination are gathered into one body: a plain value
					 * ends by the separator of a pair, by the comment character after a whitespace character
					 * or by the end of the line, while a quoted one — by the closing quoting
					 * @param line line being parsed
					 * @param offset offset of the beginning of the value in the line
					 * @param style kind of the notation of the value determined by the first character
					 * @param length length of the record of the value together with the quoting
					 * @return sign of the fact that the value is read in full
					 *
					 * \~
					 */
					bool bounds(const string_view line, const size_t offset, style_t & style, size_t & length) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора одной логической строки текста
					 *
					 * @param line разбираемая строка без знака конца строки
					 * @return     признак успешного разбора строки
					 *
					 * \~english
					 * @brief Method of the parsing of one logical line of a text
					 * @param line line being parsed without the line break character
					 * @return sign of the successful parsing of the line
					 *
					 * \~
					 */
					bool record(const string_view line) noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса накопленных свойств узла в собранное событие
					 *
					 * @details Метка и метка типа стоят прежде узла своего и ожидают его - быть
					 * может, и не в той строке, где записаны. Событие узла забирает их себе, и
					 * ожидание тем прекращается
					 *
					 * @param item событие, свойства узла принимающее
					 *
					 * \~english
					 * @brief Method of the transfer of the accumulated properties of a node into an assembled event
					 * @details An anchor and a tag stand before their node and await it — perhaps
					 * not even in the line where they are written. An event of a node takes them to itself,
					 * and the awaiting is thereby terminated
					 * @param item event accepting the properties of a node
					 *
					 * \~
					 */
					void attach(item_t & item) noexcept;
					/**
					 * \~russian
					 * @brief Метод разрешения вида скалярного значения
					 *
					 * @details Метка типа перебивает разрешение схемою: `!!str 12` есть строка,
					 * а не число, ибо метка сказана прямо, а схема лишь угадывает по записи
					 *
					 * @param text   содержимое скалярного значения
					 * @param style  вид записи значения в исходном тексте
					 * @param column положение значения в разбираемой строке
					 * @param type   разрешённый вид скалярного значения
					 * @return       признак успешного разрешения вида
					 *
					 * \~english
					 * @brief Method of the resolution of the kind of a scalar value
					 * @details A tag overrides the resolution by the schema: `!!str 12` is a string
					 * rather than a number, for the tag is said directly while the schema only guesses by the notation
					 * @param text content of the scalar value
					 * @param style kind of the notation of the value in the source text
					 * @param column position of the value in the line being parsed
					 * @param type resolved kind of the scalar value
					 * @return sign of the successful resolution of the kind
					 *
					 * \~
					 */
					bool typing(const string_view text, const style_t style, const size_t column, type_t & type) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора свойств узла, стоящих прежде него
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала свойств в строке, по выходе - смещение за ними
					 * @return       признак успешного разбора свойств узла
					 *
					 * \~english
					 * @brief Method of the parsing of the properties of a node standing before it
					 * @param line line being parsed
					 * @param offset offset of the beginning of the properties in the line, at the exit — the offset after them
					 * @return sign of the successful parsing of the properties of a node
					 *
					 * \~
					 */
					bool property(const string_view line, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора ссылки на объявленную метку
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала ссылки в строке, по выходе - смещение за нею
					 * @return       признак успешного разбора ссылки
					 *
					 * \~english
					 * @brief Method of the parsing of an alias to a declared anchor
					 * @param line line being parsed
					 * @param offset offset of the beginning of the alias in the line, at the exit — the offset after it
					 * @return sign of the successful parsing of the alias
					 *
					 * \~
					 */
					bool referred(const string_view line, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора директивы, документу предпосланной
					 *
					 * @details Директивы, чьё имя не опознано, описанием велено пропускать без
					 * отказа: наречия последующие вправе завести свои, и текст, ими писанный,
					 * читающему прежнему понятен остаётся
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала директивы в строке
					 * @return       признак успешного разбора директивы
					 *
					 * \~english
					 * @brief Method of the parsing of a directive placed before a document
					 * @details The directives whose name is not recognised are ordered by the specification to be skipped
					 * without a refusal: the subsequent versions are entitled to introduce their own, and a text
					 * written by them remains understandable to a previous reader
					 * @param line line being parsed
					 * @param offset offset of the beginning of the directive in the line
					 * @return sign of the successful parsing of the directive
					 *
					 * \~
					 */
					bool directive(const string_view line, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора заголовка блочного значения
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение заголовка блочного значения в строке
					 * @param indent отступ строки, заголовок несущей
					 * @return       признак успешного разбора заголовка
					 *
					 * \~english
					 * @brief Method of the parsing of the header of a block scalar
					 * @param line line being parsed
					 * @param offset offset of the header of the block scalar in the line
					 * @param indent indentation of the line carrying the header
					 * @return sign of the successful parsing of the header
					 *
					 * \~
					 */
					bool opening(const string_view line, const size_t offset, const uint32_t indent) noexcept;
					/**
					 * \~russian
					 * @brief Метод присоединения очередной строки к блочному значению
					 *
					 * @details Строка присоединяется, покуда отступ её глубже отступа заголовка;
					 * строка мельче отступом блочное значение завершает и разбирается затем
					 * обычным порядком
					 *
					 * @param line     присоединяемая строка
					 * @param attached признак присоединения строки к блочному значению
					 * @return         признак успешного присоединения строки
					 *
					 * \~english
					 * @brief Method of the attaching of the next line to a block scalar
					 * @details A line is attached as long as its indentation is deeper than the indentation of the header;
					 * a line with a smaller indentation terminates the block scalar and is then parsed
					 * by the usual order
					 * @param line line being attached
					 * @param attached sign of the attaching of the line to the block scalar
					 * @return sign of the successful attaching of the line
					 *
					 * \~
					 */
					bool blocking(const string_view line, bool & attached) noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения собираемого блочного значения
					 *
					 * @param column положение завершения в разбираемой строке
					 * @return       признак успешного завершения блочного значения
					 *
					 * \~english
					 * @brief Method of the termination of a block scalar being assembled
					 * @param column position of the termination in the line being parsed
					 * @return sign of the successful termination of the block scalar
					 *
					 * \~
					 */
					bool closing(const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора поточного построения
					 *
					 * @details Поточные построения записываются скобками, как в JSON, и правила
					 * окончания значений внутри них иные, нежели в блочных: значение
					 * оканчивается запятой либо закрывающей скобкой, а не одним лишь концом
					 * строки
					 *
					 * @details Стопа открытых скобок держится полем, а не возвратностью вызовов:
					 * построение вправе растянуться на многие строки, и разбор его обязан
					 * прерваться концом строки и продолжиться со следующей ровно с того места,
					 * где прервался
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала разбора в строке, по выходе - смещение за ним
					 * @return       признак успешного разбора построения
					 *
					 * \~english
					 * @brief Method of the parsing of a flow construction
					 * @details The flow constructions are written by the brackets as in JSON, and the rules
					 * of the termination of the values inside them are other than in the block ones: a value
					 * ends by a comma or by a closing bracket rather than by the end of the line
					 * alone
					 * @param line line being parsed
					 * @param offset offset of the beginning of the parsing in the line, at the exit — the offset after it
					 * @return sign of the successful parsing of the construction
					 *
					 * \~
					 */
					bool flowing(const string_view line, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод откладывания выдачи простого значения
					 *
					 * @details Простое значение, добежавшее до конца строки, вправе
					 * продолжиться строкою ниже, и знать о том сейчас нечем: узнаётся это лишь
					 * отступом строки следующей. Выдача оттого откладывается до неё
					 *
					 * @param text   содержимое простого значения
					 * @param column положение значения в разбираемой строке
					 * @return       признак успешного откладывания выдачи
					 *
					 * \~english
					 * @brief Method of the postponement of the issuance of a plain scalar
					 * @details A plain scalar which has run to the end of a line is entitled to
					 * continue on the line below, and there is nothing to know it by now: it is learned
					 * only by the indentation of the next line. The issuance is therefore postponed until it
					 * @param text content of the plain scalar
					 * @param column position of the value in the line being parsed
					 * @return sign of the successful postponement of the issuance
					 *
					 * \~
					 */
					bool deferred(const string & text, const size_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи собранного простого значения
					 *
					 * @return признак успешной выдачи значения
					 *
					 * \~english
					 * @brief Method of the issuance of an assembled plain scalar
					 * @return sign of the successful issuance of the value
					 *
					 * \~
					 */
					bool settle() noexcept;
					/**
					 * \~russian
					 * @brief Метод присоединения очередной строки к простому значению
					 *
					 * @details Строка присоединяется, покуда отступ её глубже отступа
					 * построения, значение объемлющего; строка мельче отступом значение
					 * завершает и разбирается затем обычным порядком
					 *
					 * @param line     присоединяемая строка
					 * @param attached признак присоединения строки к простому значению
					 * @return         признак успешного присоединения строки
					 *
					 * \~english
					 * @brief Method of the attaching of the next line to a plain scalar
					 * @details A line is attached as long as its indentation is deeper than the indentation
					 * of the construction enclosing the value; a line with a smaller indentation terminates the value
					 * and is then parsed by the usual order
					 * @param line line being attached
					 * @param attached sign of the attaching of the line to the plain scalar
					 * @return sign of the successful attaching of the line
					 *
					 * \~
					 */
					bool plaining(const string_view line, bool & attached) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора значения внутри поточного построения
					 *
					 * @param line   разбираемая строка
					 * @param offset смещение начала значения в строке, по выходе - смещение за ним
					 * @return       признак успешного разбора значения
					 *
					 * \~english
					 * @brief Method of the parsing of a value inside a flow construction
					 * @param line line being parsed
					 * @param offset offset of the beginning of the value in the line, at the exit — the offset after it
					 * @return sign of the successful parsing of the value
					 *
					 * \~
					 */
					bool flowed(const string_view line, size_t & offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора содержимого строки за отступом
					 *
					 * @param line   разбираемая строка без знака конца строки
					 * @param offset смещение начала содержимого в строке
					 * @param indent отступ, на котором содержимое стоит
					 * @return       признак успешного разбора содержимого
					 *
					 * \~english
					 * @brief Method of the parsing of the content of a line after the indentation
					 * @param line line being parsed without the line break character
					 * @param offset offset of the beginning of the content in the line
					 * @param indent indentation at which the content stands
					 * @return sign of the successful parsing of the content
					 *
					 * \~
					 */
					bool content(const string_view line, const size_t offset, const uint32_t indent) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек разбора текста
					 *
					 * @return настройки разбора текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the settings of the parsing of a text
					 * @return settings of the parsing of a text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора текста
					 *
					 * @param settings устанавливаемые настройки разбора
					 * @return         признак принятия настроек разбора
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a text
					 * @param settings settings of the parsing being set
					 * @return sign of the acceptance of the settings of the parsing
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи очередного куска исходного текста
					 *
					 * @param buffer подаваемый кусок исходного текста
					 * @param size   размер подаваемого куска
					 * @param end    признак того, что текст окончен
					 * @return       признак успешного разбора поданного куска
					 *
					 * \~english
					 * @brief Method of the feeding of the next chunk of the source text
					 * @param buffer chunk of the source text being fed
					 * @param size size of the chunk being fed
					 * @param end sign of the fact that the text is ended
					 * @return sign of the successful parsing of the fed chunk
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи исходного текста целиком
					 *
					 * @param text подаваемый исходный текст
					 * @return     признак успешного разбора поданного текста
					 *
					 * \~english
					 * @brief Method of the feeding of the source text in full
					 * @param text source text being fed
					 * @return sign of the successful parsing of the fed text
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения очередного события разбора
					 *
					 * @return признак того, что событие получено
					 *
					 * \~english
					 * @brief Method of the obtaining of the next event of the parsing
					 * @return sign of the fact that an event is obtained
					 *
					 * \~
					 */
					bool next() noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида последнего события разбора
					 *
					 * @return вид последнего события разбора
					 *
					 * \~english
					 * @brief Method of the obtaining of the kind of the last event of the parsing
					 * @return kind of the last event of the parsing
					 *
					 * \~
					 */
					event_t event() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения значения последнего события разбора
					 *
					 * @return значение последнего события разбора
					 *
					 * \~english
					 * @brief Method of the obtaining of the value of the last event of the parsing
					 * @return value of the last event of the parsing
					 *
					 * \~
					 */
					const content_t & value() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения состояния потокового чтения
					 *
					 * @return состояние потокового чтения текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the state of the streaming reading
					 * @return state of the streaming reading of a text
					 *
					 * \~
					 */
					state_t state() const noexcept;
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
					 * @brief Метод получения опознанной кодировки исходного текста
					 *
					 * @return опознанная кодировка исходного текста
					 *
					 * \~english
					 * @brief Method of the obtaining of the recognised encoding of the source text
					 * @return recognised encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения схемы, над разбором действующей
					 *
					 * @details Схема эта настройками задана не всегда: директива `%YAML 1.1`
					 * переводит разбор на схему наречия 1.1, и записи вроде `on` становятся
					 * логическими. Перезапись такого текста обязана директиву сохранить -
					 * иначе `on` вернётся строкою, и круговой ход переменит смысл
					 *
					 * \~english
					 * @brief Method of the extraction of the schema acting over the parsing
					 * @details This schema is not always set by the settings: the `%YAML 1.1` directive
					 * switches the parsing to the schema of the 1.1 dialect, and the records like `on` become
					 * logical ones. A rewriting of such a text must preserve the directive —
					 * otherwise `on` will return as a string, and the round trip will change the meaning
					 *
					 * \~
					 * @return схема, над разбором действующая
					 */
					schema_t dialect() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки объявления наречия директивой
					 *
					 * @return признак объявления наречия директивой
					 *
					 * \~english
					 * @brief Method of the check of the declaration of the dialect by a directive
					 * @return sign of the declaration of the dialect by a directive
					 *
					 * \~
					 */
					bool declared() const noexcept;
					/**
					 * \~russian
					 * @brief Метод сброса состояния потокового чтения
					 *
					 * \~english
					 * @brief Method of the reset of the state of the streaming reading
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
					Reader() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки разбора текста
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the parsing of a text
					 *
					 * \~
					 */
					explicit Reader(const settings_t & settings) noexcept;
			} reader_t;
		};
	};
};

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_YAML_READER__
