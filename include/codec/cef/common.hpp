/**
 * @file common.hpp
 * @date 2026-09-04
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
 * @brief Заголовочный файл общих определений контейнера CEF — коды ошибок разбора, виды событий чтения,
 *        области записи, строгость сличения со словарём расширений, виды значений словаря, правила
 *        обращения с пустым значением и с вложенностью, пределы разбора и положение в исходном тексте
 *
 * \~english
 * @brief Header file of the common definitions of the CEF container — the error codes of the parsing, the kinds of the events of the reading,
 *        the areas of a record, the strictness of the matching against the dictionary of the extensions, the kinds of the values of the dictionary, the rules
 *        of the treatment of an empty value and of a nesting, the limits of the parsing and the position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CEF_COMMON__
#define __AWH_CODEC_CEF_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/log.hpp"
#include "../../sys/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, IP и TIMESTAMP у Sun Solaris.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

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
		 * @brief Пространство имён контейнера CEF
		 *
		 * @details Разбор и запись событий безопасности в записи Common Event Format,
		 * описанной ArcSight: необязательной приставки syslog, заголовка из семи полей,
		 * отделённых прямой чертой, и расширения из пар «ключ=значение», отделённых
		 * пробелом
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Заголовок и расширение суть РАЗНЫЕ области с разным экранированием.**
		 * Описание требует отменять в заголовке прямую черту и обратную косую, а в
		 * расширении - знак равенства, и прямо оговаривает, что черта и косая в
		 * расширении отмены не требуют, а знак равенства в заголовке не требует её
		 * тоже. Свести обе области к одному правилу значило бы разбирать неверно обе,
		 * оттого области разделены и в разборе, и в дереве
		 *
		 * @li **Обход по пути выдаёт СЫРЫЕ ключи, а сведённые имена берутся вторым
		 * ходом.** Запись есть источник, и потребитель, идущий по пути, обязан видеть
		 * то, что в записи стоит: «cs1» и «cs1Label» порознь. Сведение пары
		 * «cs1Label=IDSClass» с «cs1=not-suspicious» в поле с человеческим именем есть
		 * ПЕРЕВОД, а не разбор, и совершать его молча значило бы отдавать потребителю
		 * запись, которой в источнике не было. Невыбранное при этом достижимо: длинные
		 * имена словаря и сведённые метки выдаются отдельными ходами
		 *
		 * @li **Пустое значение есть пустая последовательность знаков.** Запись «cs3=»
		 * описанием не оговорена вовсе - ни разрешена, ни запрещена, - но в живых
		 * журналах обычна и образует пару с «cs3Label=CVEID». Обратить её в логическую
		 * истину значило бы отдать потребителю утверждение, какого в записи не было, и
		 * лишить его различия «поле пусто» и «поле установлено»; оттого умолчание -
		 * последовательность знаков, а иные толкования берутся настройкой
		 *
		 * @li **Пустое значение и отсутствие значения запись CEF НЕ различает.** Ключа
		 * без знака равенства в расширении не бывает, а значения, означающего пустоту,
		 * наподобие `null` записи JSON, у CEF нет вовсе. Оттого сброс значения выдаёт
		 * «cs3=» и от поля, пустым записанного в источнике, неотличим; различие «поле
		 * есть - поля нет» несёт снос пары целиком
		 *
		 * @li **Повтор ключа даёт ОДИН потомок перечнем.** В живых журналах ключ
		 * повторяется - «ad.prog-id» трижды подряд, «deviceExternalId» дважды, - и
		 * терять повтор молча нельзя. Устройство взято у кодека INI, где оно уже
		 * измерено: обход остаётся замкнутым числовыми звеньями пути
		 *
		 * @li **Дерева произвольной глубины у записи CEF нет.** Глубина ограничена
		 * устройством самой записи: приставка, заголовок, расширение - и всё. Это
		 * граница формата, а не недоделка кодека; при записи же дерева обратно
		 * вложенное значение обращается по правилу, настройкой заданному, а не молча
		 *
		 * @li **Разбор пар «ключ=значение» ведётся ходом `fmk_t::kv`.** Устройство его
		 * рассчитано на записи вида CEF: значение может нести разделитель записей и
		 * кончается перед разделителем ключа следующей записи, последняя запись
		 * занимает весь остаток, знак считается отменённым при нечётном числе
		 * предшествующих косых. Заводить второй разбор того же в кодеке значило бы
		 * держать один договор в двух местах
		 *
		 * @li **Опознание вида значения ведётся словарём, а не угадыванием.** Вид поля
		 * берётся из словаря расширений по ключу; строгость сличения задаётся
		 * настройкой. Угадывание вида по виду знаков порождает разночтения: «011»
		 * разбирается то восьмеричным, то десятичным, а «1.10» числом теряет разряд
		 *
		 * @li **Внешних обращений разбор не совершает.** Ни к файловой системе, ни к
		 * сети: разрешение имён устройств, поиск словарей и проверка подписей в задачу
		 * кодека не входят
		 *
		 * \~english
		 * @brief CEF container namespace
		 * @details The parsing and the writing of the events of security in the Common Event Format record
		 * described by ArcSight: of the optional syslog prefix, of the header of seven fields
		 * separated by a vertical bar, and of the extension of the pairs «key=value» separated
		 * by a space
		 * @par Deliberate decisions
		 * What is listed below is not a gap of the implementation: these are the outlined boundaries of the
		 * task, and each of the decisions is fixed by a verifying test
		 * @li **The header and the extension are DIFFERENT areas with different escaping.**
		 * The specification requires escaping the vertical bar and the backslash in the header, and
		 * the equals sign in the extension, and it states explicitly that the bar and the backslash in the
		 * extension require no escaping, and that the equals sign in the header requires none either.
		 * To reduce both areas to one rule would mean to parse both of them wrongly
		 * @li **A traversal by a path issues the RAW keys, while the reduced names are taken by a second
		 * method.** The record is the source, and a consumer going by a path must see
		 * what stands in the record: «cs1» and «cs1Label» separately. The reduction of the pair
		 * «cs1Label=IDSClass» with «cs1=not-suspicious» into a field with a human-readable name is a
		 * TRANSLATION rather than a parsing, and to perform it silently would mean to give the consumer
		 * a record that was not in the source. What is not chosen remains reachable: the long
		 * names of the dictionary and the reduced labels are issued by separate methods
		 * @li **An empty value is an empty sequence of characters.** The record «cs3=»
		 * is not stipulated by the specification at all — neither allowed nor forbidden — but in the living
		 * logs it is common and forms a pair with «cs3Label=CVEID». To turn it into a logical
		 * truth would mean to give the consumer an assertion that was not in the record, and
		 * to deprive him of the distinction «the field is empty» and «the field is set»; therefore the default is
		 * a sequence of characters, while other interpretations are taken by a setting
		 * @li **The CEF record does NOT distinguish an empty value from an absence of a value.** A key
		 * without an equals sign does not occur in an extension, while a value meaning emptiness,
		 * like the `null` of the JSON notation, does not exist in CEF at all. Therefore a resetting of a value issues
		 * «cs3=» and is indistinguishable from a field written empty in the source; the distinction «the field
		 * exists — the field does not» is carried by the erasing of the pair as a whole
		 * @li **A repetition of a key gives ONE descendant as a list.** In the living logs a key
		 * repeats itself — «ad.prog-id» three times in a row, «deviceExternalId» twice — and
		 * the repetition cannot be lost silently. The construction is taken from the INI codec, where it is
		 * already measured: the traversal remains closed by the numeric links of a path
		 * @li **A tree of an arbitrary depth does not exist in a CEF record.** The depth is limited by
		 * the construction of the record itself: the prefix, the header, the extension — and that is all. This is
		 * a boundary of the format rather than an incompleteness of the codec
		 * @li **The parsing of the pairs «key=value» is conducted by the method `fmk_t::kv`.** Its construction
		 * is designed for the records of the CEF kind, and to create a second parsing of the same thing in the codec
		 * would mean to hold one contract in two places
		 * @li **The recognition of the kind of a value is conducted by a dictionary rather than by a guessing.** The kind of a field
		 * is taken from the dictionary of the extensions by the key; the strictness of the matching is given by
		 * a setting
		 * @li **The parsing performs no external calls.** Neither to the file system nor to the
		 * network
		 *
		 * \~
		 */
		namespace cef {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина одной записи CEF в байтах
			 *
			 * @details Предел считается на запись целиком - вместе с приставкой, заголовком
			 * и расширением, - иначе перенос строки внутри значения давал бы обход предела
			 *
			 * \~english
			 * @brief Largest admissible length of one CEF record in bytes
			 * @details The limit is counted over the record as a whole — together with the prefix, the header
			 * and the extension — otherwise a line break inside a value would give a bypass of the limit
			 *
			 * \~
			 */
			constexpr uint32_t MAX_RECORD = 0x100000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина одного поля заголовка в байтах
			 *
			 * \~english
			 * @brief Largest admissible length of one field of the header in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_HEADER_FIELD = 0x4000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени ключа расширения в байтах
			 *
			 * \~english
			 * @brief Largest admissible length of the name of a key of an extension in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NAME = 1024;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество пар расширения у одной записи
			 *
			 * \~english
			 * @brief Largest admissible number of the pairs of an extension of one record
			 *
			 * \~
			 */
			constexpr uint32_t MAX_EXTENSIONS = 0x10000;

			/**
			 * \~russian
			 * @brief Количество пар расширения, начиная с какого заводится указатель имён
			 *
			 * @details Ниже порога имя разыскивается перебором, и это дешевле всякого
			 * указателя: сличение имён идёт по памяти подряд, а заведение указателя стоит
			 * выделения памяти и подсчёта отпечатка на всякое имя
			 *
			 * @note Порог держится равным порогу кодека INI намеренно: устройство хранения
			 * пар у обоих кодеков одно, и расхождение порогов означало бы расхождение
			 * замеров без всякого к тому основания
			 *
			 * \~english
			 * @brief Number of the pairs of an extension starting from which the index of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * index
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;

			/**
			 * \~russian
			 * @brief Количество полей заголовка записи CEF
			 *
			 * @details Полей ровно семь: слово «CEF» с номером редакции, поставщик,
			 * изделие, его редакция, опознаватель события, имя события и важность.
			 * Восьмым полем идёт расширение, к заголовку не относящееся
			 *
			 * \~english
			 * @brief Number of the fields of the header of a CEF record
			 * @details There are exactly seven fields: the word «CEF» with the number of the version, the vendor,
			 * the product, its version, the identifier of the event, the name of the event and the severity.
			 * The eighth field is the extension, which does not belong to the header
			 *
			 * \~
			 */
			constexpr uint32_t HEADER_FIELDS = 7;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое значение важности события
			 *
			 * @details Описание дозволяет числа от нуля до десяти, где десять означает
			 * событие наибольшей важности
			 *
			 * \~english
			 * @brief Largest admissible value of the severity of an event
			 * @details The specification allows the numbers from zero to ten, where ten means
			 * an event of the greatest importance
			 *
			 * \~
			 */
			constexpr uint32_t MAX_SEVERITY = 10;

			/**
			 * \~russian
			 * @brief Обозначение отсутствующего положения в исходном тексте
			 *
			 * \~english
			 * @brief Designation of an absent position in the source text
			 *
			 * \~
			 */
			constexpr uint64_t NO_OFFSET = static_cast <uint64_t> (~0ull);

			/**
			 * \~russian
			 * @brief Обозначение отсутствующей записи разобранного текста
			 *
			 * \~english
			 * @brief Designation of an absent record of the parsed text
			 *
			 * \~
			 */
			constexpr uint32_t NO_RECORD = static_cast <uint32_t> (~0u);

			/**
			 * \~russian
			 * @brief Слово, открывающее заголовок записи
			 *
			 * \~english
			 * @brief Word opening the header of a record
			 *
			 * \~
			 */
			constexpr string_view SIGNATURE = "CEF:";

			/**
			 * \~russian
			 * @brief Окончание имени ключа, несущего имя парного ему поля
			 *
			 * @details Пара «cs1Label=IDSClass» и «cs1=not-suspicious» связывается именно
			 * этим окончанием: ключ, оканчивающийся на него, задаёт человеческое имя
			 * ключу, тем же началом обозначенному
			 *
			 * \~english
			 * @brief Ending of the name of a key carrying the name of the field paired with it
			 * @details The pair «cs1Label=IDSClass» and «cs1=not-suspicious» is linked precisely
			 * by this ending
			 *
			 * \~
			 */
			constexpr string_view LABEL_SUFFIX = "Label";

			/**
			 * \~russian
			 * @brief Запись даты, описанием ArcSight назначенная меткам времени
			 *
			 * @details Описание назначает меткам времени запись вида
			 * «MMM dd yyyy HH:mm:ss.SSS zzz», и разбор её ведётся модулем `chrono_t`
			 * с этой записью по умолчанию. Запись эта настройкой переопределяема:
			 * живые устройства пишут метку и иначе, а описанию следуют не все
			 *
			 * \~english
			 * @brief Notation of a date appointed to the timestamps by the ArcSight specification
			 * @details The specification appoints to the timestamps the notation of the form
			 * «MMM dd yyyy HH:mm:ss.SSS zzz»
			 *
			 * \~
			 */
			constexpr string_view TIMESTAMP_FORMAT = "%b %d %Y %H:%M:%S %Z";

			/**
			 * \~russian
			 * @brief Запись даты, метку времени с долей секунды несущую
			 *
			 * @details Живые устройства пишут метку и с долей секунды, и без неё, а
			 * ОДНА запись обоих видов не покрывает: разбор записи «23:30:15.734 YEKT»
			 * записью без доли секунды съедает «.734» полем зоны, зона остаётся
			 * неразобранной, и метка МОЛЧА смещается на величину зоны. Оттого записей
			 * держится две, а выбор между ними ведётся по самой метке
			 *
			 * @warning Смещение это молчаливо: отказа разбор не выдаёт, и метка
			 * недоверенного журнала обращается в иной момент времени, оставаясь на вид
			 * правдоподобной. Замерено 04.09.2026: «Feb 17 2023 23:30:15.734 YEKT»
			 * записью без доли секунды дало 18 февраля 02:30 вместо 17 февраля 21:30
			 *
			 * \~english
			 * @brief Notation of a date carrying a timestamp with a fraction of a second
			 * @details The living devices write a timestamp both with a fraction of a second and without it,
			 * while ONE notation does not cover both kinds
			 *
			 * \~
			 */
			constexpr string_view TIMESTAMP_FRACTION_FORMAT = "%b %d %Y %H:%M:%S.%s %Z";

			/**
			 * \~russian
			 * @brief Коды ошибок разбора записи CEF
			 *
			 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
			 * вместе с положением в исходном тексте, где отказ произошёл
			 *
			 * @note Перечень держится СХОДНЫМ с перечнями кодеков INI, TOML и YAML в той
			 * части, что у записей общая: коды кодировки, пределов, хранилища и открытия
			 * файла названы теми же именами намеренно, дабы потребитель, разбирающий
			 * отказы по коду, не учил для всякого кодека свой перечень
			 *
			 * @warning Молчание кода означает НЕПРОВЕРЕННОСТЬ пути, а не исправность его
			 *
			 * \~english
			 * @brief Error codes of the parsing of a CEF record
			 * @details The parsing does not throw exceptions: the error code together with the position
			 * in the source text where the refusal has occurred serves as the sign of a refusal
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                  = 0x00, // Ошибок не обнаружено
				INTERNAL              = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF        = 0x02, // Текст оборвался посреди записи
				INVALID_CHARACTER     = 0x03, // Знак недопустим в записи
				INVALID_ENCODING      = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING  = 0x05, // Объявленная кодировка не поддерживается
				MISSING_SIGNATURE     = 0x06, // Запись не содержит слова «CEF:»
				INVALID_VERSION       = 0x07, // Номер редакции записи построен ошибочно
				UNSUPPORTED_VERSION   = 0x08, // Номер редакции записи не поддерживается
				INCOMPLETE_HEADER     = 0x09, // Полей заголовка меньше положенного
				EMPTY_HEADER_FIELD    = 0x0A, // Обязательное поле заголовка пусто
				INVALID_SEVERITY      = 0x0B, // Важность события построена ошибочно либо выходит за предел
				MISSING_SEPARATOR     = 0x0C, // Пара расширения не содержит знака равенства
				EMPTY_KEY             = 0x0D, // Имя ключа расширения пусто
				INVALID_KEY           = 0x0E, // Имя ключа расширения содержит недопустимые знаки
				UNKNOWN_KEY           = 0x0F, // Ключ расширения словарю неизвестен
				NAME_TOO_LONG         = 0x10, // Длина имени превышает допустимую
				FIELD_TOO_LONG        = 0x11, // Длина поля заголовка превышает допустимую
				RECORD_TOO_LONG       = 0x12, // Длина записи превышает допустимую
				INVALID_ESCAPE        = 0x13, // Ошибочное построение отменяющей последовательности
				TYPE_MISMATCH         = 0x14, // Значение не отвечает виду, словарём заданному
				INVALID_ADDRESS       = 0x15, // Значение не является адресом сети
				INVALID_TIMESTAMP     = 0x16, // Значение не является меткой времени
				INVALID_NUMBER        = 0x17, // Значение не является числом
				DANGLING_LABEL        = 0x18, // Имя поля задано меткой, а самого поля в записи нет
				OVERFLOW_LIMIT        = 0x19, // Превышен предел, ЗАДАННЫЙ НАСТРОЙКАМИ разбора
				STORAGE_EXHAUSTED     = 0x1A, // Разбираемый текст не помещается в разрядность хранилища
				UNKNOWN_FIELD         = 0x1B, // Поле с таким именем записью не объявлено
				UNREPRESENTABLE_VALUE = 0x1C, // Значение такого вида запись CEF выразить не может
				NESTED_VALUE          = 0x1D, // Вложенное значение записи CEF неведомо
				CONFLICTING_SETTINGS  = 0x1E, // Настройки записи противоречат толкованию читающего
				FILE_NOT_OPENED       = 0x1F  // Файл записей открыть не удалось
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения записи CEF
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a CEF record
			 * @details The reading issues the events as the text is parsed without holding it in full
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE      = 0x00, // Событие не определено
				SYSLOG    = 0x01, // Приставка syslog, заголовку предшествующая
				HEADER    = 0x02, // Поле заголовка записи
				EXTENSION = 0x03, // Пара расширения «ключ=значение»
				RECORD    = 0x04, // Запись разобрана до конца, следующая начинается заново
				FINISH    = 0x05  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Области записи CEF
			 *
			 * @details Области разделены не для удобства обхода, а по существу: правила
			 * отмены знаков у них РАЗНЫЕ. В заголовке отменяются прямая черта и обратная
			 * косая, а знак равенства отмены не требует; в расширении же отменяется знак
			 * равенства, а черта и косая отмены не требуют
			 *
			 * \~english
			 * @brief Areas of a CEF record
			 * @details The areas are separated not for the convenience of the traversal but in essence: their rules
			 * of the escaping of the characters are DIFFERENT
			 *
			 * \~
			 */
			enum class area_t : uint8_t {
				NONE      = 0x00, // Область не определена
				SYSLOG    = 0x01, // Приставка syslog: отмены знаков не имеет вовсе
				HEADER    = 0x02, // Заголовок: отменяются «\|» и «\\»
				EXTENSION = 0x03  // Расширение: отменяются «\=», «\\», «\n» и «\r»
			};

			/**
			 * \~russian
			 * @brief Поля заголовка записи CEF
			 *
			 * @details Порядок членов отвечает порядку полей в записи и служит их
			 * указателем: поле разбирается по счёту, а не по имени, ибо имён у полей
			 * заголовка запись не несёт вовсе
			 *
			 * \~english
			 * @brief Fields of the header of a CEF record
			 * @details The order of the members corresponds to the order of the fields in a record and serves as their
			 * index: a field is parsed by the count rather than by the name, for the record does not carry
			 * the names of the fields of the header at all
			 *
			 * \~
			 */
			enum class field_t : uint8_t {
				VERSION   = 0x00, // Номер редакции записи, следующий за словом «CEF:»
				VENDOR    = 0x01, // Поставщик устройства
				PRODUCT   = 0x02, // Изделие поставщика
				RELEASE   = 0x03, // Редакция изделия
				SIGNATURE = 0x04, // Опознаватель вида события
				NAME      = 0x05, // Имя события, человеку понятное
				SEVERITY  = 0x06  // Важность события
			};

			/**
			 * \~russian
			 * @brief Строгость сличения ключей расширения со словарём
			 *
			 * @details Словарь расширений задаёт и человеческие имена ключам, и виды их
			 * значений. Строгость сличения выбирается настройкой, а не зашита: журналы
			 * живых устройств несут ключи, словарю неизвестные, и отказ на них
			 * потребителю, собирающему записи с чужого оборудования, мешал бы
			 *
			 * \~english
			 * @brief Strictness of the matching of the keys of an extension against the dictionary
			 * @details The dictionary of the extensions gives both the human-readable names to the keys and the kinds of their
			 * values. The strictness of the matching is chosen by a setting rather than being hardwired
			 *
			 * \~
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Сличения не ведётся вовсе: всякий ключ принимается, значение выдаётся знаками
				LOW    = 0x01, // Сличаются одни имена ключей, вид значения не проверяется
				MEDIUM = 0x02, // Сличаются имена ключей и простые виды значений: числа и логические значения
				STRONG = 0x03  // Сличаются имена ключей и все виды значений, включая адреса сети и метки времени
			};

			/**
			 * \~russian
			 * @brief Виды значений словаря расширений
			 *
			 * @details Вид берётся из словаря по ключу, а не угадывается по виду знаков:
			 * угадывание порождает разночтения - «011» разбирается то восьмеричным, то
			 * десятичным, а «1.10» числом теряет разряд
			 *
			 * \~english
			 * @brief Kinds of the values of the dictionary of the extensions
			 * @details The kind is taken from the dictionary by the key rather than guessed by the appearance of the characters
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE      = 0x00, // Вид не установлен: значение выдаётся знаками
				STRING    = 0x01, // Последовательность знаков
				INTEGER   = 0x02, // Целое число со знаком
				UNSIGNED  = 0x03, // Целое число без знака
				DOUBLE    = 0x04, // Число дробное
				BOOLEAN   = 0x05, // Логическое значение
				MAC       = 0x06, // Адрес устройства сети
				IPV4      = 0x07, // Адрес IPv4
				IPV6      = 0x08, // Адрес IPv6
				ADDRESS   = 0x09, // Адрес сети любого из двух видов
				TIMESTAMP = 0x0A  // Метка времени
			};

			/**
			 * \~russian
			 * @brief Правила обращения с пустым значением расширения
			 *
			 * @details Запись «cs3=» описанием ArcSight не оговорена вовсе - ни разрешена,
			 * ни запрещена, - но в живых журналах обычна и образует пару с меткой
			 * «cs3Label=CVEID». Умолчанием берётся пустая последовательность знаков:
			 * обращение её в логическую истину отдавало бы потребителю утверждение,
			 * какого в записи не было
			 *
			 * \~english
			 * @brief Rules of the treatment of an empty value of an extension
			 * @details The record «cs3=» is not stipulated by the ArcSight specification at all — neither allowed
			 * nor forbidden — but in the living logs it is common
			 *
			 * \~
			 */
			enum class empty_t : uint8_t {
				STRING  = 0x00, // Пустая последовательность знаков
				BOOLEAN = 0x01, // Логическая истина: ключ признаком присутствия
				NUL     = 0x02, // Значение, означающее пустоту
				SKIP    = 0x03  // Пара в дерево не заносится вовсе
			};

			/**
			 * \~russian
			 * @brief Правила обращения с вложенностью при записи дерева в запись CEF
			 *
			 * @details Дерева произвольной глубины запись CEF не несёт, и значение
			 * вложенное выразить ей нечем. Выбор исхода принадлежит не кодеку, а тому,
			 * кто пишет: молчаливое обращение в знаки оставляло бы потребителя с
			 * записью, которая разбирается, но означает иное
			 *
			 * \~english
			 * @brief Rules of the treatment of a nesting at the writing of a tree into a CEF record
			 * @details A CEF record does not carry a tree of an arbitrary depth, and it has nothing to express
			 * a nested value with
			 *
			 * \~
			 */
			enum class nested_t : uint8_t {
				STRICT = 0x00, // Отвечать отказом с кодом NESTED_VALUE
				TEXT   = 0x01, // Обращать в последовательность знаков
				SKIP   = 0x02  // Пропускать значение вовсе
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста записи
			 *
			 * @details Описание ArcSight требует записи в UTF-8 прямо; прочие кодировки
			 * принимаются только навязанными извне, и опознанию по метке порядка байтов
			 * поддаются лишь те из них, что метку эту несут
			 *
			 * \~english
			 * @brief Encodings of the source text of a record
			 * @details The ArcSight specification requires the record to be in UTF-8 directly
			 *
			 * \~
			 */
			enum class encoding_t : uint8_t {
				NONE    = 0x00, // Кодировка не определена
				UTF8    = 0x01, // Кодировка UTF-8
				UTF16LE = 0x02, // Кодировка UTF-16 с обратным порядком байтов
				UTF16BE = 0x03, // Кодировка UTF-16 с прямым порядком байтов
				LATIN1  = 0x04, // Кодировка ISO-8859-1
				ASCII   = 0x05, // Кодировка US-ASCII
				CP1252  = 0x06, // Кодировка Windows-1252
				UTF32LE = 0x07, // Кодировка UTF-32 с обратным порядком байтов
				UTF32BE = 0x08  // Кодировка UTF-32 с прямым порядком байтов
			};

			/**
			 * \~russian
			 * @brief Положение в исходном тексте
			 *
			 * @details Положение несёт и смещение в байтах от начала текста, и номер
			 * строки со столбцом: смещение годится для отсылки к куску текста, а строка
			 * со столбцом - для сообщения человеку
			 *
			 * \~english
			 * @brief Position in the source text
			 * @details The position carries both the offset in bytes from the beginning of the text and the number
			 * of the line with the column
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Метод получения текста сообщения об ошибке разбора
			 *
			 * @details Текст выдаётся на английском языке и предназначен журналу, а не
			 * потребителю: разбирать отказы надлежит по коду, а не по тексту
			 *
			 * @param error код ошибки разбора
			 * @return      текст сообщения об ошибке разбора
			 *
			 * \~english
			 * @brief Method of getting the text of the message about an error of the parsing
			 * @details The text is issued in the English language and is intended for the log rather than for the
			 * consumer: the refusals ought to be discerned by the code rather than by the text
			 * @param error error code of the parsing
			 * @return      text of the message about an error of the parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;

			typedef struct __AWH_SHARED_EXPORT__ Position {
				// Смещение в байтах от начала текста
				uint64_t offset;
				// Номер строки, считая от единицы
				uint64_t line;
				// Номер столбца в байтах, считая от единицы
				uint64_t column;
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
				Position() noexcept : offset(0), line(1), column(1) {}
			} pos_t;
		}
	}
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_CEF_COMMON__
