/**
 * @file reader.hpp
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
 * @brief Заголовочный файл потокового чтения текста разметки XML — класс Reader, выдающий события разбора
 *        по мере поступления кусков исходного текста, с разрешением пространств имён,
 *        подстановкой сущностей и проверкой правильности построения
 *
 * \~english
 * @brief Header file of the streaming reading of an XML markup text — the Reader class, which issues the parsing events
 *        as the chunks of the source text arrive, with a resolution of the namespaces,
 *        with a substitution of the entities and with a check of the well-formedness of the construction
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_READER__
#define __AWH_CODEC_XML_READER__

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
#include <sys/log.hpp>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
			 * @brief Состояние чтения текста разметки
			 *
			 * \~english
			 * @brief State of the reading of a markup text
			 *
			 * \~
			 */
			enum class state_t : uint8_t {
				READY     = 0x00, // Событие получено и доступно для чтения
				HUNGRY    = 0x01, // Для продолжения разбора требуется следующий кусок текста
				FINISHED  = 0x02, // Текст разобран до конца
				FAILED    = 0x03  // Разбор прекращён ошибкой
			};

			/**
			 * \~russian
			 * @brief Класс потокового чтения текста разметки
			 *
			 * @details Разбор ведётся по кускам исходного текста и выдаёт события по мере
			 * их обнаружения, не удерживая текст целиком. Приостановка разбора посреди
			 * разметки допустима: разобранное сохраняется, а недостающее дочитывается
			 * следующим куском. Такое устройство позволяет разбирать ответ по мере его
			 * поступления из сети, не дожидаясь его целиком
			 *
			 * Разбор не выбрасывает исключений: признаком отказа служит состояние
			 * @c state_t::FAILED вместе с кодом ошибки и местом отказа в исходном тексте
			 *
			 * @par Порядок работы
			 *
			 * @warning Все выдаваемые последовательности знаков ссылаются на память,
			 * принадлежащую разбору, и остаются пригодными **лишь до следующего обращения**
			 * к @c next() либо @c feed(). Содержимое, нужное дольше, следует скопировать
			 * @note Для разбора текста, уже собранного целиком, есть дерево разметки:
			 * оно устроено поверх этого же чтения и удобнее там, где обход содержимого
			 * важнее расхода памяти
			 *
			 *  @code{.cpp}
			 *  reader_t reader;
			 *
			 *  while(reader.feed(chunk, size, last)){
			 *    while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *        case static_cast <uint8_t> (event_t::ELEMENT_OPEN):
			 *          // Обработка начала узла: reader.name(), reader.attributes()
			 *        break;
			 *        case static_cast <uint8_t> (event_t::TEXT):
			 *          // Обработка содержимого узла: reader.text()
			 *        break;
			 *      }
			 *    }
			 *    if(reader.state() == state_t::FAILED)
			 *      break;
			 *  }
			 *  @endcode
			 *
			 * \~english
			 * @brief Class of the streaming reading of a markup text
			 * @details The parsing is conducted by the chunks of the source text and issues the events as
			 * they are detected without holding the text in full. A suspension of the parsing in the middle of a
			 * markup is admissible: what has been parsed is preserved, while what is missing is read up by the
			 * next chunk. Such an arrangement makes it possible to parse an answer as it
			 * arrives from the network without waiting for it in full
			 * The parsing does not throw exceptions: the @c state_t::FAILED state together with the error code
			 * and the place of the refusal in the source text serves as the sign of a refusal
			 * @par Order of the work
			 * @warning All the issued sequences of characters refer to the memory
			 * belonging to the parsing and remain valid **only until the next call**
			 * to @c next() or @c feed(). The content needed for longer should be copied
			 * @note For the parsing of a text already assembled in full there is the markup tree:
			 * it is built on top of this same reading and is more convenient there where a traversal of the content
			 * is more important than the expenditure of the memory
			 *
			 *  @code{.cpp}
			 *  reader_t reader;
			 *
			 *  while(reader.feed(chunk, size, last)){
			 *    while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *        case static_cast <uint8_t> (event_t::ELEMENT_OPEN):
			 *          // The handling of the beginning of a node: reader.name(), reader.attributes()
			 *        break;
			 *        case static_cast <uint8_t> (event_t::TEXT):
			 *          // The handling of the content of a node: reader.text()
			 *        break;
			 *      }
			 *    }
			 *    if(reader.state() == state_t::FAILED)
			 *      break;
			 *  }
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора текста разметки
					 *
					 * @details Задают строгость разбора и пределы, ограничивающие расход
					 * памяти на подставном тексте
					 *
					 * \~english
					 * @brief Settings of the parsing of a markup text
					 * @details They give the strictness of the parsing and the limits restricting the expenditure
					 * of the memory on a planted text
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Флаг разрешения префиксов по договору о пространствах имён
						bool namespaces;
						// Флаг подстановки ссылок на объявленные сущности
						bool entities;
						// Флаг выдачи примечаний отдельным событием
						bool comments;
						// Флаг выдачи указаний обработчику отдельным событием
						bool processing;
						// Флаг отделения незначимого пробельного содержимого от текстового
						bool separateSpaces;
						/**
						 * \~russian
						 * Флаг склеивания подряд идущих кусков текстового содержимого в одно событие
						 *
						 * @note Склеиваются лишь куски одного и того же вида, идущие подряд.
						 * Раздел дословного текста, примечание и указание обработчику текстовое
						 * содержимое разрывают, даже когда выдача их отдельным событием отключена:
						 * событие для них не выдаётся, но границей содержимого они остаются.
						 * Разборы, не различающие раздел дословного текста и обычный, склеивают
						 * такое содержимое целиком, и слепок разбора с ними тогда не совпадёт
						 *
						 * \~english
						 * Flag of the gluing of the consecutive chunks of a text content into a single event
						 * @note Only the chunks of one and the same kind going in a row are glued.
						 * A literal text section, a comment and a processing instruction break a text
						 * content apart even when their issuance as a separate event is disabled:
						 * an event is not issued for them, but they remain a boundary of the content.
						 * The parsers that do not distinguish a literal text section from an ordinary one glue
						 * such a content in full, and the snapshot of the parsing will then not coincide with theirs
						 *
						 * \~
						 */
						bool mergeText;
						// Флаг подстановки значений атрибутов, объявленных по умолчанию
						bool defaults;
						/**
						 * \~russian
						 * @brief Флаг отказа разбора при ссылке на внешнюю сущность в содержимом
						 *
						 * @details Внешние сущности не загружаются никогда, и подставить такую
						 * ссылку нечем. Договор велит разбору, их не читающему, ссылку РАСПОЗНАТЬ
						 * и пропустить, а не отвергать текст: отказ здесь - строгость сверх
						 * договора, и оттого он вынесен в настройку, а по умолчанию снят
						 *
						 * @note В значении атрибута ссылка на внешнюю сущность запрещена самим
						 * договором, и там отказ следует ВСЕГДА, независимо от этого флага
						 *
						 * \~english
						 * @brief Flag of the refusal of the parsing upon a reference to an external entity in a content
						 * @details External entities are never loaded, and there is nothing to substitute such
						 * a reference with. The standard orders a parser not reading them to RECOGNIZE the reference
						 * and skip it rather than to reject the text: a refusal here is a strictness beyond
						 * the standard, and therefore it is carried out into a setting, being removed by default
						 * @note In an attribute value a reference to an external entity is forbidden by the standard
						 * itself, and a refusal there follows ALWAYS, independently of this flag
						 *
						 * \~
						 */
						bool externals;
						// Наибольшая допустимая глубина вложенности узлов
						uint32_t maxDepth;
						// Наибольшая допустимая длина имени в знаках
						uint32_t maxName;
						// Наибольшее допустимое количество атрибутов у одного узла
						uint32_t maxAttributes;
						// Наибольшее допустимое количество объявленных сущностей
						uint32_t maxEntities;
						/**
						 * \~russian
						 * Наибольший допустимый общий объём подстановки сущностей в байтах
						 *
						 * @warning Поднятие этого предела выше четырёх гигабайт упирается в
						 * разрядность отрезка хранилища: места значений записаны 32 разрядами,
						 * и за этим пределом разбор отвечает отказом `OVERFLOW_LIMIT`. Отказ
						 * заведён намеренно вместо расширения разрядности: без него смещение
						 * обращалось в младшие разряды, и значение атрибута молча выдавалось
						 * чужое - воспроизведено на 34 атрибутах по 128 мегабайт. Разметки,
						 * где значения одного узла превосходят четыре гигабайта, не бывает,
						 * а восьмибайтовое место удвоило бы объём перечня атрибутов у всех
						 *
						 * \~english
						 * Largest admissible total volume of the substitution of the entities in bytes
						 * @warning A raising of this limit above four gigabytes runs into the
						 * width of a segment of the storage: the places of the values are written with 32 bits,
						 * and beyond that limit the parsing answers with an `OVERFLOW_LIMIT` refusal. The refusal
						 * has been introduced deliberately instead of a widening of the width: without it the offset
						 * turned into the low bits, and the value of an attribute was silently issued as
						 * a foreign one — reproduced on 34 attributes of 128 megabytes each. There is no markup
						 * where the values of a single node exceed four gigabytes,
						 * while an eight-byte place would double the volume of the list of the attributes for everyone
						 *
						 * \~
						 */
						uint64_t maxExpansion;
						/**
						 * \~russian
						 * Наибольший допустимый объём одного события в байтах, ноль снимает предел
						 *
						 * @note Предел держится одинаково, как бы исходный текст ни был нарезан.
						 * Сличается с ним и длина разметки события - от её начала до её конца в
						 * исходном тексте, - и объём выданного содержимого. Одного содержимого мало:
						 * у начала узла оно пусто вовсе, у описания типа документа сводится к имени,
						 * а разметка их произвольно длинна, и текст, пришедший целиком, проходил бы
						 * мимо предела там, где тот же текст кусками отвергается. Одной разметки
						 * мало тоже: подстановка сущности содержимое удлиняет. Иначе предел зависел
						 * бы от устройства сети, а не от настроек вызывающего
						 *
						 * @warning Предел ограничивает тем самым и длину открывающей метки со всеми
						 * её атрибутами, и длину описания типа документа: узел с сотней атрибутов
						 * под пределом в сотню байтов не пройдёт, сколько бы ни было содержимого у
						 * самого события
						 *
						 * \~english
						 * Largest admissible volume of a single event in bytes, zero removes the limit
						 * @note The limit is held identically however the source text may be cut.
						 * Both the length of the markup of an event — from its beginning to its end in
						 * the source text — and the volume of the issued content are compared against it. The content alone is not enough:
						 * at the beginning of a node it is empty altogether, at a document type definition it comes down to a name,
						 * while their markup is arbitrarily long, and a text that has arrived in full would pass
						 * by the limit there where the same text by chunks is rejected. The markup alone
						 * is not enough either: a substitution of an entity lengthens the content. Otherwise the limit would depend
						 * on the arrangement of the network rather than on the settings of the caller
						 * @warning The limit thereby restricts also the length of an opening tag with all
						 * its attributes and the length of a document type definition: a node with a hundred attributes
						 * under a limit of a hundred bytes will not pass, however much content the event
						 * itself may have
						 *
						 * \~
						 */
						uint64_t maxEvent;
						// Кодировка, навязанная извне вопреки объявленной в тексте
						encoding_t encoding;
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
					 * @brief Объявление сущности внутреннего подмножества описания типа
					 *
					 * \~english
					 * @brief Entity declaration of the internal subset of a type definition
					 *
					 * \~
					 */
					typedef struct Entity {
						// Подставляемое значение сущности
						string value;
						// Флаг того, что сущность объявлена внешней
						bool external;
						/**
						 * Признак сущности, объявленной НЕРАЗБИРАЕМОЙ (словом «NDATA»)
						 *
						 * @note Ссылка на такую сущность в содержимом запрещена договором прямо
						 * и является ошибкой ВСЕГДА - в отличие от ссылки на обычную внешнюю
						 * сущность, какую разбору, её не читающему, велено пропускать
						 */
						bool unparsed;
						// Флаг того, что подстановка сущности выполняется в текущий миг
						bool active;
						// Флаг того, что значение сущности содержит разметку
						bool markup;
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
						Entity() noexcept : external(false), unparsed(false), active(false), markup(false) {}
					} entity_t;
					/**
					 * \~russian
					 * @brief Значение атрибута, объявленное по умолчанию
					 *
					 * @details Объявляется перечнем атрибутов в описании типа документа и
					 * подставляется тем узлам, где атрибут в тексте опущен
					 *
					 * \~english
					 * @brief Value of an attribute declared by default
					 * @details Declared by a list of the attributes in a document type definition and
					 * substituted to those nodes where the attribute is omitted in the text
					 *
					 * \~
					 */
					typedef struct Default {
						/**
						 * Признак объявления атрибута видом, отличным от «CDATA»
						 *
						 * @details Договор велит приводить значение такого атрибута: снимать
						 * пробельные знаки по краям и сводить всякую их вереницу к одному
						 * пробелу. Приведение это к проверке по описанию типа документа не
						 * относится и выполняется независимо от неё
						 */
						bool tokenized;
						/**
						 * Признак объявления атрибуту значения по умолчанию
						 *
						 * @note Объявление удерживается и без значения: вид атрибута нужен
						 * для приведения значения, записанного в самом тексте
						 */
						bool defaulted;
						// Префикс имени атрибута
						string prefix;
						// Местное имя атрибута
						string local;
						// Значение атрибута по умолчанию
						string value;
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
						Default() noexcept : tokenized(false), defaulted(false) {}
					} default_t;
					/**
					 * \~russian
					 * @brief Ход поиска конца описания типа документа
					 *
					 * @details Описание типа документа приходит кусками наравне с прочей разметкой,
					 * а конец его отыскивается перебором знаков: внутри описания стоят и значения в
					 * кавычках, и примечания, и указания обработчику, и всякий знак конца внутри них
					 * концом описания не является. Перебор с начала при каждом куске обращал бы
					 * разбор описания из линейного в квадратичный, и подмножество в несколько десятков
					 * килобайтов, приходящее мелкими кусками, разбиралось бы сотни раз дольше
					 * пришедшего целиком. Достигнутое положение потому удерживается между кусками
					 *
					 * @note Положения хранятся отсчётом от начала описания, а не от начала
					 * приведённого текста: изъятие разобранного начала буфера сдвигает второе,
					 * а первое оставляет на месте
					 *
					 * \~english
					 * @brief Course of the search of the end of a document type definition
					 * @details A document type definition arrives by chunks on a par with the rest of the markup,
					 * while its end is found by a traversal of the characters: inside a definition there stand both the values in
					 * quotes, and the comments, and the processing instructions, and every ending character inside them
					 * is not the end of the definition. A traversal from the beginning at every chunk would turn
					 * the parsing of a definition from a linear one into a quadratic one, and a subset of several dozen
					 * kilobytes arriving in small chunks would be parsed hundreds of times longer than one
					 * that has arrived in full. The reached position is therefore held between the chunks
					 * @note The positions are stored as a count from the beginning of the definition rather than from the beginning of the
					 * converted text: the withdrawal of the parsed beginning of the buffer shifts the second one,
					 * while it leaves the first one in place
					 *
					 * \~
					 */
					typedef struct Survey {
						// Достигнутое положение поиска от начала описания
						size_t offset;
						// Положение начала внутреннего подмножества от начала описания
						size_t begin;
						// Положение конца внутреннего подмножества от начала описания
						size_t stop;
						// Знак кавычки, которым заключено значение внутри описания
						char quote;
						// Признак того, что поиск находится внутри внутреннего подмножества
						bool inside;
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
						Survey() noexcept : offset(9), begin(string::npos), stop(string::npos), quote(0), inside(false) {}
					} survey_t;
					/**
					 * \~russian
					 * @brief Запись раскладки имени атрибута по свёртке
					 *
					 * @details Поиск повторов среди атрибутов узла сличает имена попарно, и число
					 * сличений растёт квадратом. Раскладка по свёртке имени сводит совпадающие имена
					 * в соседи, после чего сличать требуется лишь их
					 *
					 * \~english
					 * @brief Record of the layout of the name of an attribute by a hash
					 * @details The search of the repetitions among the attributes of a node compares the names pairwise, and the number
					 * of the comparisons grows as a square. The layout by the hash of a name brings the coinciding names
					 * into neighbours, after which only they need to be compared
					 *
					 * \~
					 */
					typedef struct Digest {
						// Свёртка имени атрибута с учётом пространства имён
						uint64_t hash;
						// Положение атрибута в перечне узла
						uint32_t index;
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
						Digest() noexcept : hash(0), index(0) {}
					} digest_t;
					/**
					 * \~russian
					 * @brief Область подставленной в текст сущности
					 *
					 * @details Сущность, значение которой содержит разметку, подставляется
					 * прямо в разбираемый текст: разметка внутри неё разбирается наравне с
					 * записанной в тексте. Область подстановки удерживается, чтобы проверить
					 * договорное требование - узел разметки обязан начинаться и
					 * заканчиваться в пределах одной сущности
					 *
					 * \~english
					 * @brief Region of an entity substituted into the text
					 * @details An entity the value of which contains a markup is substituted
					 * right into the text being parsed: the markup inside it is parsed on a par with the one
					 * written in the text. The region of the substitution is held in order to check
					 * a requirement of the protocol — a markup node is obliged to begin and
					 * to end within the limits of a single entity
					 *
					 * \~
					 */
					typedef struct Splice {
						// Положение конца области подстановки в приведённом тексте
						size_t end;
						// Глубина стека открытых узлов на начало подстановки
						size_t depth;
						// Имя подставленной сущности
						string name;
						// Положение ссылки на сущность в исходном тексте
						location_t location;
						/**
						 * Место разбора, каким оно было бы, будь ссылка на сущность пройдена
						 * без подстановки. Подстановка замещает ссылку значением прямо в
						 * разбираемом тексте, отчего и смещение, и номер строки перестают
						 * отвечать исходному тексту: восстанавливаются они по закрытии области
						 */
						location_t resume;
						// Количество байтов, на которое подстановка удлинила разбираемый текст
						int64_t delta;
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
						Splice() noexcept : end(0), depth(0), delta(0) {}
					} splice_t;
					/**
					 * \~russian
					 * @brief Имя в хранилище знаков разбора
					 *
					 * \~english
					 * @brief Name in the storage of the characters of the parsing
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
					 * @brief Открытый узел разметки
					 *
					 * \~english
					 * @brief Open markup node
					 *
					 * \~
					 */
					typedef struct Element {
						// Имя узла в хранилище знаков разбора
						title_t name;
						// Имя узла в записи, принятой в исходном тексте
						span_t qname;
						// Количество связываний префиксов, объявленных узлом
						uint32_t bindings;
						// Обращение с пробельным содержимым внутри узла
						space_t space;
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
						Element() noexcept : bindings(0), space(space_t::DEFAULT) {}
					} element_t;
					/**
					 * \~russian
					 * @brief Связывание префикса с пространством имён в области видимости
					 *
					 * \~english
					 * @brief Binding of a prefix to a namespace in a scope
					 *
					 * \~
					 */
					typedef struct Scope {
						// Префикс в хранилище знаков разбора
						span_t prefix;
						// Обозначение пространства имён в хранилище знаков разбора
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
					 * @brief Объявление пространства имён узла до разрешения
					 *
					 * \~english
					 * @brief Declaration of the namespace of a node before the resolution
					 *
					 * \~
					 */
					typedef struct Record {
						// Префикс атрибута в исходном тексте
						string_view prefix;
						// Местное имя атрибута в исходном тексте
						string_view local;
						// Значение атрибута в хранилище приведённых значений
						span_t value;
						// Обозначение пространства имён в хранилище знаков разбора
						span_t uri;
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
						Record() noexcept : defaulted(false) {}
					} record_t;
				private:
					// Объект приведения исходного текста к кодировке UTF-8
					decoder_t _decoder;
				private:
					// Признак получения последнего куска исходного текста
					bool _final;
				private:
					// Признак того, что корневой узел разметки уже встречен
					bool _root;
				private:
					// Признак того, что объявление разметки уже встречено
					bool _declared;
				private:
					// Признак того, что описание типа документа уже встречено
					bool _doctype;
				private:
					/**
					 * \~russian
					 * Признак того, что объявления могут приходить извне разобранного
					 *
					 * @note Взводится внешним подмножеством описания типа документа и ссылкой
					 * на параметрическую сущность внутри него: ни того, ни другого разбор не
					 * разворачивает, и объявление сущности вправе лежать там. Пока признак
					 * взведён, недостача объявления построения текста не порочит - договор
					 * относит её к действительности, которую разбор не проверяет
					 *
					 * \~english
					 * Flag of the declarations being able to come from outside what has been parsed
					 * @note Raised by an external subset of a document type definition and by a reference
					 * to a parameter entity inside it: the parsing expands neither the one nor the other,
					 * and an entity declaration has the right to lie there. While the flag is
					 * raised, a missing declaration does not vitiate the construction of the text — the protocol
					 * assigns it to the validity, which the parsing does not check
					 *
					 * \~
					 */
					bool _foreign;
					/**
					 * \~russian
					 * Признак встреченной ссылки на непрочитанную параметрическую сущность
					 *
					 * @details Договор велит разбору, внешних сущностей не читающему, обрабатывать
					 * объявления лишь ДО первой такой ссылки: объявления за нею вправе быть
					 * отменены тем, что лежит внутри непрочитанного, и опираться на них нельзя.
					 * Исключение одно - текст, объявленный самодостаточным
					 *
					 * @note Признак этот отдельный от `_foreign`: тот поднимается и объявлением
					 * внешнего подмножества, какое само по себе объявлений внутреннего
					 * подмножества не отменяет
					 *
					 * \~english
					 * Flag of an encountered reference to an unread parameter entity
					 * @details The standard orders a parser not reading external entities to process
					 * the declarations only UP TO the first such reference: the declarations after it
					 * may be overridden by what lies inside the unread, and one may not rely upon them.
					 * There is one exception — a text declared standalone
					 * @note This flag is separate from `_foreign`: the latter is raised by a declaration
					 * of an external subset as well, which by itself does not override the declarations
					 * of the internal subset
					 *
					 * \~
					 */
					bool _incomplete;
					/**
					 * \~russian
					 * Признак того, что разобранное имя превысило предел, заданный настройками
					 *
					 * @details Отказ разбора имени выходит из двух разных поводов: имя построено
					 * ошибочно либо длина его превысила предел настроек. Поводы эти вызывающему
					 * следует различать: первый означает испорченную разметку, а второй - лишь
					 * то, что предел выставлен уже, чем разметка того требует
					 *
					 * @note Признак сбрасывается при входе в разбор всякого имени: держится он
					 * ровно до выдачи отказа и на следующее имя не переходит
					 *
					 * \~english
					 * Flag of the parsed name having exceeded the limit given by the settings
					 * @details A refusal of the parsing of a name comes out of two different causes: the name is constructed
					 * erroneously or its length has exceeded the limit of the settings. The caller should
					 * distinguish those causes: the first means a spoiled markup, while the second — only
					 * that the limit has been set narrower than the markup requires
					 * @note The flag is reset at the entry into the parsing of every name: it is held
					 * exactly until the issuance of the refusal and does not pass over to the next name
					 *
					 * \~
					 */
					bool _overlong;
				private:
					// Признак того, что текущий узел записан самозакрывающейся меткой
					bool _empty;
				private:
					// Признак того, что следующим событием является конец текущего узла
					bool _closing;
				private:
					// Признак того, что разбор находится внутри раздела дословного текста
					bool _cdata;
				private:
					/**
					 * \~russian
					 * Признак того, что часть текущего раздела дословного текста уже выдана
					 *
					 * @note Раздел, разорванный границами кусков, выдаётся частями, и последняя
					 * из них оказывается пустой всякий раз, когда граница легла прямо перед
					 * завершением раздела. Пустое событие узлу ничего не добавляет, а дерево
					 * получает от него пустой узел содержимого, из-за которого запись перестаёт
					 * складываться самозакрывающейся меткой. Пустой хвост потому пропускается -
					 * но лишь у раздела, часть которого уже выдана: раздел, пустой сам по себе,
					 * своё единственное событие выдаёт наравне с прочими
					 *
					 * \~english
					 * Flag of a part of the current literal text section having already been issued
					 * @note A section broken by the boundaries of the chunks is issued by parts, and the last
					 * of them turns out empty every time the boundary has fallen right before
					 * the termination of the section. An empty event adds nothing to a node, while the tree
					 * receives from it an empty content node, because of which the writing ceases
					 * to fold into a self-closing tag. An empty tail is therefore skipped —
					 * but only for a section a part of which has already been issued: a section empty
					 * in itself issues its single event on a par with the rest
					 *
					 * \~
					 */
					bool _partial;
				private:
					/**
					 * \~russian
					 * Признак того, что начало содержимого перенесено с пропущенного шага
					 *
					 * @note Содержимое, целиком составленное ссылками на пустые сущности,
					 * события не выдаёт, а место разбора проходит вперёд. Начало содержимого
					 * потому переносится на следующий шаг: иначе место события зависело бы
					 * от того, где легла граница куска, - содержимое, пришедшее целиком,
					 * дало бы местом начало всего содержимого, а разорванное границей -
					 * начало своей непустой части
					 *
					 * \~english
					 * Flag of the beginning of a content having been carried over from a skipped step
					 * @note A content composed entirely of the references to empty entities
					 * issues no event, while the place of the parsing moves forward. The beginning of the content
					 * is therefore carried over to the next step: otherwise the place of an event would depend
					 * on where the boundary of a chunk has fallen — a content that arrived in full
					 * would give as the place the beginning of the whole content, while one broken by a boundary —
					 * the beginning of its non-empty part
					 *
					 * \~
					 */
					bool _carried;
				private:
					// Перенесённое с пропущенного шага место начала содержимого
					location_t _resumed;
				private:
					/**
					 * \~russian
					 * Место начала раздела дословного текста в исходном тексте
					 *
					 * @note Место события считается по ходу разбора и вперёд, а начало раздела
					 * к мигу его выдачи остаётся позади: раздел дочитывается следующими кусками.
					 * Оттого место запоминается при входе в раздел, а не считается заново -
					 * иначе оно зависело бы от того, где легла граница куска
					 *
					 * \~english
					 * Place of the beginning of a literal text section in the source text
					 * @note The place of an event is counted in the course of the parsing and forwards, while the beginning of a section
					 * by the moment of its issuance remains behind: the section is read up by the following chunks.
					 * Because of that the place is remembered at the entry into the section rather than being counted anew —
					 * otherwise it would depend on where the boundary of a chunk has fallen
					 *
					 * \~
					 */
					location_t _opening;
				private:
					/**
					 * \~russian
					 * Положение начала раздела дословного текста в приведённом тексте
					 *
					 * @note Держится ради проверки границы подстановки: раздел, начатый внутри
					 * подставленной сущности, обязан внутри неё и закончиться, а разбор его
					 * растянут по кускам исходного текста - начало к концу разбора уже пройдено
					 *
					 * \~english
					 * Position of the beginning of a literal text section in the converted text
					 * @note It is kept for the sake of the check of the boundary of a substitution: a section begun inside
					 * a substituted entity is obliged to end inside it as well, while its parsing
					 * is stretched over the chunks of the source text — the beginning by the end of the parsing has already been passed
					 *
					 * \~
					 */
					size_t _section;
				private:
					/**
					 * \~russian
					 * Признак того, что текущее содержимое узла уже выдано непробельной частью
					 *
					 * @note Незначимым пробельным содержимое считается лишь тогда, когда пробельным
					 * оказалось оно целиком, а выдаётся оно частями по мере поступления кусков
					 * исходного текста. Стоит хоть одной части оказаться непробельной, как остальные
					 * части того же содержимого незначимыми быть перестают, чем бы они ни были
					 * записаны: иначе вид события зависел бы от того, где легла граница куска
					 *
					 * \~english
					 * Flag of the current content of a node having already been issued by a non-whitespace part
					 * @note A content is considered an insignificant whitespace one only when it has turned out to be
					 * a whitespace one in full, while it is issued by parts as the chunks of the source text
					 * arrive. It is enough for even one part to turn out non-whitespace for the remaining
					 * parts of the same content to cease being insignificant, however they may be
					 * written: otherwise the kind of the event would depend on where the boundary of a chunk has fallen
					 *
					 * \~
					 */
					bool _dirty;
				private:
					// Приведённый к кодировке UTF-8 исходный текст
					string _buffer;
				private:
					// Положение разбора в приведённом исходном тексте
					size_t _offset;
				private:
					// Количество байтов, изъятых из начала приведённого текста
					uint64_t _consumed;
				private:
					// Текущий номер строки в исходном тексте
					uint32_t _line;
				private:
					// Текущее положение в строке исходного текста
					uint32_t _column;
				private:
					// Глубина вложенности узла текущего события
					uint32_t _depth;
				private:
					// Хранилище знаков имён узлов и обозначений пространств имён
					string _names;
				private:
					// Отрезок хранилища с обозначением пространства имён, отведённого префиксу «xml»
					span_t _xml;
				private:
					// Смещение усечения хранилища знаков, отложенного до следующего события
					size_t _truncate;
				private:
					/**
					 * \~russian
					 * Буфер ключа отыскания в хранилищах объявлений описания типа документа
					 *
					 * @details Хранилища объявлений ведутся по имени в записи, принятой в исходном
					 * тексте, а разбор отыскивает их по отрезку разбираемого текста. Отдельный тип
					 * отрезка от ключа хранилища язык различать научился лишь изданием 2020 года, а
					 * до него всякое отыскание требует ключа заданного хранилищем типа: имя
					 * приходится переписывать. Переписывается оно в один и тот же буфер - тогда
					 * место под него отводится единожды, а не на каждое отыскание
					 *
					 * @note Буфер помечен изменяемым намеренно: отыскание сущности, содержащей
					 * разметку, ведётся из неизменяющего метода, а к состоянию разбора буфер
					 * отношения не имеет - он живёт лишь на время самого отыскания
					 *
					 * \~english
					 * Buffer of the search key in the storages of the declarations of a document type definition
					 * @details The storages of the declarations are kept by the name in the notation accepted in the source
					 * text, while the parsing searches for them by a segment of the text being parsed. A separate type
					 * of a segment from the key of a storage the language has learnt to distinguish only by the 2020 edition, while
					 * before it every search requires a key of the type given by the storage: the name
					 * has to be rewritten. It is rewritten into one and the same buffer — then
					 * the place for it is allotted once rather than at every search
					 * @note The buffer is marked as mutable deliberately: the search of an entity containing
					 * a markup is conducted from a non-modifying method, while the buffer has no relation to the state of the parsing —
					 * it lives only for the duration of the search itself
					 *
					 * \~
					 */
					mutable string _lookup;
				private:
					// Хранилище приведённых значений атрибутов и содержимого узлов
					string _scratch;
				private:
					// Общий объём выполненной подстановки сущностей в байтах
					uint64_t _expansion;
				private:
					// Стек открытых узлов разметки
					vector <element_t> _stack;
				private:
					// Действующие связывания префиксов с пространствами имён
					vector <scope_t> _scopes;
				private:
					// Объявления пространств имён текущего узла до разрешения
					vector <record_t> _declares;
				private:
					// Отрезки значений атрибутов текущего узла в хранилище приведённых значений
					vector <span_t> _values;
				private:
					// Открытые области подставленных в текст сущностей
					vector <splice_t> _splices;
				private:
					// Ход поиска конца описания типа документа между кусками текста
					survey_t _survey;
				private:
					// Ключи имён атрибутов текущего узла для поиска повторов
					vector <uint64_t> _keys;
				private:
					// Раскладка имён атрибутов текущего узла по свёртке для поиска повторов
					vector <digest_t> _digests;
				private:
					/**
					 * \~russian
					 * @brief Вид значения в кавычках внутреннего подмножества
					 *
					 * @details Договор задаёт четыре вида значений в кавычках, и допустимое
					 * внутри них содержимое у каждого своё: значение сущности не допускает
					 * одиноких знаков ссылки, значение атрибута - знака начала разметки,
					 * обозначение общедоступного источника - лишь ограниченного набора знаков
					 *
					 * \~english
					 * @brief Kind of a quoted value of the internal subset
					 * @details The protocol gives four kinds of the quoted values, and the content admissible
					 * inside them is its own for each of them: the value of an entity does not admit
					 * the lone characters of a reference, the value of an attribute — the character of the beginning of a markup,
					 * the designation of a public source — only a limited set of the characters
					 *
					 * \~
					 */
					enum class literal_t : uint8_t {
						ENTITY    = 0x01, // Значение объявляемой сущности
						ATTRIBUTE = 0x02, // Объявленное по умолчанию значение атрибута
						SYSTEM    = 0x03, // Обозначение размещения внешнего источника
						PUBLIC    = 0x04  // Обозначение общедоступного внешнего источника
					};
					/**
					 * \~russian
					 * @brief Итог одного шага разбора
					 *
					 * \~english
					 * @brief Result of a single step of the parsing
					 *
					 * \~
					 */
					enum class step_t : uint8_t {
						DONE   = 0x00, // Событие разбора получено
						HUNGRY = 0x01, // Для продолжения разбора требуется следующий кусок текста
						FAILED = 0x02  // Разбор прекращён ошибкой
					};
				private:
					/**
					 * \~russian
					 * @brief Метод прекращения разбора ошибкой
					 *
					 * @param error  код ошибки разбора
					 * @param offset положение ошибки в приведённом исходном тексте
					 * @return       итог шага разбора, всегда отрицательный
					 *
					 * \~english
					 * @brief Method of terminating the parsing with an error
					 * @param error  error code of the parsing
					 * @param offset position of the error in the converted source text
					 * @return       result of the step of the parsing, always a negative one
					 *
					 * \~
					 */
					step_t fail(const error_t error, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод отказа разбора с сообщением о нём в журнал
					 *
					 * @details Способ этот стоит ГОРЛОМ: всякий отказ, идущий не через `fail`,
					 * обязан идти через него. Сообщение в журнал пишется в двух местах на весь
					 * разбор - здесь и в `fail`, - а не при всяком присваивании кода отказа:
					 * иначе места записи разошлись бы с местами отказа, и часть бед уходила бы
					 * молча
					 *
					 * @param error код ошибки разбора
					 * @return      всегда ложь, ради возврата им из места отказа
					 *
					 * \~english
					 * @brief Method of the refusal of the parsing with a report of it to the log
					 *
					 * @param error the code of the parsing error
					 * @return      always false, for the returning by it from the place of the refusal
					 *
					 * \~
					 */
					bool refuse(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод сообщения об отказе разбора в журнал
					 *
					 * @param error    код ошибки разбора
					 * @param location место обнаруженной ошибки в исходном тексте
					 *
					 * \~english
					 * @brief Method of the reporting of a parsing refusal to the log
					 *
					 * @param error    the code of the parsing error
					 * @param location the place of the found error in the source text
					 *
					 * \~
					 */
					void report(const error_t error, const location_t & location) const noexcept;
					/**
					 * \~russian
					 * @brief Метод прекращения разбора ошибкой с заданным местом
					 *
					 * @details Место отказа считается по ходу разбора и только вперёд, а
					 * построение, растянутое по кускам исходного текста, к мигу отказа
					 * начало своё уже миновало: досчитать место назад нечем. Такому отказу
					 * место передаётся снятым при входе в построение - иначе оно зависело бы
					 * от того, где легла граница куска
					 *
					 * @param error    код ошибки разбора
					 * @param location место обнаруженной ошибки в исходном тексте
					 * @return         итог шага разбора, всегда отрицательный
					 *
					 * \~english
					 * @brief Method of terminating the parsing with an error at a given place
					 * @details The place of a refusal is counted in the course of the parsing and only forwards, while
					 * a construct stretched over the chunks of the source text by the moment of the refusal
					 * has already passed its own beginning: there is nothing to count the place backwards with. To such a refusal
					 * the place taken at the entry into the construct is passed — otherwise it would depend
					 * on where the boundary of a chunk has fallen
					 * @param error    error code of the parsing
					 * @param location place of the detected error in the source text
					 * @return         result of the step of the parsing, always a negative one
					 *
					 * \~
					 */
					step_t fail(const error_t error, const location_t & location) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения положения в исходном тексте
					 *
					 * @param offset положение в приведённом исходном тексте
					 * @return       положение в исходном тексте разметки
					 *
					 * \~english
					 * @brief Method of getting the position in the source text
					 * @param offset position in the converted source text
					 * @return       position in the source markup text
					 *
					 * \~
					 */
					location_t locate(const size_t offset) const noexcept;
					/**
					 * \~russian
					 * @brief Метод досчёта положения в исходном тексте от ранее полученного
					 *
					 * @details Получение места с начала разбора обходится проходом по всему
					 * пройденному тексту, и для каждого атрибута метки такой проход
					 * повторялся бы заново. Досчёт ведёт место от предыдущего полученного,
					 * проходя по тексту метки единожды
					 *
					 * @param location досчитываемое положение в исходном тексте
					 * @param from     положение, до которого место уже досчитано
					 * @param offset   положение, до которого место требуется досчитать
					 *
					 * \~english
					 * @brief Method of counting the position in the source text up from a previously obtained one
					 * @details The getting of a place from the beginning of the parsing costs a pass over the whole
					 * text that has been passed, and for every attribute of a tag such a pass
					 * would be repeated anew. The counting-up leads the place from the previously obtained one,
					 * passing over the text of the tag once
					 * @param location position in the source text being counted up
					 * @param from     position up to which the place has already been counted
					 * @param offset   position up to which the place is required to be counted
					 *
					 * \~
					 */
					void relocate(location_t & location, size_t & from, const size_t offset) const noexcept;
					/**
					 * \~russian
					 * @brief Метод перемещения разбора к указанному положению
					 *
					 * @details Попутно ведётся счёт строк и положения в строке: считать их
					 * отдельным проходом обошлось бы дороже
					 *
					 * @param offset положение в приведённом исходном тексте
					 *
					 * \~english
					 * @brief Method of moving the parsing to the specified position
					 * @details Along the way the count of the lines and of the position in the line is kept: to count them
					 * by a separate pass would come more expensive
					 * @param offset position in the converted source text
					 *
					 * \~
					 */
					void advance(const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод изъятия разобранного начала приведённого текста
					 *
					 * \~english
					 * @brief Method of withdrawing the parsed beginning of the converted text
					 *
					 * \~
					 */
					void compact() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод проверки того, что разметка не пересекает границу подстановки
					 *
					 * @param begin положение начала разметки в приведённом тексте
					 * @param end   положение конца разметки в приведённом тексте
					 * @return      признак того, что разметка границу подстановки пересекает
					 *
					 * \~english
					 * @brief Method of checking that a markup does not cross the boundary of a substitution
					 * @param begin position of the beginning of the markup in the converted text
					 * @param end   position of the end of the markup in the converted text
					 * @return      flag of the markup crossing the boundary of the substitution
					 *
					 * \~
					 */
					bool crosses(const size_t begin, const size_t end) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки разметки события на превышение предела объёма
					 *
					 * @details Предел объёма события сличается по ходу накопления с длиной
					 * накопленной разметки, а не с содержимым события: содержимое к тому мигу
					 * ещё неизвестно. Событиям, чьё содержимое короче своей разметки, - началу
					 * узла, описанию типа документа, указанию обработчику - одной проверки
					 * содержимого потому мало: разметка их произвольно длинна, и текст,
					 * пришедший целиком, проходил бы мимо предела там, где тот же текст
					 * кусками отвергается. Настоящая проверка сличает с пределом ту же самую
					 * величину, что и накопление
					 *
					 * @note Разметка, накопленная к мигу отказа накопления, всегда короче
					 * разметки события целиком: накопление отказывает лишь там, где откажет
					 * и эта проверка, и разбору всё равно, как текст нарезан на куски
					 *
					 * @param end положение конца разметки события в приведённом тексте
					 * @return    признак превышения заданного настройками предела
					 *
					 * \~english
					 * @brief Method of checking the markup of an event for an excess of the limit of the volume
					 * @details The limit of the volume of an event is compared in the course of the accumulation with the length of the
					 * accumulated markup rather than with the content of the event: the content by that moment
					 * is still unknown. For the events whose content is shorter than their markup — the beginning of a
					 * node, a document type definition, a processing instruction — a check of the content alone is
					 * therefore not enough: their markup is arbitrarily long, and a text
					 * that has arrived in full would pass by the limit there where the same text
					 * by chunks is rejected. The present check compares against the limit the same
					 * quantity as the accumulation does
					 * @note The markup accumulated by the moment of a refusal of the accumulation is always shorter than
					 * the markup of the event as a whole: the accumulation refuses only there where
					 * this check will refuse as well, and it is all the same to the parsing how the text is cut into chunks
					 * @param end position of the end of the markup of the event in the converted text
					 * @return    flag of an excess of the limit given by the settings
					 *
					 * \~
					 */
					bool oversize(const size_t end) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод получения последовательности знаков по отрезку хранилища
					 *
					 * @param span отрезок хранилища знаков разбора
					 * @return     последовательность знаков указанного отрезка
					 *
					 * \~english
					 * @brief Method of getting a sequence of characters by a segment of the storage
					 * @param span segment of the storage of the characters of the parsing
					 * @return     sequence of characters of the specified segment
					 *
					 * \~
					 */
					string_view view(const span_t & span) const noexcept;
					/**
					 * \~russian
					 * @brief Метод размещения последовательности знаков в хранилище разбора
					 *
					 * @param text размещаемая последовательность знаков
					 * @return     отрезок хранилища знаков разбора
					 *
					 * \~english
					 * @brief Method of placing a sequence of characters in the storage of the parsing
					 * @param text sequence of characters being placed
					 * @return     segment of the storage of the characters of the parsing
					 *
					 * \~
					 */
					span_t store(const string_view text) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод выполнения одного шага разбора
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of performing a single step of the parsing
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parse() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора текстового содержимого узла
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing the text content of a node
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseText() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора разметки, начинающейся угловой скобкой
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing a markup beginning with an angle bracket
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseMarkup() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора открывающей метки узла
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing the opening tag of a node
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseElement() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора закрывающей метки узла
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing the closing tag of a node
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseClosing() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора примечания
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing a comment
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseComment() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора раздела дословного текста
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing a literal text section
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseCdata() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора указания обработчику и объявления разметки
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing a processing instruction and the markup declaration
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseProcessing() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора описания типа документа
					 *
					 * @return итог шага разбора
					 *
					 * \~english
					 * @brief Method of parsing a document type definition
					 * @return result of the step of the parsing
					 *
					 * \~
					 */
					step_t parseDoctype() noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод разбора имени с необязательным префиксом
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца разбираемой разметки
					 * @param prefix префикс разобранного имени
					 * @param local  местное имя разобранного имени
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a name with an optional prefix
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the markup being parsed
					 * @param prefix prefix of the parsed name
					 * @param local  local name of the parsed name
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool parseName(size_t & offset, const size_t end, string_view & prefix, string_view & local) noexcept;
					/**
					 * \~russian
					 * @brief Метод разделения разобранного имени на префикс и местное имя
					 *
					 * @param begin  положение начала разобранного имени
					 * @param end    положение конца разобранного имени
					 * @param count  количество знаков разобранного имени
					 * @param flags  собранные по всему имени разряды таблицы знаков US-ASCII
					 * @param prefix префикс разобранного имени
					 * @param local  местное имя разобранного имени
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of splitting a parsed name into a prefix and a local name
					 * @param begin  position of the beginning of the parsed name
					 * @param end    position of the end of the parsed name
					 * @param count  number of the characters of the parsed name
					 * @param flags  bits of the table of the US-ASCII characters collected over the whole name
					 * @param prefix prefix of the parsed name
					 * @param local  local name of the parsed name
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool qualify(const size_t begin, const size_t end, const uint32_t count, const uint8_t flags, string_view & prefix, string_view & local) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора значения атрибута
					 *
					 * @details Значение приводится к окончательному виду: ссылки на сущности
					 * подставляются, пробельные знаки заменяются пробелом
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца разбираемой разметки
					 * @param value  отрезок хранилища приведённых значений
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the value of an attribute
					 * @details The value is brought to its final form: the references to the entities
					 * are substituted, the whitespace characters are replaced by a space
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the markup being parsed
					 * @param value  segment of the storage of the converted values
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool parseValue(size_t & offset, const size_t end, span_t & value) noexcept;
					/**
					 * \~russian
					 * @brief Метод подстановки ссылки на сущность
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца разбираемой разметки
					 * @param result  текст, к которому дописывается подставленное значение
					 * @param depth   текущая глубина вложенности ссылок на сущности
					 * @param space   признак приведения пробельных знаков значения
					 * @param content признак подстановки в содержимое узла
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of substituting a reference to an entity
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the markup being parsed
					 * @param result  text to which the substituted value is appended
					 * @param depth   current depth of the nesting of the references to the entities
					 * @param space   flag of the conversion of the whitespace characters of the value
					 * @param content flag of the substitution into the content of a node
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool parseReference(size_t & offset, const size_t end, string & result, const uint32_t depth, const bool space = false, const bool content = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод подстановки значения сущности по её имени
					 *
					 * @details Подстановка ведётся по значению сущности, а не по исходному
					 * тексту: вложенные ссылки внутри значения подставляются тем же способом
					 *
					 * @param name    имя подставляемой сущности либо числовая ссылка
					 * @param result  текст, к которому дописывается подставленное значение
					 * @param depth   текущая глубина вложенности ссылок на сущности
					 * @param space   признак приведения пробельных знаков значения
					 * @param content признак подстановки в содержимое узла
					 * @return        результат выполнения операции
					 *
					 * \~english
					 * @brief Method of substituting the value of an entity by its name
					 * @details The substitution is conducted by the value of the entity rather than by the source
					 * text: the nested references inside the value are substituted in the same way
					 * @param name    name of the entity being substituted or a numeric reference
					 * @param result  text to which the substituted value is appended
					 * @param depth   current depth of the nesting of the references to the entities
					 * @param space   flag of the conversion of the whitespace characters of the value
					 * @param content flag of the substitution into the content of a node
					 * @return        result of performing the operation
					 *
					 * \~
					 */
					bool expand(const string_view name, string & result, const uint32_t depth, const bool space = false, const bool content = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод подстановки сущности с разметкой прямо в разбираемый текст
					 *
					 * @details Значение сущности замещает ссылку на неё в приведённом тексте,
					 * после чего разбирается наравне с записанным в тексте. Иным способом
					 * разметку внутри сущности разобрать нельзя: она обязана давать те же
					 * узлы, что и записанная прямо
					 *
					 * @param begin положение начала ссылки на сущность
					 * @param end   положение конца ссылки на сущность
					 * @param name  имя подставляемой сущности
					 * @return      результат выполнения операции
					 *
					 * \~english
					 * @brief Method of substituting an entity with a markup right into the text being parsed
					 * @details The value of the entity replaces the reference to it in the converted text,
					 * after which it is parsed on a par with what has been written in the text. A markup inside an entity
					 * cannot be parsed in another way: it is obliged to give the same nodes
					 * as one written directly
					 * @param begin position of the beginning of the reference to the entity
					 * @param end   position of the end of the reference to the entity
					 * @param name  name of the entity being substituted
					 * @return      result of performing the operation
					 *
					 * \~
					 */
					bool inject(const size_t begin, const size_t end, const string_view name) noexcept;
					/**
					 * \~russian
					 * @brief Метод определения сущности, содержащей разметку
					 *
					 * @param offset положение начала ссылки на сущность
					 * @param name   имя обнаруженной сущности
					 * @param end    положение конца ссылки на сущность
					 * @return       результат проверки
					 *
					 * \~english
					 * @brief Method of determining an entity containing a markup
					 * @param offset position of the beginning of the reference to the entity
					 * @param name   name of the detected entity
					 * @param end    position of the end of the reference to the entity
					 * @return       result of the check
					 *
					 * \~
					 */
					bool markup(const size_t offset, string_view & name, size_t & end) const noexcept;
					/**
					 * \~russian
					 * @brief Метод переноса признака разметки по вложенным ссылкам сущностей
					 *
					 * @details Значение сущности вправе ссылаться на другую сущность, и
					 * подставляется такая ссылка не при объявлении, а при обращении. Оттого
					 * сущность, сама знака начала разметки не несущая, приносит разметку той,
					 * на которую ссылается, и признак следует переносить по ссылкам
					 *
					 * \~english
					 * @brief Method of transferring the flag of a markup along the nested references of the entities
					 * @details The value of an entity has the right to refer to another entity, and
					 * such a reference is substituted not at the declaration but at the call. Because of that
					 * an entity that itself carries no character of the beginning of a markup brings a markup from the one
					 * it refers to, and the flag should be transferred along the references
					 *
					 * \~
					 */
					void inherit() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора внутреннего подмножества описания типа документа
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the internal subset of a document type definition
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool parseSubset(size_t offset, const size_t end) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод пропуска пробельных знаков внутреннего подмножества
					 *
					 * @param offset   положение разбора в приведённом исходном тексте
					 * @param end      положение конца внутреннего подмножества
					 * @param required признак обязательности пробельного знака
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of skipping the whitespace characters of the internal subset
					 * @param offset   position of the parsing in the converted source text
					 * @param end      position of the end of the internal subset
					 * @param required flag of the obligatoriness of a whitespace character
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool subsetSpace(size_t & offset, const size_t end, const bool required) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора имени внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @param token  признак разбора имени, допускающего любой знак в начале
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a name of the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @param token  flag of the parsing of a name admitting any character at the beginning
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetName(size_t & offset, const size_t end, const bool token) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора отведённого договором слова внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @param word   отведённое договором слово
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a word allotted by the protocol in the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @param word   word allotted by the protocol
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetKeyword(size_t & offset, const size_t end, const string_view word) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора значения в кавычках внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @param kind   вид разбираемого значения
					 * @param result текст, к которому дописывается приведённое значение
					 * @param markup признак обнаружения разметки в значении
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a quoted value of the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @param kind   kind of the value being parsed
					 * @param result text to which the converted value is appended
					 * @param markup flag of the detection of a markup in the value
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetLiteral(size_t & offset, const size_t end, const literal_t kind, string * result, bool * markup) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора обозначения внешнего источника внутреннего подмножества
					 *
					 * @param offset   положение разбора в приведённом исходном тексте
					 * @param end      положение конца внутреннего подмножества
					 * @param system   признак обязательности обозначения размещения источника
					 * @param external признак обнаружения обозначения внешнего источника
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the designation of an external source of the internal subset
					 * @param offset   position of the parsing in the converted source text
					 * @param end      position of the end of the internal subset
					 * @param system   flag of the obligatoriness of the designation of the location of the source
					 * @param external flag of the detection of the designation of an external source
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool subsetExternal(size_t & offset, const size_t end, const bool system, bool & external) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора описания строения узла внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @param depth  текущая глубина вложенности описания строения
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the description of the structure of a node of the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @param depth  current depth of the nesting of the description of the structure
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetChildren(size_t & offset, const size_t end, const uint32_t depth) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора описания содержимого узла внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the description of the content of a node of the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetContent(size_t & offset, const size_t end) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора вида объявляемого атрибута внутреннего подмножества
					 *
					 * @param offset положение разбора в приведённом исходном тексте
					 * @param end    положение конца внутреннего подмножества
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing the kind of an attribute being declared in the internal subset
					 * @param offset position of the parsing in the converted source text
					 * @param end    position of the end of the internal subset
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool subsetAttribute(size_t & offset, const size_t end, bool & tokenized) noexcept;
					/**
					 * \~russian
					 * @brief Метод разделения имени описания типа документа на префикс и местное имя
					 *
					 * @param qname  имя в записи, принятой в исходном тексте
					 * @param prefix префикс разделённого имени
					 * @param local  местное имя разделённого имени
					 *
					 * \~english
					 * @brief Method of splitting a name of a document type definition into a prefix and a local name
					 * @param qname  name in the notation accepted in the source text
					 * @param prefix prefix of the split name
					 * @param local  local name of the split name
					 *
					 * \~
					 */
					void divide(const string_view qname, string & prefix, string & local) const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод добавления разобранного атрибута к перечню узла
					 *
					 * @details Объявления пространств имён в перечне атрибутов узла не выдаются
					 * и отводятся в отдельный перечень: они не описывают узел, а связывают
					 * префиксы. Прочие атрибуты собираются сразу в выдаваемом виде, а значение
					 * получают отдельным отрезком - хранилище приведённых значений ещё
					 * перемещается по мере разбора, и указывать на него пока нельзя
					 *
					 * @param record разобранный атрибут узла
					 *
					 * \~english
					 * @brief Method of adding a parsed attribute to the list of a node
					 * @details The declarations of the namespaces are not issued in the list of the attributes of a node
					 * and are assigned to a separate list: they do not describe the node but bind
					 * the prefixes. The other attributes are assembled at once in the form to be issued, while the value
					 * they receive as a separate segment — the storage of the converted values is still
					 * being moved as the parsing goes on, and it cannot be pointed to yet
					 * @param record parsed attribute of the node
					 *
					 * \~
					 */
					void append(const record_t & record) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод разрешения пространств имён текущего узла
					 *
					 * @return результат выполнения операции
					 *
					 * \~english
					 * @brief Method of resolving the namespaces of the current node
					 * @return result of performing the operation
					 *
					 * \~
					 */
					bool bind() noexcept;
					/**
					 * \~russian
					 * @brief Метод завершения текущего узла разметки
					 *
					 * @details Снимает узел со стека вместе с объявленными им связываниями
					 * префиксов и готовит событие конца узла
					 *
					 * \~english
					 * @brief Method of completing the current markup node
					 * @details Pops the node off the stack together with the bindings of the prefixes declared by it
					 * and prepares the event of the end of the node
					 *
					 * \~
					 */
					void closure() noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска пространства имён по префиксу
					 *
					 * @param prefix префикс без разделителя
					 * @return       отрезок хранилища с обозначением пространства имён
					 *
					 * \~english
					 * @brief Method of searching for a namespace by a prefix
					 * @param prefix prefix without the separator
					 * @return       segment of the storage with the designation of the namespace
					 *
					 * \~
					 */
					span_t lookup(const string_view prefix) const noexcept;
				private:
					// Настройки разбора текста разметки
					settings_t _settings;
				private:
					// Текущее состояние чтения текста разметки
					state_t _state;
				private:
					// Вид текущего события разбора
					event_t _event;
				private:
					// Код ошибки последней операции разбора
					error_t _error;
				private:
					/**
					 * \~russian
					 * Объект ведения журнала работы
					 *
					 * @note Указание вправе быть пустым: разбор работает и без журнала, лишь
					 *       не сообщая о бедах никуда. Сличение на пустоту стоит в одном месте -
					 *       в способе записи, - и по местам отказа не разносится
					 *
					 * \~english
					 * Object of the keeping of the work log
					 *
					 * \~
					 */
					const log_t * _log = nullptr;
				private:
					/**
					 * \~russian
					 * Отложенный код ошибки приведения исходного текста к кодировке UTF-8
					 *
					 * @note Приведение переносит в хранилище всё, что успело проверить, и
					 *       лишь затем отвечает отказом. Отказ выдаётся не сразу, а по
					 *       исчерпании приведённого начала текста: иначе разбор одного и
					 *       того же текста целиком и кусками расходился бы событиями
					 *
					 * \~english
					 * Postponed error code of the conversion of the source text to the UTF-8 encoding
					 * @note The conversion transfers into the storage everything it has managed to check, and
					 *       only then answers with a refusal. The refusal is issued not at once but upon
					 *       the exhaustion of the converted beginning of the text: otherwise the parsing of one and
					 *       the same text in full and by chunks would diverge in the events
					 *
					 * \~
					 */
					error_t _decoding;
				private:
					/**
					 * \~russian
					 * Отказ разбора, отложенный до выдачи накопленного содержимого
					 *
					 * @details Ссылка на необъявленную сущность стоит посреди содержимого, и
					 * содержимое, разобранное до неё, отказом не отменяется: подача кусками
					 * успевала выдать его событием, а подача целиком выбрасывала - тот же текст
					 * давал разный набор событий. Отказ потому запоминается, накопленное
					 * выдаётся событием, и лишь следом за ним разбор отвечает отказом
					 *
					 * \~english
					 * Refusal of the parsing deferred until the issuing of the accumulated content
					 * @details A reference to an undeclared entity stands in the middle of a content, and the
					 * content parsed before it is not cancelled by the refusal: the feeding by chunks
					 * managed to issue it as an event, while the feeding in full discarded it — the same text
					 * gave a different set of events. The refusal is therefore remembered, what has been accumulated
					 * is issued as an event, and only after it does the parsing answer with a refusal
					 *
					 * \~
					 */
					error_t _deferred;
				private:
					// Положение отложенного отказа разбора в исходном тексте
					size_t _postponed;
				private:
					// Определённая кодировка исходного текста
					encoding_t _encoding;
				private:
					// Признак самодостаточности текста разметки
					standalone_t _standalone;
				private:
					// Обращение с пробельным содержимым в текущем узле
					space_t _space;
				private:
					// Имя узла либо цель указания обработчику текущего события
					name_t _name;
				private:
					// Содержимое текущего события
					string_view _text;
				private:
					// Положение начала текущего события в исходном тексте
					location_t _location;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					location_t _errorLocation;
				private:
					// Объявленное издание разметки
					string _version;
				private:
					// Перечень атрибутов текущего узла
					vector <attribute_t> _attributes;
				private:
					// Перечень объявлений пространств имён текущего узла
					vector <binding_t> _bindings;
				private:
					// Объявленные сущности внутреннего подмножества описания типа
					unordered_map <string, entity_t> _entities;
				private:
					// Объявленные по умолчанию значения атрибутов, разложенные по именам узлов
					unordered_map <string, vector <default_t>> _attlists;
					/**
					 * \~russian
					 * Признак объявления хотя бы одного атрибута видом, приведению подлежащим
					 *
					 * @details Приведение значений обходится в четверть скорости чтения документа
					 * с описанием типа, а описание, объявляющее одни лишь атрибуты вида «CDATA»,
					 * приводить нечего вовсе. Признак этот снимает с такого описания и розыск
					 * объявлений на всякий узел, и перебор их по всякому атрибуту
					 *
					 * \~english
					 * Flag of a declaration of at least one attribute by a kind subject to the normalization
					 * @details The normalization of the values costs a quarter of the speed of the reading
					 * of a document with a document type definition, while a definition declaring only
					 * the attributes of the kind «CDATA» has nothing to normalize at all. This flag removes
					 * from such a definition both the search of the declarations for every node and their
					 * enumeration for every attribute
					 *
					 * \~
					 */
					bool _tokenized;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек разбора
					 *
					 * @return текущие настройки разбора текста разметки
					 *
					 * \~english
					 * @brief Method of getting the current settings of the parsing
					 * @return current settings of the parsing of a markup text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора
					 *
					 * @warning Настройки применяются к разбору целиком и смены посреди
					 * текста не допускают: устанавливать их следует до первого куска
					 *
					 * @param settings настройки разбора текста разметки
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing
					 * @warning The settings are applied to the parsing as a whole and do not admit a change in the middle of
					 * a text: they should be set before the first chunk
					 * @param settings settings of the parsing of a markup text
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса разбора в исходное состояние
					 *
					 * @details Освобождает накопленное и подготавливает чтение к разбору
					 * нового текста, сохраняя установленные настройки
					 *
					 * \~english
					 * @brief Method of resetting the parsing into the initial state
					 * @details Releases what has been accumulated and prepares the reading for the parsing of
					 * a new text, preserving the settings that have been set
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод передачи очередного куска исходного текста
					 *
					 * @details Куски передаются в порядке их следования в тексте и делятся
					 * произвольно: разрыв допустим в любом месте, в том числе посреди имени
					 * либо ссылки на сущность
					 *
					 * @note Признак последнего куска обязателен: без него не отличить конец
					 * текста от его обрыва, и незакрытые узлы останутся незамеченными
					 *
					 * @note Отказ приведения исходного текста к кодировке UTF-8 выдаётся не
					 * настоящим методом, а по исчерпании уже приведённого начала текста:
					 * события, разобранные до испорченного знака, выдаются, и подача текста
					 * целиком выдаёт то же самое, что и подача его кусками. Настоящий метод
					 * отвечает при этом положительно - события ещё предстоит вычитать
					 * методом @c next(), - а следующий кусок принят уже не будет. Отказ
					 * читается состоянием @c state_t::FAILED и кодом ошибки, как всякий
					 * другой
					 *
					 * @note Пустой указатель на буфер приравнивается к пустой подаче независимо от
					 * заданного размера: проверка стоит в приведении текста, и разыменования не
					 * происходит. Отвечает разбор при этом по своему правилу о пустом тексте
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the next chunk of the source text
					 * @details The chunks are passed in the order of their succession in the text and are divided
					 * arbitrarily: a break is admissible in any place, including in the middle of a name
					 * or of a reference to an entity
					 * @note The flag of the last chunk is obligatory: without it the end of a text cannot be distinguished
					 * from its cut-off, and the unclosed nodes will remain unnoticed
					 * @note A refusal of the conversion of the source text to the UTF-8 encoding is issued not by
					 * the present method but upon the exhaustion of the already converted beginning of the text:
					 * the events parsed before the spoiled character are issued, and a feeding of the text
					 * in full issues the same as a feeding of it by chunks. The present method
					 * thereby answers positively — the events are still to be read out
					 * by the @c next() method — while the next chunk will no longer be accepted. The refusal
					 * is read by the @c state_t::FAILED state and by the error code, like every
					 * other one
					 * @note A null pointer to the buffer is equated to an empty feeding independently of
					 * the given size: the check stands in the conversion of the text, and no dereferencing
					 * takes place. The parsing answers thereby by its own rule about an empty text
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод передачи исходного текста целиком
					 *
					 * @details Разбирает переданный текст как единственный и последний кусок
					 *
					 * @note Переданный текст переживать вызов не обязан: он приводится к
					 * кодировке UTF-8 в собственное хранилище разборщика, и выдаваемые
					 * последовательности знаков ссылаются на него, а не на переданное.
					 * Держать исходный текст до конца разбора поэтому не требуется
					 *
					 * @param text исходный текст разметки целиком
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the source text in full
					 * @details Parses the passed text as the single and last chunk
					 * @note The passed text is not obliged to outlive the call: it is converted to
					 * the UTF-8 encoding into the own storage of the parser, and the issued
					 * sequences of characters refer to it rather than to what has been passed.
					 * It is therefore not required to keep the source text until the end of the parsing
					 * @param text source markup text in full
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему событию разбора
					 *
					 * @details Отрицательный итог означает, что событий больше нет: разбор
					 * либо исчерпал переданное и ждёт следующего куска, либо дошёл до конца
					 * текста, либо прекращён ошибкой. Что именно произошло, сообщает
					 * состояние чтения
					 *
					 * @return признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 * @details A negative result means that there are no more events: the parsing
					 * has either exhausted what has been passed and is waiting for the next chunk, or has reached the end of the
					 * text, or has been terminated by an error. What exactly has happened is reported by the
					 * state of the reading
					 * @return flag of the presence of the next parsing event
					 *
					 * \~
					 */
					bool next() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущего состояния чтения
					 *
					 * @return текущее состояние чтения текста разметки
					 *
					 * \~english
					 * @brief Method of getting the current state of the reading
					 * @return current state of the reading of a markup text
					 *
					 * \~
					 */
					state_t state() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида текущего события разбора
					 *
					 * @return вид текущего события разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the kind of the current parsing event
					 * @return kind of the current parsing event
					 *
					 * \~
					 */
					event_t event() const noexcept;
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
				public:
					/**
					 * \~russian
					 * @brief Метод получения имени текущего события
					 *
					 * @details Для начала и конца узла - имя узла, для указания обработчику -
					 * его цель. Для прочих событий имя пусто
					 *
					 * @return имя узла с учётом пространства имён
					 *
					 * \~english
					 * @brief Method of getting the name of the current event
					 * @details For the beginning and the end of a node — the name of the node, for a processing instruction —
					 * its target. For the other events the name is empty
					 * @return name of the node with regard to the namespace
					 *
					 * \~
					 */
					const name_t & name() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого текущего события
					 *
					 * @details Для текстового содержимого, дословного раздела и примечания -
					 * их содержимое, для указания обработчику - его данные, для описания типа
					 * документа - его имя. Для прочих событий содержимое пусто
					 *
					 * @warning Возвращаемая последовательность знаков остаётся пригодной лишь
					 * до следующего обращения к @c next() либо @c feed()
					 *
					 * @return содержимое текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the content of the current event
					 * @details For a text content, a literal section and a comment —
					 * their content, for a processing instruction — its data, for a document type
					 * definition — its name. For the other events the content is empty
					 * @warning The returned sequence of characters remains valid only
					 * until the next call to @c next() or @c feed()
					 * @return content of the current parsing event
					 *
					 * \~
					 */
					string_view text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места текущего события в исходном тексте
					 *
					 * @return положение начала текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the current event in the source text
					 * @return position of the beginning of the current event in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения перечня атрибутов текущего узла
					 *
					 * @details Перечень заполняется событием начала узла и остаётся
					 * пригодным до перехода к следующему событию
					 *
					 * @note Объявления пространств имён в перечень не входят: они доступны
					 * отдельным перечнем связываний
					 *
					 * @return перечень атрибутов текущего узла
					 *
					 * \~english
					 * @brief Method of getting the list of the attributes of the current node
					 * @details The list is filled in by the event of the beginning of a node and remains
					 * valid until the transition to the next event
					 * @note The declarations of the namespaces do not enter the list: they are available
					 * as a separate list of the bindings
					 * @return list of the attributes of the current node
					 *
					 * \~
					 */
					const vector <attribute_t> & attributes() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения перечня объявлений пространств имён
					 *
					 * @return перечень связываний префиксов, объявленных текущим узлом
					 *
					 * \~english
					 * @brief Method of getting the list of the declarations of the namespaces
					 * @return list of the bindings of the prefixes declared by the current node
					 *
					 * \~
					 */
					const vector <binding_t> & bindings() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод поиска атрибута текущего узла по имени
					 *
					 * @details Сличение ведётся по паре из обозначения пространства имён и
					 * местного имени. Пустое обозначение отвечает атрибуту без префикса:
					 * атрибуты, в отличие от узлов, объявлению пространства имён по умолчанию
					 * не подчиняются
					 *
					 * @param local местное имя искомого атрибута
					 * @param uri   обозначение пространства имён искомого атрибута
					 * @return      значение найденного атрибута либо пустая последовательность
					 *
					 * \~english
					 * @brief Method of searching for an attribute of the current node by a name
					 * @details The comparison is conducted by the pair of the designation of the namespace and the
					 * local name. An empty designation corresponds to an attribute without a prefix:
					 * the attributes, unlike the nodes, are not subject to a default namespace
					 * declaration
					 * @param local local name of the attribute being sought
					 * @param uri   designation of the namespace of the attribute being sought
					 * @return      value of the found attribute or an empty sequence
					 *
					 * \~
					 */
					string_view attribute(const string_view local, const string_view uri = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки наличия атрибута у текущего узла
					 *
					 * @param local местное имя искомого атрибута
					 * @param uri   обозначение пространства имён искомого атрибута
					 * @return      результат проверки
					 *
					 * \~english
					 * @brief Method of checking the presence of an attribute at the current node
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
					 * @brief Метод получения содержимого текущего события числом
					 *
					 * @details Разбор ведётся по правилам местности «C» с отбрасыванием
					 * пробельной обвязки и с проверкой выхода за пределы запрошенного типа
					 *
					 * @warning При разборе кусками содержимое узла выдаётся несколькими
					 * событиями, и разбирать числом следует лишь содержимое, собранное
					 * целиком: для этого предназначено склеивание содержимого настройками
					 *
					 * @param result ссылка на результат разбора
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the content of the current event as a number
					 * @details The parsing is conducted by the rules of the «C» locale with the discarding of the
					 * whitespace padding and with a check of going beyond the limits of the requested type
					 * @warning At a parsing by chunks the content of a node is issued by several
					 * events, and only a content assembled in full should be parsed as a number:
					 * for this the gluing of the content by the settings is intended
					 * @param result reference to the result of the parsing
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result) const noexcept {
						// Выполняем разбор содержимого текущего события числом
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
					 * @brief Метод получения значения атрибута текущего узла числом
					 *
					 * @param result ссылка на результат разбора
					 * @param local  местное имя искомого атрибута
					 * @param uri    обозначение пространства имён искомого атрибута
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the value of an attribute of the current node as a number
					 * @param result reference to the result of the parsing
					 * @param local  local name of the attribute being sought
					 * @param uri    designation of the namespace of the attribute being sought
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result, const string_view local, const string_view uri = "") const noexcept {
						// Выполняем разбор значения атрибута текущего узла числом
						return numeric(this->attribute(local, uri), result);
					}
				public:
					/**
					 * \~russian
					 * @brief Метод получения обозначения пространства имён по префиксу
					 *
					 * @details Поиск ведётся по связываниям, действующим в текущем месте
					 * разбора, от ближайшего узла к корню
					 *
					 * @param prefix префикс без разделителя, пустой для объявления по умолчанию
					 * @return       обозначение связанного пространства имён
					 *
					 * \~english
					 * @brief Method of getting the designation of a namespace by a prefix
					 * @details The search is conducted by the bindings effective at the current place
					 * of the parsing, from the nearest node to the root
					 * @param prefix prefix without the separator, empty for a default declaration
					 * @return       designation of the bound namespace
					 *
					 * \~
					 */
					string_view resolve(const string_view prefix) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения глубины вложенности текущего узла
					 *
					 * @return глубина вложенности, нулевая для корневого узла
					 *
					 * \~english
					 * @brief Method of getting the depth of the nesting of the current node
					 * @return depth of the nesting, zero for the root node
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки узла на отсутствие содержимого
					 *
					 * @details Узел, записанный самозакрывающейся меткой, выдаёт события
					 * начала и конца подряд; настоящий признак позволяет их различить
					 *
					 * @return результат проверки
					 *
					 * \~english
					 * @brief Method of checking a node for the absence of a content
					 * @details A node written by a self-closing tag issues the events of the
					 * beginning and of the end in a row; the present flag makes it possible to distinguish them
					 * @return result of the check
					 *
					 * \~
					 */
					bool empty() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения принятого обращения с пробельным содержимым
					 *
					 * @details Обращение задаётся отведённым договором атрибутом `xml:space` и
					 * наследуется вложенными узлами до отмены изнутри. Значение, договором не
					 * отведённое, толкуется отменой наравне с `default`
					 *
					 * @note Атрибут отведён самим договором о разметке, а не договором о
					 *       пространствах имён: он учитывается и при выключенном разрешении
					 *       префиксов, где имя его не разделено и узнаётся записью целиком
					 *
					 * @return обращение с пробельным содержимым в текущем узле
					 *
					 * \~english
					 * @brief Method of getting the accepted treatment of the whitespace content
					 * @details The treatment is given by the `xml:space` attribute allotted by the protocol and is
					 * inherited by the nested nodes until a cancellation from within. A value not allotted by the protocol
					 * is interpreted as a cancellation on a par with `default`
					 * @note The attribute is allotted by the markup protocol itself rather than by the protocol about the
					 *       namespaces: it is taken into account even when the resolution of the prefixes is disabled,
					 *       where its name is not split and is recognized by the record as a whole
					 * @return treatment of the whitespace content in the current node
					 *
					 * \~
					 */
					space_t space() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения определённой кодировки исходного текста
					 *
					 * @return определённая кодировка исходного текста
					 *
					 *
					 * \~english
					 * @brief Method of getting the determined encoding of the source text
					 * @return determined encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения объявленного издания разметки
					 *
					 * @return объявленное издание разметки
					 *
					 * \~english
					 * @brief Method of getting the declared edition of the markup
					 * @return declared edition of the markup
					 *
					 * \~
					 */
					string_view version() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения признака самодостаточности текста
					 *
					 * @return признак самодостаточности текста разметки
					 *
					 * \~english
					 * @brief Method of getting the flag of the standaloneness of the text
					 * @return flag of the standaloneness of the markup text
					 *
					 * \~
					 */
					standalone_t standalone() const noexcept;
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
					explicit Reader(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param settings настройки разбора текста разметки
					 *
					 * \~english
					 * @brief Constructor
					 * @param settings settings of the parsing of a markup text
					 *
					 * \~
					 */
					explicit Reader(const log_t * log, const settings_t & settings) noexcept;
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
					~Reader() noexcept;
			} reader_t;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_XML_READER__
