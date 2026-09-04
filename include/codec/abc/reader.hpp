/**
 * @file reader.hpp
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
 * @brief Заголовочный файл поточного чтения бинарного контейнера ABC
 *
 * \~english
 * @brief Header file of the streaming reading of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_ABC_READER__
#define __AWH_CODEC_ABC_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"

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
		 * @brief Пространство имён бинарного контейнера ABC
		 *
		 * \~english
		 * @brief ABC binary container namespace
		 *
		 * \~
		 */
		namespace abc {
			/**
			 * \~russian
			 * @brief Класс поточного чтения бинарной записи
			 *
			 * @details Чтение выдаёт события по мере разбора записи, не удерживая её целиком.
			 * Разбор ведётся без рекурсии: вложенность хранится стеком вместимых, а не стеком
			 * вызовов
			 *
			 * @details **Независимость от нарезки на куски.** Единица записи, поданная не
			 * целиком, разбор не сдвигает: недостающие октеты дожидаются следующей подачи, а
			 * выдача событий от того, как запись нарезана, не зависит ничем
			 *
			 * @note **Разбиратель надлежит держать между записями, а не заводить на всякую.**
			 * Сброс через `reset()` вместимость буфера сохраняет, а новый разбиратель заводит
			 * буфер заново. Плата за это невелика по себе, но от размера разбираемого не
			 * зависит вовсе и потому целиком ложится на дешёвый разбор: замер 22.08.2026
			 * показал, что у нового разбирателя на всякую запись она забирает от 6 до 47
			 * процентов времени разбора смотря по системе (стенды Linux против macOS). Ради
			 * этого заведён и сценарий замера «плата за заведение разбирателя»
			 *
			 * @warning **Замком работа НЕ защищена: один объект — один поток.** Замок держит
			 * лишь `Editor` — ему он нужен ради фиксации по сроку своим потоком, — и
			 * равняться по нему нельзя. Замер 25.08.2026, один `Fetcher` на четыре потока:
			 * тринадцать донесений TSan и девятнадцать неверно прочитанных записей из
			 * четырёхсот, молча. Свой объект у всякого потока над ОБЩИМ источником чтения:
			 * ноль донесений, ноль расхождений — источник читается, а не правится, и делится
			 * свободно
			 *
			 * \~english
			 * @brief Class of the streaming reading of a binary record
			 * @details The reading issues the events as the record is parsed without holding it in full.
			 * The parsing is conducted without a recursion: the nesting is held by a stack of the containers rather than by the stack
			 * of the calls
			 * @details **Independence from the cutting into chunks.** A unit of the record submitted not
			 * in full does not shift the parsing: the missing octets wait for the next submission, while
			 * the issuance of the events does not depend in any way on how the record is cut
			 * @note **The reader is to be held between the records rather than created for each one.**
			 * The reset via `reset()` preserves the capacity of the buffer, while a new reader creates
			 * the buffer anew. The price of that is small by itself, yet it does not depend on the size
			 * of what is being parsed at all and therefore falls entirely upon the cheap parsing
			 *
			 * \~
			 * @warning **The work is NOT protected by a lock: one object — one thread.** Only `Editor`
			 * holds a lock, and one must not judge the others by it. A measurement of 25.08.2026, one `Fetcher`
			 * on four threads: thirteen reports of TSan and nineteen records of four hundred read wrongly,
			 * silently. An own object per thread over a SHARED source of the reading: zero reports,
			 * zero divergences
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора записи
					 *
					 * \~english
					 * @brief Settings of the parsing of a record
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						/**
						 * \~russian
						 * Признак разбора потока документов
						 *
						 * @note Поток этот - последовательность документов подряд, без всякого
						 * разделителя: длина у двоичной записи объявлена, и конец документа
						 * опознаётся ею, а не знаком
						 *
						 * \~english
						 * Flag of the parsing of a stream of the documents
						 * @note This stream is a sequence of the documents in a row without any
						 * separator: the length of a binary record is declared, and the end of a document
						 * is recognised by it rather than by a character
						 *
						 * \~
						 */
						bool stream;
						/**
						 * \~russian
						 * Правило обращения со строкой, не отвечающей кодировке UTF-8
						 *
						 * @details Правило это стоит НА МЕСТЕ прежнего признака проверки строк, а
						 * не рядом с ним: две настройки на одно дело суть ловушка, где одна молча
						 * отменяет другую. `REFUSE` отвечает прежнему признаку поднятым, `PASS` -
						 * снятым, а `REPLACE` заведён им обоим третьим исходом
						 *
						 * @note `REPLACE` подменяет негодную последовательность знаком U+FFFD
						 * НАИБОЛЬШЕЙ ОСМЫСЛЕННОЙ ЧАСТЬЮ, как то предписано Юникодом, и содержимое
						 * события ложится в отдельное хранилище исправленных строк: длина замены
						 * с длиною подменяемого не совпадает, и отрезок в буфере разбора её не
						 * вместил бы. Признак `record_t::repaired` сообщает потребителю, откуда
						 * взято содержимое
						 *
						 * @note Сличение имён полей - на повтор и на возрастание - ведётся по
						 * записи, а НЕ по исправленной строке. Оттого `REPLACE` повторов не
						 * создаёт: два разных негодных имени, обратившихся в один знак замены,
						 * повтором не станут. Уклад этот намеренный: поверка строгого вида есть
						 * договор о ЗАПИСИ, и исправление содержимого его не касается.
						 * Закреплено проверкой `CodecAbcReader.RepairCreatesNoDuplicates`,
						 * утверждающей обе половины: исправление повторов НЕ создаёт и
						 * настоящего повтора НЕ прячет
						 *
						 * @note Умолчанием взят ОТКАЗ: строка объявлена кодировкой UTF-8, а данные,
						 * ей не подчинённые, записываются двоичным значением, - на то оно и
						 * заведено. `REPLACE` есть уступка носителю, где негодные октеты уже
						 * лежат и переписать их нельзя
						 *
						 * \~english
						 * Rule of the handling of a string not conforming to the UTF-8 encoding
						 * @details This rule stands IN PLACE of the former flag of the checking of the strings
						 * rather than beside it: two settings for one matter are a trap
						 * @note `REPLACE` substitutes a malformed sequence by the U+FFFD character by the
						 * MAXIMAL SUBPART, and the content of the event is placed into a separate storage
						 *
						 * \~
						 */
						malformed_t malformed;
						/**
						 * \~russian
						 * Признак поверки записи на строгий вид
						 *
						 * @details Строгий вид требует трёх условий разом: наименьшая запись всякой
						 * метки (значение, вместимое наименьшей шириной, шире не пишется), запрет
						 * неопределённой длины и возрастание имён полей отображения. Сборка тем же
						 * признаком (`Writer::Settings::canonical`) их соблюдает, а заголовок
						 * контейнера объявляет признаком `flag_t::CANONICAL`
						 *
						 * @note Признак этот по умолчанию снят: запись, собранная не строгим видом,
						 * годна к разбору, и поверка навязывалась бы всякому потребителю. Строгий вид
						 * нужен там, где запись подписывается либо сличается октет в октет: одно и то
						 * же значение обязано записываться единственным способом
						 *
						 * \~english
						 * Flag of the checking of the record for the strict kind
						 * @details The strict kind demands three conditions at once: the smallest record of
						 * every tag, the prohibition of the indefinite length and the ascending of the names
						 * of the fields of a mapping
						 * @note This flag is removed by default
						 *
						 * \~
						 */
						bool canonical;
						/**
						 * \~russian
						 * Правило обращения с повторяющимся именем поля отображения
						 *
						 * @details Потоковому разбору доступны лишь ДВА исхода из четырёх: отказ и
						 * пропуск. Выбор одного из двух значений (`FIRST`, `LAST`) требует видеть
						 * отображение целиком, а разбор выдаёт события по одному и назад не ходит, -
						 * решают их дерево документа и владеющее значение. Здесь `FIRST`, `LAST` и
						 * `KEEP` ведут себя одинаково: событие повтора выдаётся как есть
						 *
						 * @note Умолчанием взят ОТКАЗ, как то и объявлено у самого правила: повтор
						 * имени есть приём путаницы разборов, и принимать его молча небезопасно.
						 * Сборщик записи отвергает повтор своим умолчанием точно так же
						 *
						 * \~english
						 * Rule of the handling of a repeating name of a field of a mapping
						 * @details Only TWO outcomes of the four are available to the streaming parsing:
						 * the refusal and the passing through. The choice of one of the two values requires
						 * seeing the whole mapping, whereby it is decided by the tree of the document
						 * @note The refusal is taken by default, as is declared at the rule itself
						 *
						 * \~
						 */
						duplicate_t duplicates;
						// Наибольшая допустимая длина строкового значения в октетах, ноль - без предела
						uint64_t maxString;
						// Наибольшая допустимая длина двоичного значения в октетах, ноль - без предела
						uint64_t maxBlob;
						// Наибольшая допустимая глубина вложенности, ноль - предел модуля
						uint32_t maxDepth;
						// Наибольшее допустимое количество узлов документа, ноль - предел модуля
						uint32_t maxNodes;
						/**
						 * \~russian
						 * Наибольшее допустимое количество неснятых событий, ноль - без предела
						 *
						 * @details Пределы на строку, на данные, на глубину и на узлы держат разбор,
						 * а память держит очередь событий: потребитель, подающий и не снимающий,
						 * растит её без всякой границы. Замер 24.08.2026, поток 79 720 окт. о 20 000
						 * документах: очередь снимается - 20 624 окт. взято, не снимается -
						 * 21 233 552 окт., умножение в 266 раз
						 *
						 * @note Предел этот по умолчанию снят: подающий и снимающий подряд в него не
						 * упрётся никогда, а поточному потребителю он даёт границу, за какой подача
						 * отвечает отказом вместо молчаливого роста
						 *
						 * \~english
						 * Greatest admissible number of the unclaimed events, zero — without a limit
						 * @details The limits on a string, on data, on the depth and on the nodes hold the parsing,
						 * while the memory is held by the queue of the events
						 * @note This limit is removed by default
						 *
						 * \~
						 */
						size_t maxEvents;
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
					/**
					 * \~russian
					 * @brief Значение, выданное событием разбора
					 *
					 * @details Значение ссылается на память, принадлежащую разбору, и живёт не
					 * дольше следующей подачи куска записи
					 *
					 * @note Число выдаётся тем видом, каким оно записано: разбор его не
					 * преобразует и не огрубляет. Целое свыше родных видов и десятичное
					 * выдаются октетами величины вместе со знаком и порядком
					 *
					 * \~english
					 * @brief Value issued by an event of the parsing
					 * @details The value refers to the memory belonging to the parsing and lives no
					 * longer than the next submission of a chunk of the record
					 * @note A number is issued by that kind by which it is recorded: the parsing does not
					 * convert and does not coarsen it. An integer above the native kinds and a decimal one
					 * are issued by the octets of the magnitude together with the sign and the exponent
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Value {
						/**
						 * \~russian
						 * Содержимое значения
						 *
						 * @note У строки и у двоичных данных - их октеты; у опознавателя -
						 * шестнадцать его октетов; у целого свыше родных видов и у десятичного -
						 * октеты величины от младшего к старшему
						 *
						 * \~english
						 * Content of the value
						 * @note For a string and for binary data — their octets; for an identifier —
						 * its sixteen octets; for an integer above the native kinds and for a decimal one —
						 * the octets of the magnitude from the low one to the high one
						 *
						 * \~
						 */
						string_view data;
						// Вид значения
						type_t type;
						// Количество значений вместимого, ноль при неопределённой длине
						uint64_t count;
						// Целое без знака
						uint64_t number;
						// Целое со знаком, а у отметки времени - её значение
						int64_t integer;
						// Дробное число
						double real;
						// Десятичный порядок величины
						int64_t exponent;
						// Логическое значение
						bool boolean;
						// Признак того, что величина меньше нуля
						bool negative;
						// Признак неопределённой длины вместимого
						bool indefinite;
						/**
						 * \~russian
						 * Признак того, что содержимое строки было исправлено
						 *
						 * @note Признак этот поднимается лишь правилом `malformed_t::REPLACE` и
						 * лишь на строке, кодировке не отвечавшей. Потребителю он нужен затем,
						 * что исправление есть ПОТЕРЯ: негодные октеты подменены знаком замены
						 * и восстановлению не подлежат, - и решение, годится ли такое значение
						 * в дело, принадлежит потребителю, а не кодеку
						 *
						 * \~english
						 * Flag that the content of the string was repaired
						 * @note This flag is raised only by the rule `malformed_t::REPLACE`
						 *
						 * \~
						 */
						bool repaired;
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
						Value() noexcept :
						 type(type_t::UNDEFINED), count(0), number(0), integer(0), real(0.0),
						 exponent(0), boolean(false), negative(false), indefinite(false),
						 repaired(false) {}
					} value_t;
					/**
					 * \~russian
					 * @brief Обработчик прямой выдачи событий разбора
					 *
					 * \~english
					 * @brief Handler of the direct issuance of the events of the parsing
					 *
					 * \~
					 */
					typedef void (* handler_t) (void * context, Reader & reader, const event_t event);
				private:
					/**
					 * \~russian
					 * @brief Состояние разбора записи
					 *
					 * @details Состояние хранит то, чего не видно в поданном куске: единица,
					 * поданная наполовину, переводит разбор в отдельное состояние, а не требует
					 * недостающих октетов немедленно
					 *
					 * \~english
					 * @brief State of the parsing of a record
					 * @details The state holds that which is not visible in the submitted chunk: a unit
					 * submitted by half puts the parsing into a separate state rather than demanding
					 * the missing octets immediately
					 *
					 * \~
					 */
					enum class state_t : uint8_t {
						ITEM,             // Ожидается ведущий октет очередной единицы
						SEGMENT,          // Ожидаются октеты строки либо двоичных данных
						SINGLE,           // Ожидаются октеты одиночного значения
						EXTEND_EXPONENT,  // Ожидается десятичный порядок величины
						EXTEND_LENGTH,    // Ожидается длина октетов величины
						EXTEND_SIGN,      // Ожидается знак величины
						EXTEND_DATA,      // Ожидаются октеты величины
						SPANNED_WIDTH,    // Ожидаются октеты объявленного размаха вместимого
						CUSTOM_SUBTYPE,   // Ожидается номер подвида открытого расширения
						CUSTOM_LENGTH,    // Ожидается длина октетов открытого расширения
						CUSTOM_DATA,      // Ожидаются октеты открытого расширения
						FINISHED,         // Разбор завершён
						FAILED            // Разбор отвечен отказом
					};
					/**
					 * \~russian
					 * @brief Звено стека вместимых
					 *
					 * \~english
					 * @brief Link of the stack of the containers
					 *
					 * \~
					 */
					typedef struct Frame {
						/**
						 * \~russian
						 * Место записи за вместимым, объявленное размахом, ноль - не объявлено
						 *
						 * @note Место это ПОЛНОЕ, от начала записи, а не смещение в буфере:
						 * буфер ужимается по выдаче событий, и смещение в нём не переживает
						 * ужатия
						 *
						 * \~english
						 * Place of the record past the container declared by the span, zero — not declared
						 * @note This place is FULL, from the beginning of the record, rather than an offset
						 * in the buffer: the buffer is compacted upon the issuing of the events, and an offset
						 * in it does not survive a compaction
						 *
						 * \~
						 */
						uint64_t beyond;
						// Признак того, что вместимое является отображением
						bool mapping;
						// Признак неопределённой длины вместимого
						bool indefinite;
						// Признак того, что следующим ожидается имя поля
						bool expectKey;
						// Количество оставшихся значений вместимого
						uint64_t remain;
						/**
						 * \~russian
						 * Вид значения, собираемого кусками, либо UNDEFINED у вместимого
						 *
						 * @note Значение, собираемое кусками, ведётся тем же стеком, что и
						 * вместимые: закрывается оно тем же концом, и куски его обязаны
						 * учитываться не значениями вместившего, а частями его самого
						 *
						 * \~english
						 * Kind of the value assembled by the chunks, or UNDEFINED at a container
						 * @note A value assembled by the chunks is led by the same stack as the
						 * containers: it is closed by the same end, and its chunks are obliged
						 * to be counted not as the values of the container but as the parts of itself
						 *
						 * \~
						 */
						type_t segment;
						/**
						 * \~russian
						 * Длина кусков значения, собранная вместимым
						 *
						 * @note Предел длины значения поверяется по СУММЕ кусков, а не по всякому
						 * куску порознь: значение неопределённой длины иначе обходило бы предел
						 * дроблением на куски ниже него
						 *
						 * \~english
						 * Length of the pieces of a value gathered by the container
						 * @note The limit of the length of a value is checked by the SUM of the pieces
						 * rather than by every piece separately
						 *
						 * \~
						 */
						uint64_t gathered;
						/**
						 * \~russian
						 * Признак того, что имя поля отображения этим вместимым уже укладывалось
						 *
						 * \~english
						 * Flag that a name of a field of a mapping has already been laid by this container
						 *
						 * \~
						 */
						bool marked;
						/**
						 * \~russian
						 * Запись имени поля отображения, разобранного прежде, у строгого вида
						 *
						 * @details Возрастание имён поверяется сличением с ПРЕДЫДУЩИМ именем, а не со
						 * всеми: у строгого вида имена идут по возрастанию, и повтор встал бы рядом.
						 * Запись держится своей копией, а не отрезком буфера: буфер ужимается по
						 * выдаче событий, и отрезок в нём ужатия не переживает
						 *
						 * @note Вместилище это наполняется ЛИШЬ при строгом виде разбора: вне его
						 * сличать нечего, и памяти оно не занимает вовсе
						 *
						 * \~english
						 * Record of the name of a field of a mapping parsed previously, at the strict kind
						 * @details The ascending is checked by the comparison with the PREVIOUS name
						 * @note This container is filled ONLY at the strict kind of the parsing
						 *
						 * \~
						 */
						vector <uint8_t> key;
						/**
						 * \~russian
						 * Начало части общего перечня имён полей, принадлежащей вместимому
						 *
						 * @details Перечень ведётся общим на весь стек (`_spans` вместе с `_pool`), а
						 * звено держит лишь начало своей части: перечень в самом звене заводил бы
						 * память на всякое отображение записи
						 *
						 * @note Перечень наполняется ЛИШЬ при правиле `duplicate_t::REFUSE` вне
						 * строгого вида: строгий вид отсеивает повтор возрастанием, а прочие
						 * правила решаются деревом, а не потоковым разбором
						 *
						 * \~english
						 * Beginning of the part of the shared list of the names of the fields belonging to a container
						 * @note The list is filled ONLY at the rule `duplicate_t::REFUSE` outside the strict kind
						 *
						 * \~
						 */
						size_t base;
						/**
						 * \~russian
						 * Начало части общего вместилища октетов имён, принадлежащей вместимому
						 *
						 * \~english
						 * Beginning of the part of the shared container of the octets of the names belonging to a container
						 *
						 * \~
						 */
						size_t mark;
						/**
						 * \~russian
						 * Величина указателя имён полей вместимого, ведомого открытой засылкой
						 *
						 * @details Сам указатель лежит НЕ здесь, а в запасе разбирателя
						 * (`_tables`) по глубине вложенности звена, звено же держит лишь величину
						 * его: нуль означает, что указателя вместимое не завело. Отделены они
						 * ради выделений - звено заводится на всякое вместимое записи, и указатель
						 * внутри него выделял бы память всякий раз заново, тогда как запас
						 * разбирателя переживает и вместимое, и самоё запись
						 *
						 * @note Указатель заводится ЛИШЬ по достижении порога `HASHING_NAMES`
						 * имён: малому отображению обход перечня дешевле, ибо обход десятка
						 * свёрток укладывается в считанные такты, а указателю надобно обнуление
						 * гнёзд
						 *
						 * \~english
						 * Index of the names of the fields of a container, kept by open addressing
						 * @note Created ONLY upon reaching the threshold `HASHING_NAMES` of the names
						 *
						 * \~
						 */
						size_t hashed;
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
						Frame() noexcept :
						 beyond(0), mapping(false), indefinite(false), expectKey(false), remain(0),
						 segment(type_t::UNDEFINED), gathered(0), marked(false), base(0), mark(0), hashed(0) {}
					} frame_t;
					/**
					 * \~russian
					 * @brief Собранное событие разбора
					 *
					 * @details Содержимое хранится отрезком в буфере разбора, а не видом на
					 * него: буфер растёт дописыванием и вправе переехать в памяти
					 *
					 * \~english
					 * @brief Assembled event of the parsing
					 * @details The content is held by a segment in the buffer of the parsing rather than by a view
					 * of it: the buffer grows by an appending and may move in the memory
					 *
					 * \~
					 */
					typedef struct Record {
						// Вид события разбора
						event_t event;
						// Вид значения
						type_t type;
						// Отрезок содержимого значения в буфере разбора
						span_t span;
						// Количество значений вместимого
						uint64_t count;
						// Целое без знака
						uint64_t number;
						// Целое со знаком
						int64_t integer;
						// Дробное число
						double real;
						// Десятичный порядок величины
						int64_t exponent;
						// Логическое значение
						bool boolean;
						// Признак того, что величина меньше нуля
						bool negative;
						// Признак неопределённой длины вместимого
						bool indefinite;
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
						/**
						 * \~russian
						 * Место события в поданной записи
						 *
						 * @note Место запоминается событием, а не берётся состоянием
						 * разбирателя: события копятся очередью, и состояние к выдаче события
						 * уходит вперёд тем дальше, чем крупнее поданный кусок. Место,
						 * взятое состоянием, разошлось бы от одной лишь нарезки на куски
						 *
						 * \~english
						 * Place of the event in the submitted record
						 * @note The place is remembered by the event rather than being taken from the state
						 * of the parser: the events accumulate in a queue, and by the issuing of an event the state
						 * goes ahead the further the larger the submitted chunk is. A place
						 * taken from the state would diverge from the slicing into the chunks alone
						 *
						 * \~
						 */
						location_t location;
						/**
						 * \~russian
						 * Признак того, что содержимое взято из хранилища исправленных строк
						 *
						 * @note Признак этот поднимается лишь правилом `malformed_t::REPLACE` и
						 * лишь на строке, кодировке не отвечавшей: длина замены с длиною
						 * подменяемого не совпадает, и отрезок в буфере разбора её не вместил бы
						 *
						 * \~english
						 * Flag that the content is taken from the storage of the repaired strings
						 * @note This flag is raised only by the rule `malformed_t::REPLACE`
						 *
						 * \~
						 */
						bool repaired;
						/**
						 * \~russian
						 * Отрезок исправленного содержимого в хранилище исправленных строк
						 *
						 * @note Отрезок этот заведён ОТДЕЛЬНЫМ от `span` намеренно: `span`
						 * обязан смотреть в буфер разбора ВСЕГДА, ибо им пользуется сличение
						 * имён полей - и на повтор, и на возрастание, - а сличение это ведётся
						 * по ЗАПИСИ имени, а не по исправленному содержимому его. Подмени
						 * исправление отрезок записи, сличение читало бы чужое хранилище
						 *
						 * \~english
						 * Segment of the repaired content in the storage of the repaired strings
						 * @note This segment is made SEPARATE from `span` deliberately: `span` must look
						 * into the buffer of the parsing ALWAYS, for it is used by the comparison of the names
						 *
						 * \~
						 */
						span_t patch;
						Record() noexcept :
						 event(event_t::NONE), type(type_t::UNDEFINED), count(0), number(0),
						 integer(0), real(0.0), exponent(0), boolean(false), negative(false),
						 indefinite(false), repaired(false) {}
					} record_t;
				private:
					// Настройки разбора записи
					settings_t _settings;
				private:
					// Состояние разбора записи
					state_t _state;
				private:
					// Код отказа разбора
					error_t _error;
				private:
					// Положение разбора в поданной записи
					location_t _location;
				private:
					// Буфер накопленных октетов поданной записи
					vector <uint8_t> _buffer;
				private:
					/**
					 * \~russian
					 * Место начала разбираемой единицы
					 *
					 * @note Место это запоминается началом единицы и держится до следующей:
					 * значение, разбираемое несколькими подачами, началось раньше, а место
					 * события обязано указывать на начало его, а не на конец
					 *
					 * \~english
					 * Place of the beginning of the unit being parsed
					 * @note This place is remembered at the beginning of a unit and is held until the next one:
					 * a value parsed by several submissions began earlier, while the place
					 * of the event is obliged to point to its beginning rather than to its end
					 *
					 * \~
					 */
					location_t _mark;
				private:
					/**
					 * \~russian
					 * Ведущие октеты разбираемой единицы у строгого вида разбора
					 *
					 * @details Сличение имён полей отображения идёт по ПОЛНОЙ записи имени, вместе
					 * с меткою: так же сличает их и сборка, и всякий иной порядок разошёлся бы с нею.
					 * Метка вместе с ведомой ею записью держится здесь, а содержимое берётся отрезком
					 * события в миг выдачи его
					 *
					 * @note Вместилище это наполняется ЛИШЬ при строгом виде разбора
					 *
					 * \~english
					 * Leading octets of the unit being parsed at the strict kind of the parsing
					 * @details The comparison of the names of the fields of a mapping goes by the FULL record
					 * of a name together with the tag, the same way as the writing compares them
					 * @note This container is filled ONLY at the strict kind of the parsing
					 *
					 * \~
					 */
					vector <uint8_t> _lead;
				private:
					// Смещение разбора в буфере накопленных октетов
					size_t _offset;
				private:
					// Смещение начала буфера от начала поданной записи
					uint64_t _origin;
				private:
					// Размер места, выданного под приём октетов записи
					size_t _reserved;
				private:
					// Стек вместимых разбора
					vector <frame_t> _stack;
				private:
					/**
					 * \~russian
					 * @brief Отрезок записи имени поля отображения вместе со свёрткой его
					 *
					 * @details Свёртка держится РЯДОМ с отрезком намеренно: сличение имён идёт
					 * со всеми прежними именами вместимого, то есть квадратом их количества, и
					 * полное сличение октетов на всякой паре обходилось дорого. Замер 30.08.2026
					 * дал выигрыш от пропуска груза 26 крат против 55 при снятой поверке, а со
					 * снятым одним лишь обходом - 49: плата сидела именно в обходе, а не в
					 * переносе имён. Свёртка обращает сличение в числовое, а полное остаётся
					 * лишь при совпадении её
					 *
					 * \~english
					 * @brief Segment of the record of the name of a field of a mapping together with its digest
					 * @details The digest is kept BESIDE the segment deliberately: the comparison of the names
					 * goes over all the previous names of the container, that is, by the square of their number
					 *
					 * \~
					 */
					typedef struct Naming {
						// Смещение записи имени поля в общем вместилище октетов
						uint32_t offset;
						// Длина записи имени поля отображения
						uint32_t length;
						// Свёртка записи имени поля отображения
						uint64_t digest;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param offset смещение записи имени поля
						 * @param length длина записи имени поля
						 * @param digest свёртка записи имени поля
						 *
						 * \~english
						 * @brief Constructor
						 * @param offset offset of the record of the name of a field
						 * @param length length of the record of the name of a field
						 * @param digest digest of the record of the name of a field
						 *
						 * \~
						 */
						Naming(const uint32_t offset = 0, const uint32_t length = 0, const uint64_t digest = 0) noexcept :
						 offset(offset), length(length), digest(digest) {}
					} naming_t;
					/**
					 * \~russian
					 * Отрезки записей имён полей всех вместимых стека
					 *
					 * @details Перечень общий: часть его, принадлежащая звену, начинается со
					 * смещения «frame_t::base», а снятие звена усекает перечень до него. Общим он
					 * заведён ради памяти: вместимость держится между отображениями, тогда как
					 * перечень в самом звене заводился бы заново на всякое из них
					 *
					 * @note Наполняется ЛИШЬ при правиле `duplicate_t::REFUSE` вне строгого вида
					 *
					 * \~english
					 * Segments of the records of the names of the fields of all the containers of the stack
					 * @note Filled ONLY at the rule `duplicate_t::REFUSE` outside the strict kind
					 *
					 * \~
					 */
					vector <naming_t> _spans;
				private:
					/**
					 * \~russian
					 * Запас указателей имён полей по глубине вложенности вместимых
					 *
					 * @details Указатель берётся глубиною звена в стеке, а не заводится внутри
					 * него: звено живёт одним вместимым, тогда как запас переживает и запись, -
					 * оттого выделение памяти случается однажды на разбиратель, а не на всякое
					 * отображение. Сбросом разбора запас НЕ освобождается: переиспользуемый
					 * разбиратель тем и ценен, что памяти заново не просит
					 *
					 * @note Гнездо держит место имени в общем перечне `_spans`, увеличенное на
					 * единицу: нуль означает гнездо пустое
					 *
					 * \~english
					 * Reserve of the indexes of the names of the fields by the depth of the nesting of the containers
					 * @note A slot holds the place of a name in the shared list `_spans` increased by one
					 *
					 * \~
					 */
					vector <vector <uint32_t>> _tables;
				private:
					/**
					 * \~russian
					 * Октеты записей имён полей всех вместимых стека
					 *
					 * @note Октеты держатся своей копией, а не отрезком буфера разбора: буфер
					 * ужимается по выдаче событий, и отрезок в нём ужатия не переживает
					 *
					 * \~english
					 * Octets of the records of the names of the fields of all the containers of the stack
					 *
					 * \~
					 */
					vector <uint8_t> _pool;
					/**
					 * \~russian
					 * Хранилище строки, исправленной правилом `malformed_t::REPLACE`
					 *
					 * @details Хранилище это заведено отдельным от буфера разбора НАМЕРЕННО:
					 * знак замены занимает три октета, а подменяемая последовательность - от
					 * одного до четырёх, и вписать исправленную строку на место негодной значило
					 * бы двигать всё, что за нею стоит, вместе со смещениями разбора. Хранилище
					 * живёт одним событием и переписывается на всякой исправленной строке
					 *
					 * \~english
					 * Storage of a string repaired by the rule `malformed_t::REPLACE`
					 * @details This storage is made separate from the buffer of the parsing DELIBERATELY
					 *
					 * \~
					 */
					vector <uint8_t> _repair;
				private:
					/**
					 * \~russian
					 * Очередь собранных событий разбора
					 *
					 * @details Очередь ведётся кольцом на вместилище, память какого держится
					 * между подачами: событий у крупной записи миллионы, и заводись вместилище
					 * под них заново, расход выделений рос бы с размером записи
					 *
					 * \~english
					 * Queue of the assembled events of the parsing
					 * @details The queue is led by a ring on a container whose memory is held
					 * between the submissions: a large record has millions of events, and were the container
					 * created anew, the expenditure of the allocations would grow with the size of the record
					 *
					 * \~
					 */
					vector <record_t> _events;
				private:
					// Смещение первого невыданного события очереди
					size_t _head;
				private:
					// Событие, выданное последним переходом
					record_t _current;
				private:
					// Количество разобранных узлов документа
					uint32_t _nodes;
				private:
					// Количество недостающих октетов ожидаемой записи
					uint64_t _pending;
				private:
					// Вид значения, чьи октеты ожидаются
					type_t _awaited;
				private:
					// Разновидность ожидаемого расширения
					extend_t _extend;
				private:
					/**
					 * \~russian
					 * Размах вместимого, объявленный меткою, ноль - не объявлен
					 *
					 * @note Размах считается от конца записи его: чтение, пропускающее
					 * вместимое, прибавляет размах к своему месту и оказывается за ним
					 *
					 * \~english
					 * Span of the container declared by a tag, zero — not declared
					 * @note The span is counted from the end of its record: a reading skipping
					 * the container adds the span to its place and finds itself past it
					 *
					 * \~
					 */
					uint64_t _span;
				private:
					// Место записи за вместимым, объявленное последнею меткою размаха
					uint64_t _beyond;
				private:
					/**
					 * \~russian
					 * Признак затребованного пропуска открываемого вместимого
					 *
					 * @note Пропуск объявляется обработчиком прямой выдачи ПОСРЕДИ открытия
					 * вместимого: событие начала его выдано, а звено стека ещё не заведено.
					 * Признак этот и переносит волю обработчика к тому месту, где вместимое
					 * заводится
					 *
					 * \~english
					 * Sign of a demanded skipping of the container being opened
					 * @note The skipping is declared by the handler of the direct issuing IN THE MIDDLE of
					 * the opening of a container: the event of its beginning is issued, while the link of
					 * the stack is not yet created
					 *
					 * \~
					 */
					bool _skipping;
				private:
					// Десятичный порядок ожидаемой величины
					int64_t _exponent;
				private:
					// Номер подвида ожидаемого открытого расширения
					uint64_t _subtype;
				private:
					// Признак того, что величина ожидаемого расширения меньше нуля
					bool _negative;
				private:
					// Признак завершённости хотя бы одного документа
					bool _document;
				private:
					// Обработчик прямой выдачи событий разбора
					handler_t _handler;
				private:
					// Опора обработчика прямой выдачи событий разбора
					void * _context;
				protected:
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод объявления отказа разбора
					 *
					 * @param error код отказа разбора
					 * @return      признак успешности разбора, всегда ложь
					 *
					 * \~english
					 * @brief Method of the declaration of a refusal of the parsing
					 * @param error error code of the parsing
					 * @return sign of the success of the parsing, always false
					 *
					 * \~
					 */
					[[nodiscard]] bool fail(const error_t error) noexcept;
					/**
					 * \~russian
					 * @brief Метод перестроения указателя имён полей вместимого
					 *
					 * @details Указатель заводится заново по всей части перечня, принадлежащей
					 * звену: перестроение обходится дешевле поддержания указателя при усечении
					 * перечня, а случается оно на удвоении числа имён, то есть впятеро реже
					 *
					 * @param depth  глубина вложенности звена, чей указатель перестраивается
					 * @param frame  звено стека вместимых, чей указатель перестраивается
					 * @param needed число имён, какое указателю надлежит вместить
					 *
					 * \~english
					 * @brief Method of the rebuilding of the index of the names of the fields of a container
					 * @param frame  frame of the stack of the containers whose index is rebuilt
					 * @param needed number of the names which the index has to hold
					 *
					 * \~
					 */
					void rehash(const size_t depth, frame_t & frame, const size_t needed) noexcept;
					/**
					 * \~russian
					 * @brief Метод поверки имени поля отображения на повтор
					 *
					 * @details Способ поверки выбирается числом имён вместимого: малое
					 * отображение сличается обходом перечня, а по достижении порога
					 * `HASHING_NAMES` заводится указатель, и сличение идёт гнездом. Обход
					 * растёт квадратом числа имён, и на больших отображениях он и был главною
					 * платою поверки (замер 30.08.2026)
					 *
					 * @note Имя, повтором НЕ оказавшееся, вносится в указатель здесь же, но в
					 *       общий перечень его укладывает зовущий: перечень и вместилище
					 *       октетов ведутся вместе, и разрывать их надвое незачем
					 *
					 * @param frame  звено стека вместимых, чьё имя поля поверяется
					 * @param offset смещение записи имени поля в общем вместилище октетов
					 * @param length длина записи имени поля отображения
					 * @param digest свёртка записи имени поля отображения
					 * @return       признак того, что имя поля в этом вместимом уже было
					 *
					 * \~english
					 * @brief Method of the check of the name of a field of a mapping for a duplicate
					 * @param frame  frame of the stack of the containers whose name of a field is checked
					 * @param offset offset of the record of the name of a field in the shared container of the octets
					 * @param length length of the record of the name of a field of a mapping
					 * @param digest digest of the record of the name of a field of a mapping
					 * @return sign that the name of a field has already been in this container
					 *
					 * \~
					 */
					[[nodiscard]] bool duplicated(frame_t & frame, const size_t offset, const size_t length, const uint64_t digest) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи собранного события разбора
					 *
					 * @param record собранное событие разбора
					 *
					 * \~english
					 * @brief Method of the issuance of an assembled event of the parsing
					 * @param record assembled event of the parsing
					 *
					 * \~
					 */
					void emit(const record_t & record) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи события завершённого значения
					 *
					 * @details Работа сама решает, событием значения выдать запись либо
					 * событием имени поля, и сама закрывает вместимые, чьи значения исчерпаны
					 *
					 * @param record собранное событие разбора
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the issuance of an event of a completed value
					 * @details The work itself decides whether to issue the record by an event of a value or
					 * by an event of a name of a field, and itself closes the containers whose values are exhausted
					 * @param record assembled event of the parsing
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool settle(record_t & record) noexcept;
					/**
					 * \~russian
					 * @brief Метод учёта завершённого значения вместившим его
					 *
					 * @param record выдаваемое событие завершённого значения
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the accounting of a completed value by its container
					 * @param record issued event of the completed value
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool finalize(record_t & record) noexcept;
					/**
					 * \~russian
					 * @brief Метод закрытия исчерпанных вместимых
					 *
					 * @return признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the closing of the exhausted containers
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool unwind() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора накопленных октетов записи
					 *
					 * @return признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the accumulated octets of a record
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool process() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора очередной единицы проволочной записи
					 *
					 * @param done признак того, что единицу разобрать не удалось
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the next unit of the wire record
					 * @param done sign that the unit could not be parsed
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool item(bool & done) noexcept;
					/**
					 * \~russian
					 * @brief Метод усечения разобранной части буфера
					 *
					 * \~english
					 * @brief Method of the truncation of the parsed part of the buffer
					 *
					 * \~
					 */
					void trim() noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора накопленных октетов вместе с окончанием записи
					 *
					 * @param last признак того, что поданный кусок записи последний
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the parsing of the accumulated octets together with the end of the record
					 * @param last flag that the submitted chunk of the record is the last one
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool digest(const bool last) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса состояния разбора
					 *
					 * @details Работа эта возвращает разбирателя к началу: снимает код отказа,
					 * очередь событий и накопленные октеты, - после чего он годен разбирать
					 * запись заново. Звать её надлежит В ДВУХ случаях: после отказа, чтобы
					 * оживить разбирателя, и МЕЖДУ ЗАПИСЯМИ потока, ибо законченная запись
					 * делает разбирателя законченным наравне с отказавшим
					 *
					 * @note Отличие от `abort` в том, что `reset` ОЖИВЛЯЕТ, а `abort`
					 *       прекращает: код отказа переживает `abort` и не переживает `reset`.
					 *       Дороги эти близнецы по виду и противоположны по смыслу
					 *
					 * \~english
					 * @brief Method of the reset of the state of the parsing
					 * @details Removes the code of the failure, the queue of the events and the accumulated octets
					 *
					 * \~
					 */
					void reset() noexcept;
					/**
					 * \~russian
					 * @brief Метод прекращения разбора
					 *
					 * @details Работа эта прекращает разбор, СОХРАНЯЯ код отказа: разбиратель
					 * остаётся мёртвым, и всякая последующая подача отвечает отказом. Заведена
					 * она для потребителя, которому надо остановить разбор, не потеряв причины,
					 * - скажем, чтобы донести её выше по слоям и лишь затем оживить
					 *
					 * @warning Работа эта разбирателя НЕ ОЖИВЛЯЕТ, и путать её с `reset` не
					 *          следует: позвавший `abort` в надежде продолжить получит отказ
					 *          на всякой подаче, а причина его будет прежняя - та, что была до
					 *          прекращения, - и оттого укажет не туда
					 *
					 * \~english
					 * @brief Method of the termination of the parsing
					 * @details Terminates the parsing KEEPING the code of the failure
					 *
					 * \~
					 */
					void abort() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод подачи куска разбираемой записи
					 *
					 * @details Разбиратель живёт ОДНОЙ записью, а не потоком записей. Подача,
					 * помеченная `last`, завершает запись, и разбиратель обращается в
					 * законченный: подай ему следом вторую запись - и она отвергнется с
					 * причиною `TRAILING_OCTETS`, ибо октеты её лежат за концом документа
					 *
					 * @note Отсюда правило потребителю, разбирающему ПОТОК записей: между
					 *       записями звать `reset`. Правило это держится и на пути успеха, а
					 *       не только после отказа, - и в том его коварство: разбор первой
					 *       записи удался, а вторая молча отвергается, и причина её лежит не
					 *       в ней самой
					 *
					 * @note Судьба ОТКАЗА иная, и различать их надлежит. Отказ оставляет
					 *       разбирателя мёртвым: всякая последующая подача отвечает отказом с
					 *       прежнею причиною, покуда не позван `reset`. Вернуть его к работе
					 *       умеет только `reset`, а `abort` - НЕ умеет, он прекращает
					 *
					 * @warning События, выданные ДО отказа, у разбирателя остаются, и вычерпать
					 *          их потребитель может. События эти суть НАЧАЛО записи, а не
					 *          запись: потокового разбирателя нельзя заставить взять выданное
					 *          обратно, оттого сам признак отказа и есть единственное, чем
					 *          отличить полную запись от оборванной. Дерево документа тем от
					 *          него и отличается, что собирает целое и при отказе снимает его
					 *          целиком
					 *
					 * @param buffer буфер подаваемой записи
					 * @param size   размер буфера подаваемой записи
					 * @param last   признак того, что кусок последний
					 * @return       признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the feeding of a chunk of a record being parsed
					 * @param buffer buffer of the record being fed
					 * @param size size of the buffer of the record being fed
					 * @param last flag that the chunk is the last one
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool feed(const void * buffer, const size_t size, const bool last = false) noexcept;
					/**
					 * \~russian
					 * @brief Метод выдачи места под приём октетов записи
					 *
					 * @details Место это выдаётся в самом буфере разбора: принятое ложится в него
					 * сразу, и подача разбирателю лишнего копирования не стоит вовсе
					 *
					 * @note Выданное место годно до ближайшего вызова, приняв октеты, обязан звать
					 * commit: всякий иной вызов вправе буфер подвинуть, и указатель станет негоден
					 *
					 * @param size размер запрашиваемого места в октетах
					 * @return     указатель на выданное место, ноль - разбор отвечен отказом
					 *
					 * \~english
					 * @brief Method of the issuance of a place for the reception of the octets of a record
					 * @details This place is issued in the very buffer of the parsing: the received lies into it
					 * at once, and the submission to the parser does not cost a superfluous copying at all
					 * @note The issued place is valid until the nearest call, and having received the octets one is obliged to call
					 * commit: any other call has the right to move the buffer, and the pointer will become invalid
					 * @param size size of the requested place in octets
					 * @return pointer to the issued place, zero — the parsing is answered by a refusal
					 *
					 * \~
					 */
					[[nodiscard]] void * reserve(const size_t size) noexcept;
					/**
					 * \~russian
					 * @brief Метод подачи октетов, принятых в выданное место
					 *
					 * @param size размер принятых октетов, не свыше выданного места
					 * @param last признак того, что кусок последний
					 * @return     признак успешности разбора
					 *
					 * \~english
					 * @brief Method of the submission of the octets received into the issued place
					 * @param size size of the received octets, not above the issued place
					 * @param last flag that the chunk is the last one
					 * @return sign of the success of the parsing
					 *
					 * \~
					 */
					[[nodiscard]] bool commit(const size_t size, const bool last = false) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему собранному событию
					 *
					 * @return признак наличия события
					 *
					 * \~english
					 * @brief Method of the transition to the next assembled event
					 * @return sign of the presence of an event
					 *
					 * \~
					 */
					[[nodiscard]] bool next() noexcept;
					/**
					 * \~russian
					 * @brief Метод пропуска открытого вместимого целиком
					 *
					 * @details Пропуск даётся ОДНИМ сложением и только тому вместимому, чей
					 * размах объявлен меткою при сборке (настройка `spanned` у сборки).
					 * Замер 21.08.2026: обход поддерева из ста тысяч значений стоил 2 мс,
					 * и обращение к соседнему полю стоило ровно столько же
					 *
					 * @note Звать надлежит сразу по событию начала вместимого. Пропуск выдаёт
					 * событие конца его, и разбор продолжается с записи, стоящей следом
					 *
					 * @note Отказом отвечается, если размах не объявлен либо если октеты
					 * вместимого ещё не поданы целиком: поточному чтению пропускать нечего,
					 * покуда пропускаемое не пришло
					 *
					 * @note Неопределённая длина пропуску НЕ мешает: размах объявлен в октетах,
					 * и метка конца вместимого лежит внутри него. Проверено 25.08.2026 подачей
					 * всякой нарезкой - от одного октета до целой записи
					 *
					 * @return признак успешного пропуска вместимого
					 *
					 * \~english
					 * @brief Method of the skipping of an opened container as a whole
					 * @details The skipping is given by ONE addition and only to a container whose span
					 * is declared by a tag upon the assembling (the setting `spanned` of the assembling)
					 * @note It ought to be called right upon the event of the beginning of a container. The skipping
					 * issues the event of its end, and the parsing continues from the record standing next
					 * @note It is answered by a refusal if the span is not declared or if the octets of the container
					 * have not yet been submitted as a whole
					 * @note An indefinite length does NOT hinder the skipping: the span is declared in octets,
					 * and the tag of the end of the container lies inside it
					 * @return sign of the success of the skipping of the container
					 *
					 * \~
					 */
					[[nodiscard]] bool skip() noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения вида текущего события
					 *
					 * @return вид текущего события разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the kind of the current event
					 * @return kind of the current event of the parsing
					 *
					 * \~
					 */
					event_t event() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения значения текущего события
					 *
					 * @warning Строка и двоичное содержимое выдаются ССЫЛКОЙ во вместилище
					 * разбора, копии не делается вовсе - ради того событийное чтение и заводят.
					 * Ссылка эта годна ЛИШЬ ДО СЛЕДУЮЩЕЙ ПОДАЧИ: разбор срезает разобранное
					 * начало вместилища, и выданное прежде указывает уже в чужое место.
					 * Пережить подачу должна КОПИЯ, снятая потребителем
					 *
					 * @return значение текущего события разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the value of the current event
					 * @warning A string and a binary content are issued by a REFERENCE into the storage
					 * of the parsing, no copy is made at all. This reference is valid ONLY UNTIL THE NEXT
					 * SUBMISSION: the parsing trims the parsed beginning of the storage. What must outlive
					 * the submission is a COPY taken by the consumer
					 * @return value of the current event of the parsing
					 *
					 * \~
					 */
					value_t value() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод установки обработчика прямой выдачи событий разбора
					 *
					 * @details Обработчик очередь выдачи собою заменяет: событие приходит ему
					 * прямо из разбора, а в очередь не ложится вовсе. Иначе работа выходила бы
					 * двойной, а очередь росла бы без предела - снимать с неё стало бы некому
					 *
					 * @note Оттого при установленном обработчике переход к следующему событию
					 * не выдаёт ничего: события все до одного ушли обработчику
					 *
					 * @param callback устанавливаемый обработчик, ноль - снятие обработчика
					 * @param context  опора обработчика
					 *
					 * \~english
					 * @brief Method of the setting of the handler of the direct issuance of the events of the parsing
					 * @details The handler replaces the queue of the issuance by itself: an event comes to it
					 * directly from the parsing and does not lie into the queue at all. Otherwise the work would come out
					 * double, while the queue would grow without a limit — there would be nobody to take from it
					 * @note Whereby with an installed handler the transition to the next event
					 * issues nothing: all the events to a single one have gone to the handler
					 * @param callback handler being set, zero — removal of the handler
					 * @param context support of the handler
					 *
					 * \~
					 */
					void handler(handler_t callback, void * context) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения кода отказа разбора
					 *
					 * @return код отказа разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the error code of the parsing
					 * @return error code of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения положения разбора в поданной записи
					 *
					 * @return положение разбора в поданной записи
					 *
					 * \~english
					 * @brief Method of the extraction of the position of the parsing in the submitted record
					 * @return position of the parsing in the submitted record
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения глубины вложенности разбора
					 *
					 * @return глубина вложенности разбора
					 *
					 * \~english
					 * @brief Method of the extraction of the depth of the nesting of the parsing
					 * @return depth of the nesting of the parsing
					 *
					 * \~
					 */
					uint32_t depth() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения настроек разбора записи
					 *
					 * @return настройки разбора записи
					 *
					 * \~english
					 * @brief Method of the extraction of the settings of the parsing of a record
					 * @return settings of the parsing of a record
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора записи
					 *
					 * @param settings устанавливаемые настройки разбора записи
					 *
					 * \~english
					 * @brief Method of the setting of the settings of the parsing of a record
					 * @param settings settings of the parsing of a record being set
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
					 *
					 * \~
					 */
					explicit Reader(const log_t * log) noexcept;
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
					~Reader() noexcept {}
			} reader_t;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_ABC_READER__
