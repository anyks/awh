/**
 * @file common.hpp
 * @date 2026-08-12
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
 * @brief Заголовочный файл общих определений контейнера TOML — коды ошибок разбора, виды
 *        событий чтения, типы значений, записи строк и чисел, кодировки исходного текста,
 *        пределы разбора, структуры имени ключа, значения, примечания и положения в тексте
 *
 * \~english
 * @brief Header file of the common definitions of the TOML container — the error codes of the parsing, the kinds
 *        of the events of the reading, the types of the values, the notations of the strings and of the numbers, the encodings of the source text,
 *        the limits of the parsing, the structures of the name of a key, of a value, of a comment and of a position in the text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_TOML_COMMON__
#define __AWH_CODEC_TOML_COMMON__

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
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "../../sys/push.hpp"

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
		 * @brief Пространство имён контейнера TOML
		 *
		 * @details Разбор и запись текста настроек в записи TOML версии 1.0.0 - таблиц в
		 * квадратных скобках, наборов таблиц в двойных скобках, пар «ключ = значение» с
		 * выведенным типом значения, перечней, встроенных таблиц и примечаний
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Описание у записи единственное, и наречий здесь нет.** В отличие от INI,
		 * у TOML есть описание (toml.io, версия 1.0.0), и разбор ведётся по нему, а не по
		 * настройкам наречия. Настройки задают пределы разбора, вид собираемого текста и
		 * дозволенные послабления - но не устройство записи
		 *
		 * @li **Тип значения выводится разбором, а не запросом потребителя.** Это
		 * противоположность решению, принятому у INI, и различие это заложено самими
		 * записями: у INI значение всегда последовательность знаков и угадывание типа
		 * породило бы разночтения, а у TOML тип задан описанием - «1.10» там число, «011»
		 * ошибочная запись, а «true» логическое значение, и выдавать их строками значило
		 * бы разбирать запись неверно
		 *
		 * @li **Числа с плавающей точкой хранятся двоичным представлением двойной
		 * точности.** Описание задаёт им именно его, и разрядность сверх неё запись не
		 * переживает. Записанное число выдаётся кратчайшей записью, обратно дающей то же
		 * самое значение: иначе «0.1» уходило бы в файл как «0.10000000000000001»
		 *
		 * @li **Целые числа хранятся знаковым числом в шестьдесят четыре разряда.**
		 * Описание требует принимать весь этот отрезок значений и допускает отвергать
		 * выходящее за него; выход за отрезок отвергается отказом, а не молчаливым
		 * приведением по кругу
		 *
		 * @li **Отметки времени хранятся разобранными по полям, а не строкой.** Описание
		 * отводит им четыре вида - со смещением, местную отметку, местную дату и местное
		 * время, - и различить их можно лишь разбором. Приведение к единому виду отняло
		 * бы у местной отметки её местность
		 *
		 * @li **Разряды доли секунды сверх девяти отбрасываются.** Описание дозволяет
		 * обращение это прямо, и хранение доли ведётся наносекундами: принимать разрядов
		 * больше значило бы требовать целого числа неограниченной длины ради разрядов,
		 * которых не выдаёт ни один источник времени
		 *
		 * @li **Порядок записей сохраняется.** Дерево настроек хранит записи в порядке
		 * появления их в исходном тексте вместе с примечаниями и пустыми строками:
		 * перезапись обязана возвращать файл, узнаваемый его хозяином. Описание порядка
		 * не требует, но файл настроек пишет человек
		 *
		 * @li **Внешние файлы не подключаются.** Указаний подключения запись не имеет
		 * вовсе, и вводить их собственным расширением разбор не станет: ни обращения к
		 * файловой системе, ни к сети разбор не совершает
		 *
		 * @li **Записи INI разбор не принимает.** Внешне записи схожи, но у TOML свои
		 * типы, свои таблицы и свои правила ограждения: принимать записи INI значило бы
		 * принимать неоднозначное. Разбор их выдаёт отказом, а место им в модуле INI
		 *
		 * \~english
		 * @brief TOML container namespace
		 * @details The parsing and the writing of a settings text in the TOML notation of the version 1.0.0 — of the tables in
		 * square brackets, of the arrays of tables in double brackets, of the pairs «key = value» with
		 * an inferred type of the value, of the arrays, of the inline tables and of the comments
		 * @par Deliberate decisions
		 * What is listed below is not a gap of the implementation: these are the outlined boundaries of the
		 * task, and each of the decisions is fixed by a verifying test
		 * @li **The specification of the notation is a single one, and there are no dialects here.** Unlike INI,
		 * TOML has a specification (toml.io, version 1.0.0), and the parsing is conducted by it rather than by
		 * the settings of a dialect. The settings give the limits of the parsing, the form of the assembled text and
		 * the permitted relaxations — but not the arrangement of the notation
		 * @li **The type of a value is inferred by the parsing rather than by a request of the consumer.** This is
		 * the opposite of the decision taken for INI, and that difference is laid down by the notations
		 * themselves: for INI a value is always a sequence of characters and a guessing of the type would
		 * give birth to discrepancies, while for TOML the type is given by the specification — «1.10» there is a number, «011»
		 * is an erroneous record, and «true» is a logical value, and to issue them as strings would mean
		 * to parse the notation incorrectly
		 * @li **The floating-point numbers are stored as a binary representation of a double
		 * precision.** The specification allots exactly it to them, and a width beyond it the notation does not
		 * survive. A written number is issued as the shortest record giving back the same
		 * value: otherwise «0.1» would go into the file as «0.10000000000000001»
		 * @li **The integers are stored as a signed number of sixty-four bits.**
		 * The specification requires accepting that whole range of the values and permits rejecting
		 * what goes beyond it; a going beyond the range is rejected by a refusal rather than by a silent
		 * wrap-around conversion
		 * @li **The timestamps are stored parsed by the fields rather than as a string.** The specification
		 * allots four kinds to them — with an offset, a local timestamp, a local date and a local
		 * time — and they can be distinguished only by a parsing. A bringing to a single form would take
		 * away from a local timestamp its locality
		 * @li **The digits of a fraction of a second beyond nine are discarded.** The specification permits this
		 * treatment directly, while the storing of the fraction is conducted in nanoseconds: to accept more digits
		 * would mean to require an integer of an unlimited length for the sake of the digits
		 * which not a single source of the time issues
		 * @li **The order of the records is preserved.** The settings tree stores the records in the order of
		 * their appearance in the source text together with the comments and the empty lines:
		 * a rewriting is obliged to return a file recognizable by its owner. The specification does not require
		 * the order, but a settings file is written by a human
		 * @li **External files are not included.** The notation has no inclusion directives
		 * at all, and the parsing will not introduce them by an extension of its own: the parsing performs neither calls to
		 * the file system nor to the network
		 * @li **The parsing does not accept the INI notation.** Outwardly the notations are similar, but TOML has its own
		 * types, its own tables and its own rules of the fencing: to accept the INI notation would mean
		 * to accept the ambiguous. The parsing issues them as a refusal, while their place is in the INI module
		 *
		 * \~
		 */
		namespace toml {
			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина логической строки в байтах
			 *
			 * @details Предел считается на строку целиком - вместе со всеми строками
			 * многострочного значения, - иначе многострочная запись давала бы обход
			 * предела
			 *
			 * \~english
			 * @brief Largest admissible length of a logical line in bytes
			 * @details The limit is counted over the line as a whole — together with all the lines of
			 * a multiline value — otherwise a multiline record would give a bypass of
			 * the limit
			 *
			 * \~
			 */
			constexpr uint32_t MAX_LINE = 0x10000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени ключа в байтах
			 *
			 * @details Предел считается на составную часть имени, а не на составное имя
			 * целиком: у составного имени своя мера - глубина вложенности
			 *
			 * \~english
			 * @brief Largest admissible length of the name of a key in bytes
			 * @details The limit is counted over a component part of a name rather than over a compound name
			 * as a whole: a compound name has its own measure — the depth of the nesting
			 *
			 * \~
			 */
			constexpr uint32_t MAX_KEY = 0x1000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая глубина вложенности значений
			 *
			 * @details Меряется вложенностью перечней и встроенных таблиц друг в друга.
			 * Предел этот не украшение: разбор вложенности ведётся стопой состояний, и
			 * текст в несколько килобайт из одних открывающих скобок исчерпывает память
			 * узла, если глубину не ограничить
			 *
			 * \~english
			 * @brief Largest admissible depth of the nesting of the values
			 * @details Measured by the nesting of the arrays and of the inline tables into one another.
			 * This limit is not an adornment: the parsing of the nesting is conducted by a stack of the states, and
			 * a text of several kilobytes made of opening brackets alone exhausts the memory of
			 * a node if the depth is not limited
			 *
			 * \~
			 */
			constexpr uint32_t MAX_DEPTH = 0x40;

			/**
			 * \~russian
			 * @brief Количество пар вместилища, начиная с какого заводится указатель имён
			 *
			 * @details Ниже порога имя разыскивается перебором, и это дешевле всякого
			 * указателя: сличение имён идёт по памяти подряд, а заведение указателя стоит
			 * выделения памяти и подсчёта отпечатка на всякое имя. Выше порога перебор
			 * обращает и сборку вместилища, и чтение его по именам в квадратичные, и
			 * указатель окупается
			 *
			 * @note Указатель заводится **по требованию** - при первом обращении по имени, а
			 * не при разборе. Вместилище, к которому по имени не обращались, не платит ничего
			 *
			 * \~english
			 * @brief Number of the pairs of a container starting from which the index of the names is created
			 * @details Below this threshold a name is searched for by an enumeration, and this is cheaper than any
			 * index: above the threshold an enumeration turns both the assembly of a container and the reading of it
			 * by the names into quadratic ones, and the index pays off
			 * @note The index is created **on demand** — at the first access by a name
			 *
			 * \~
			 */
			constexpr uint32_t INDEX_THRESHOLD = 16;


			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество составных частей имени ключа
			 *
			 * \~english
			 * @brief Largest admissible number of the component parts of the name of a key
			 *
			 * \~
			 */
			constexpr uint32_t MAX_PARTS = 0x20;

			/**
			 * \~russian
			 * @brief Наибольшее количество разрядов доли секунды, принимаемое разбором
			 *
			 * @note Доля секунды хранится наносекундами, и разрядов сверх девяти
			 *       хранилище не вмещает. Описание отбрасывание лишних разрядов дозволяет
			 *
			 * \~english
			 * @brief Largest number of the digits of a fraction of a second accepted by the parsing
			 * @note The fraction of a second is stored in nanoseconds, and the storage does not accommodate
			 *       more than nine digits. The specification permits the discarding of the superfluous digits
			 *
			 * \~
			 */
			constexpr uint32_t MAX_FRACTION = 9;

			/**
			 * \~russian
			 * @brief Обозначение неизвестного смещения от начала текста
			 *
			 * \~english
			 * @brief Designation of an unknown offset from the beginning of the text
			 *
			 * \~
			 */
			constexpr uint64_t NO_OFFSET = static_cast <uint64_t> (-1);

			/**
			 * \~russian
			 * @brief Обозначение неизвестного смещения местного времени
			 *
			 * @note Смещение отсутствует у местной отметки времени и у местного времени:
			 *       значение это отличает их от отметки со смещением, равным нулю, -
			 *       смещение в ноль означает часовой пояс UTC и местной отметкой не
			 *       является
			 *
			 * \~english
			 * @brief Designation of an unknown offset of a local time
			 * @note The offset is absent in a local timestamp and in a local time:
			 *       this value distinguishes them from a timestamp with an offset equal to zero —
			 *       an offset of zero means the UTC time zone and is not a local timestamp
			 *
			 * \~
			 */
			constexpr int32_t NO_TIMEZONE = static_cast <int32_t> (0x7FFFFFFF);

			/**
			 * \~russian
			 * @brief Коды ошибок разбора и записи текста настроек
			 *
			 * @note Перечень этот ОБЩИЙ у кодеков INI, TOML и YAML, и держится он равным
			 * намеренно: потребитель, разбирающий отказы по коду, иначе учил бы для
			 * каждого кодека свой перечень. Оттого часть кодов кодеком TOML не выдаётся
			 * вовсе - условия у наречия TOML для них нет:
			 *
			 * @note - `CONFLICTING_SETTINGS` отвечает противоречию настроек ограждения
			 * значения кавычками, а у наречия TOML ограждение задано описанием, и
			 * настройке, ему противоречащей, взяться неоткуда;
			 *
			 * @note - `UNSUPPORTED_ENCODING` выдавался опознанием кодировки, а с
			 * заведением UTF-32 и однобытовых кодировок опознание отказа более не даёт -
			 * заход остался заставой на случай кодировки, впредь заведённой;
			 *
			 * @note - `UNEXPECTED_EOF` выдаётся заставой разбора записи: всякая запись
			 * обрыв текста отвергает у себя кодом точным - незакрытым перечнем,
			 * оборванной строкой, отсутствующим знаком равенства
			 *
			 * @note - `OVERFLOW_LIMIT` мест выдачи не имеет вовсе: пределы, настройками
			 * заданные, у разбора TOML не заведены, а переполнение хранилища знаков
			 * отвечает своим кодом `STORAGE_EXHAUSTED`
			 *
			 * @warning Молчание кода означает НЕПРОВЕРЕННОСТЬ пути, а не исправность его:
			 * снасть молчащих кодов перебирает места выдачи по трём прогонам, и код,
			 * ни разу не выданный, обещания своего потребителю не даёт
			 *
			 * \~english
			 * @brief Error codes of the parsing and of the writing of a settings text
			 * @note This list is COMMON to the INI, TOML and YAML codecs and is kept equal
			 * deliberately; hence a part of the codes is never issued by the TOML codec —
			 * the TOML dialect has no condition for them
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                  = 0x00, // Ошибок не обнаружено
				INTERNAL              = 0x01, // Внутренняя ошибка разбора
				UNEXPECTED_EOF        = 0x02, // Текст оборвался посреди записи
				INVALID_CHARACTER     = 0x03, // Знак недопустим в тексте настроек
				INVALID_ENCODING      = 0x04, // Последовательность байтов не отвечает объявленной кодировке
				UNSUPPORTED_ENCODING  = 0x05, // Объявленная кодировка не поддерживается
				INVALID_TABLE         = 0x06, // Ошибочное построение объявления таблицы
				UNCLOSED_TABLE        = 0x07, // Объявление таблицы не закрыто квадратной скобкой
				EMPTY_KEY             = 0x08, // Имя ключа пусто
				INVALID_KEY           = 0x09, // Имя ключа содержит недопустимые знаки
				DUPLICATE_KEY         = 0x0A, // Ключ с таким именем уже объявлен
				DUPLICATE_TABLE       = 0x0B, // Таблица с таким именем уже объявлена
				REDEFINE_TABLE        = 0x0C, // Объявление переопределяет уже заданное значение
				EXTEND_INLINE_TABLE   = 0x0D, // Встроенная таблица дополняется после её закрытия
				APPEND_TO_TABLE       = 0x0E, // Набор таблиц дополняет имя, набором таблиц не являющееся
				MISSING_EQUALS        = 0x0F, // Строка не содержит знака равенства
				MISSING_VALUE         = 0x10, // Значение за знаком равенства отсутствует
				INVALID_VALUE         = 0x11, // Значение построено ошибочно
				UNTERMINATED_STRING   = 0x12, // Строковое значение не закрыто кавычкой
				INVALID_ESCAPE        = 0x13, // Ошибочное построение управляющей последовательности
				INVALID_NUMBER        = 0x14, // Ошибочное построение записи числа
				NUMBER_OVERFLOW       = 0x15, // Число выходит за отведённый ему отрезок значений
				INVALID_DATETIME      = 0x16, // Ошибочное построение отметки времени
				UNCLOSED_ARRAY        = 0x17, // Перечень не закрыт квадратной скобкой
				UNCLOSED_INLINE_TABLE = 0x18, // Встроенная таблица не закрыта фигурной скобкой
				UNEXPECTED_CONTENT    = 0x19, // Содержимое за завершённой записью
				LINE_TOO_LONG         = 0x1A, // Длина логической строки превышает допустимую
				KEY_TOO_LONG          = 0x1B, // Длина имени ключа превышает допустимую
				DEPTH_EXCEEDED        = 0x1C, // Превышена допустимая глубина вложенности значений
				PARTS_EXCEEDED        = 0x1D, // Превышено допустимое количество частей имени ключа
				OVERFLOW_LIMIT        = 0x1E, // Превышен предел, ЗАДАННЫЙ НАСТРОЙКАМИ разбора: разрядность хранилища сюда не входит
				CONFLICTING_SETTINGS  = 0x1F, // Настройки записи противоречат толкованию читающего
				UNKNOWN_KEY           = 0x20, // Пара с таким именем деревом не объявлена
				UNKNOWN_TABLE         = 0x21, // Таблица с таким именем деревом не объявлена
				STORAGE_EXHAUSTED     = 0x22  // Разбираемый текст не помещается в разрядность хранилища
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения текста настроек
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его
			 * целиком. Значение составное выдаётся не одним событием, а рядом их: открытие
			 * перечня либо встроенной таблицы, содержимое, закрытие. Так вложенность
			 * достаётся потребителю потоком, а не собранным заранее деревом
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a settings text
			 * @details The reading issues the events as the text is parsed without holding it
			 * in full. A compound value is issued not by a single event but by a series of them: the opening
			 * of an array or of an inline table, the content, the closing. That way the nesting goes
			 * to the consumer as a stream rather than as a tree assembled beforehand
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE         = 0x00, // Событие не определено
				TABLE        = 0x01, // Объявление таблицы
				ARRAY_TABLE  = 0x02, // Объявление очередной таблицы набора таблиц
				KEY          = 0x03, // Имя ключа пары, за которым следует её значение
				VALUE        = 0x04, // Простое значение пары либо очередное значение перечня
				ARRAY_OPEN   = 0x05, // Начало перечня значений
				ARRAY_CLOSE  = 0x06, // Конец перечня значений
				INLINE_OPEN  = 0x07, // Начало встроенной таблицы
				INLINE_CLOSE = 0x08, // Конец встроенной таблицы
				COMMENT      = 0x09, // Примечание
				BLANK        = 0x0A, // Пустая строка
				FINISH       = 0x0B  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Типы значений текста настроек
			 *
			 * @details Отметка времени представлена четырьмя видами намеренно: описание
			 * отводит каждому свой смысл, и различить их можно лишь разбором
			 *
			 * \~english
			 * @brief Types of the values of a settings text
			 * @details A timestamp is represented by four kinds deliberately: the specification
			 * allots its own meaning to each of them, and they can be distinguished only by a parsing
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE            = 0x00, // Тип значения не определён
				STRING          = 0x01, // Последовательность знаков
				INTEGER         = 0x02, // Целое число со знаком
				FLOAT           = 0x03, // Число с плавающей точкой
				BOOLEAN         = 0x04, // Логическое значение
				OFFSET_DATETIME = 0x05, // Отметка времени со смещением часового пояса
				LOCAL_DATETIME  = 0x06, // Отметка времени без смещения часового пояса
				LOCAL_DATE      = 0x07, // Местная дата без времени
				LOCAL_TIME      = 0x08, // Местное время без даты
				ARRAY           = 0x09, // Перечень значений
				TABLE           = 0x0A  // Таблица
			};

			/**
			 * \~russian
			 * @brief Записи строковых значений
			 *
			 * @details Запись у строкового значения четыре, и различаются они не только
			 * видом: основная разбирает управляющие последовательности, а дословная
			 * выдаёт содержимое как записано. Признак этот сохраняется деревом настроек,
			 * чтобы перезапись возвращала запись, выбранную человеком
			 *
			 * \~english
			 * @brief Notations of the string values
			 * @details A string value has four notations, and they differ not only in the
			 * form: the basic one parses the escape sequences, while the literal one
			 * issues the content as it has been written. This attribute is preserved by the settings tree
			 * so that a rewriting returns the notation chosen by the human
			 *
			 * \~
			 */
			enum class string_t : uint8_t {
				BASIC             = 0x00, // Основная строка в двойных кавычках
				LITERAL           = 0x01, // Дословная строка в одинарных кавычках
				MULTILINE_BASIC   = 0x02, // Многострочная основная строка в тройных двойных кавычках
				MULTILINE_LITERAL = 0x03  // Многострочная дословная строка в тройных одинарных кавычках
			};

			/**
			 * \~russian
			 * @brief Записи имени ключа
			 *
			 * \~english
			 * @brief Notations of the name of a key
			 *
			 * \~
			 */
			enum class naming_t : uint8_t {
				BARE    = 0x00, // Имя записано без кавычек
				BASIC   = 0x01, // Имя записано основной строкой в двойных кавычках
				LITERAL = 0x02  // Имя записано дословной строкой в одинарных кавычках
			};

			/**
			 * \~russian
			 * @brief Системы счисления записи целого числа
			 *
			 * @note Признак сохраняется деревом настроек: число, записанное человеком
			 *       шестнадцатеричным, при перезаписи обязано остаться шестнадцатеричным
			 *
			 * \~english
			 * @brief Numeral systems of the notation of an integer
			 * @note The attribute is preserved by the settings tree: a number written by a human
			 *       as a hexadecimal one is obliged to remain a hexadecimal one at a rewriting
			 *
			 * \~
			 */
			enum class radix_t : uint8_t {
				DECIMAL = 0x00, // Десятичная запись числа
				HEX     = 0x01, // Шестнадцатеричная запись числа с приставкой «0x»
				OCTAL   = 0x02, // Восьмеричная запись числа с приставкой «0o»
				BINARY  = 0x03  // Двоичная запись числа с приставкой «0b»
			};

			/**
			 * \~russian
			 * @brief Виды знака конца строки собираемого текста
			 *
			 * \~english
			 * @brief Kinds of the line ending character of the text being assembled
			 *
			 * \~
			 */
			enum class newline_t : uint8_t {
				LF   = 0x00, // Перевод строки, принятый в системах семейства UNIX
				CRLF = 0x01  // Возврат каретки с переводом строки, принятый в MS Windows
			};

			/**
			 * \~russian
			 * @brief Состояния разбора текста настроек
			 *
			 * \~english
			 * @brief States of the parsing of a settings text
			 *
			 * \~
			 */
			enum class state_t : uint8_t {
				READY    = 0x00, // Разбор готов принимать текст
				HUNGRY   = 0x01, // Разбору требуется продолжение текста
				FINISHED = 0x02, // Текст разобран до конца
				FAILED   = 0x03  // Разбор прекращён ошибкой
			};

			/**
			 * \~russian
			 * @brief Кодировки исходного текста настроек
			 *
			 * @details Описание отводит тексту TOML единственную кодировку - UTF-8, - и
			 * умолчанием берётся она. Прочие кодировки принимаются лишь навязанные извне
			 * либо опознанные меткой порядка байтов: файл, полученный от чужой оснастки,
			 * бывает записан иначе, и отвергать его целиком значило бы оставлять
			 * потребителя без всякого способа его прочесть
			 *
			 * @note Кодировки однобайтовые - ISO-8859-1, US-ASCII и Windows-1252 - метки
			 * порядка байтов не имеют вовсе и опознанию не поддаются: принимаются они
			 * только навязанными извне
			 *
			 * \~english
			 * @brief Encodings of the source settings text
			 * @details The specification allots a single encoding to a TOML text — UTF-8 — and
			 * it is taken by default. The other encodings are accepted only when imposed from the outside
			 * or recognized by the byte order mark: a file received from a foreign toolchain
			 * happens to be written otherwise, and to reject it entirely would mean to leave
			 * the consumer without any means of reading it
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
			 * @brief Наибольшее кодовое значение знака Юникода
			 *
			 * \~english
			 * @brief Largest code value of a Unicode character
			 *
			 * \~
			 */
			constexpr uint32_t MAX_CODEPOINT = 0x10FFFF;

			/**
			 * \~russian
			 * @brief Обозначение ошибочного кодового значения знака
			 *
			 * \~english
			 * @brief Designation of an erroneous code value of a character
			 *
			 * \~
			 */
			constexpr uint32_t INVALID_CODEPOINT = static_cast <uint32_t> (-1);

			/**
			 * \~russian
			 * @brief Исход разбора последовательности UTF-8
			 *
			 * @details Оборванная последовательность отделена от битой намеренно: при
			 * чтении по кускам первая означает, что знак придёт следующим куском, а
			 * вторая - что он не придёт никогда
			 *
			 * \~english
			 * @brief Outcome of the parsing of a UTF-8 sequence
			 * @details A cut off sequence is deliberately separated from a broken one: when reading in chunks
			 * the former means that the character will come with the next chunk, while the latter — that it will never come
			 *
			 * \~
			 */
			enum class utf8_t : uint8_t {
				VALID     = 0x01, // Последовательность прочитана целиком и правила соблюдает
				BROKEN    = 0x02, // Последовательность построена ошибочно
				TRUNCATED = 0x03  // Последовательности не хватает байт до конца текста
			};

			/**
			 * \~russian
			 * @brief Положение в исходном тексте настроек
			 *
			 * @details Строка и знак считаются с единицы и меряются знаками приведённого
			 * текста, а смещение - в байтах исходного текста до приведения
			 *
			 * \~english
			 * @brief Position in the source settings text
			 * @details The line and the character are counted from one and are measured in the characters of the converted
			 * text, while the offset — in the bytes of the source text before the conversion
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Location {
				// Смещение от начала текста в байтах
				uint64_t offset;
				// Номер строки, считая с единицы
				uint32_t line;
				// Положение в строке, считая с единицы
				uint32_t column;
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
				Location() noexcept : offset(NO_OFFSET), line(0), column(0) {}
			} location_t;

			/**
			 * \~russian
			 * @brief Дата без времени
			 *
			 * \~english
			 * @brief Date without a time
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Date {
				// Год, считая от нашей эры
				uint16_t year;
				// Месяц, считая с единицы
				uint8_t month;
				// День месяца, считая с единицы
				uint8_t day;
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
				Date() noexcept : year(0), month(0), day(0) {}
			} date_t;

			/**
			 * \~russian
			 * @brief Время без даты
			 *
			 * \~english
			 * @brief Time without a date
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Time {
				// Доля секунды в наносекундах
				uint32_t nanosecond;
				// Час суток
				uint8_t hour;
				// Минута часа
				uint8_t minute;
				/**
				 * \~russian
				 * Секунда минуты
				 *
				 * @note Значение в шестьдесят принимается намеренно: описание дозволяет
				 *       им записывать добавочную секунду координации
				 *
				 * \~english
				 * Second of the minute
				 * @note The value of sixty is accepted deliberately: the specification permits
				 *       writing a leap second with it
				 *
				 * \~
				 */
				uint8_t second;
				/**
				 * \~russian
				 * Количество записанных разрядов доли секунды
				 *
				 * @note Разряды эти значащи при перезаписи: «01:02:03.100» и
				 *       «01:02:03.1» задают одно и то же время, но человек выбрал запись
				 *       сам, и подменять её не следует
				 *
				 * \~english
				 * Number of the written digits of the fraction of a second
				 * @note These digits are significant at a rewriting: «01:02:03.100» and
				 *       «01:02:03.1» give one and the same time, but the human has chosen the notation
				 *       himself, and it should not be substituted
				 *
				 * \~
				 */
				uint8_t digits;
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
				Time() noexcept : nanosecond(0), hour(0), minute(0), second(0), digits(0) {}
			} time_t;

			/**
			 * \~russian
			 * @brief Отметка времени
			 *
			 * @details Вид отметки задаётся типом значения: наличие даты, времени и
			 * смещения часового пояса у каждого вида своё
			 *
			 * \~english
			 * @brief Timestamp
			 * @details The kind of a timestamp is given by the type of the value: the presence of a date, of a time and of an
			 * offset of a time zone is its own for every kind
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Stamp {
				// Дата отметки времени
				date_t date;
				// Время отметки
				time_t time;
				/**
				 * \~russian
				 * Смещение часового пояса в минутах
				 *
				 * @note Значение NO_TIMEZONE означает отсутствие смещения. Ноль им не
				 *       является: он означает часовой пояс UTC, записываемый знаком «Z»
				 *
				 * \~english
				 * Offset of the time zone in minutes
				 * @note The NO_TIMEZONE value means the absence of an offset. Zero is not
				 *       it: it means the UTC time zone written with the «Z» character
				 *
				 * \~
				 */
				int32_t offset;
				/**
				 * \~russian
				 * Признак записи часового пояса UTC знаком «Z»
				 *
				 * @note Запись «Z» и запись «+00:00» задают одно смещение, но человек
				 *       выбрал её сам
				 *
				 * \~english
				 * Flag of the writing of the UTC time zone with the «Z» character
				 * @note The record «Z» and the record «+00:00» give one and the same offset, but the human
				 *       has chosen it himself
				 *
				 * \~
				 */
				bool zulu;
				/**
				 * \~russian
				 * Признак записи нулевого смещения знаком «минус»
				 *
				 * @note Описание отводит записи «-00:00» смысл, от «+00:00» отличный:
				 *       первою обозначено смещение неизвестное, второю - смещение,
				 *       заведомо нулевое. Смешивать их нельзя, и знак этот держится
				 *       отдельно от самого смещения: нуль знака не несёт
				 *
				 * \~english
				 * Flag of the writing of a zero offset with the «minus» character
				 * @note The specification allots to the record «-00:00» a meaning different from «+00:00»:
				 *       by the first an unknown offset is designated, by the second — an offset
				 *       that is knowingly zero. They must not be mixed, and this sign is kept
				 *       separately from the offset itself: zero carries no sign
				 *
				 * \~
				 */
				bool negative;
				/**
				 * \~russian
				 * Признак разделения даты и времени пробелом вместо знака «T»
				 *
				 * @note Описание дозволяет оба разделителя
				 *
				 * \~english
				 * Flag of the separation of the date and the time by a space instead of the «T» character
				 * @note The specification permits both separators
				 *
				 * \~
				 */
				bool spaced;
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
				Stamp() noexcept : offset(NO_TIMEZONE), zulu(false), negative(false), spaced(false) {}
			} stamp_t;

			/**
			 * \~russian
			 * @brief Составная часть имени ключа
			 *
			 * @warning Поле имени живёт ЛИШЬ до следующего обращения к @c next() либо
			 *          @c feed(): ссылается оно на память разбора, и следующее событие её
			 *          переиспользует. Замерено щупом на трёхстах записях с несхожими
			 *          именами - вид отдал чужое содержимое, а не своё
			 *
			 * @note Прежде здесь стояло «живёт не дольше их» - верно по духу, но срока не
			 *       называло, и потребитель не мог узнать, когда именно вид умирает. Срок
			 *       этот один у всех трёх свёрток кодека TOML, и замерен он у каждой порознь
			 *
			 * \~english
			 * @brief Component part of the name of a key
			 * @note The field of the name refers to the memory belonging to the text being parsed
			 *       or to the storage of the names, and it lives no longer than they do
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Part {
				// Имя части, приведённое к окончательному виду
				string_view name;
				// Запись имени части в исходном тексте
				naming_t naming;
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
				Part() noexcept : naming(naming_t::BARE) {}
			} part_t;

			/**
			 * \~russian
			 * @brief Значение пары либо перечня
			 *
			 * @details Выдаётся событием VALUE и несёт разобранное значение вместе с
			 * признаками его записи. Поле, отвечающее типу значения, значаще лишь для
			 * своего типа: прочие поля при этом обнулены
			 *
			 * @note Зовётся тип нагрузкою события, а не значением, оттого что имя `value_t`
			 *       отведено договором рамки владеющему значению из `value.hpp`. Имя
			 *       `content_t` для нагрузки события взято у кодека YAML, где оно стоит в
			 *       том же месте и в той же роли. Строка нагрузки есть взгляд в буфер
			 *       чтения и чтение не переживает - тем нагрузка от владеющего значения и
			 *       отличается
			 *
			 * @warning Виды свёртки живут ЛИШЬ до следующего обращения к @c next() либо
			 * @c feed(): ссылаются они на память разбора, и следующее событие её
			 * переиспользует. Замерено щупом на трёхстах записях с несхожими именами - вид
			 * отдал чужое содержимое. Срок этот ОДИН у всех трёх свёрток кодека TOML, и
			 * замерен он у каждой порознь. Держать надлежит копию, а не вид
			 *
			 * \~english
			 * @brief Value of a pair or of an array
			 * @details Issued by the VALUE event and carries the parsed value together with
			 * the attributes of its notation. The field corresponding to the type of the value is significant only for
			 * its own type: the other fields are thereby zeroed
			 * @note The type is called the payload of the event rather than the value because the name `value_t`
			 * is allotted by the contract of the framework to the owning value from `value.hpp`. The name
			 * `content_t` for the payload of the event is taken from the YAML codec, where it stands in
			 * the same place and in the same role. The string of the payload is a view into the buffer of the
			 * reading and does not outlive the reading — therein the payload differs from the owning value
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Content {
				// Тип значения
				type_t type;
				// Запись строкового значения
				string_t quoting;
				// Система счисления записи целого числа
				radix_t radix;
				// Логическое значение
				bool boolean;
				// Целое число со знаком
				int64_t integer;
				// Число с плавающей точкой
				double real;
				// Отметка времени
				stamp_t stamp;
				// Строковое значение, приведённое к окончательному виду
				string_view text;
				// Положение значения в исходном тексте
				location_t location;
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
				 type(type_t::NONE), quoting(string_t::BASIC), radix(radix_t::DECIMAL),
				 boolean(false), integer(0), real(0.0) {}
			} content_t;

			/**
			 * \~russian
			 * @brief Примечание текста настроек
			 *
			 * @details Примечания в тексте настроек пишет человек, и при перезаписи файла
			 * их надлежит сохранять: файл настроек без примечаний своему хозяину
			 * непонятен
			 *
			 * @warning Виды свёртки живут ЛИШЬ до следующего обращения к @c next() либо
			 * @c feed(): ссылаются они на память разбора, и следующее событие её
			 * переиспользует. Замерено щупом на трёхстах записях с несхожими именами - вид
			 * отдал чужое содержимое. Срок этот ОДИН у всех трёх свёрток кодека TOML, и
			 * замерен он у каждой порознь. Держать надлежит копию, а не вид
			 *
			 * \~english
			 * @brief Comment of a settings text
			 * @details The comments in a settings text are written by a human, and at a rewriting of the file
			 * they ought to be preserved: a settings file without the comments is incomprehensible to its
			 * owner
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Comment {
				/**
				 * \~russian
				 * Содержимое примечания без начального знака и без пробелов перед ним
				 *
				 * @note Пробелы, стоящие за знаком начала примечания, отбрасываются, а
				 *       стоящие в конце его - остаются содержимым. Разница намеренная:
				 *       запись ставит свой пробел между знаком начала и содержимым, и
				 *       ведущие пробелы, сохранённые тут, наращивались бы при каждом
				 *       обороте «чтение - запись», а хвостовые пробелы человек написал
				 *       сам, и отбрасывать их значило бы править файл без спросу
				 *
				 * \~english
				 * Content of the comment without the initial character and without the spaces before it
				 * @note The spaces standing after the character of the beginning of a comment are discarded, while the ones
				 *       standing at its end remain the content. The difference is deliberate:
				 *       the writing puts its own space between the character of the beginning and the content, and
				 *       the leading spaces preserved here would accumulate at every
				 *       «reading — writing» turn, while the trailing spaces have been written by the human
				 *       himself, and to discard them would mean to edit the file without asking
				 *
				 * \~
				 */
				string_view text;
				// Признак того, что примечание дописано к готовой строке
				bool trailing;
				// Положение примечания в исходном тексте
				location_t location;
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
				Comment() noexcept : trailing(false) {}
			} comment_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода ошибки
			 *
			 * @param error код ошибки разбора или записи
			 * @return      описание кода ошибки
			 *
			 * \~english
			 * @brief Method of getting the description of an error code
			 * @param error error code of the parsing or of the writing
			 * @return      description of the error code
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения названия кодировки
			 *
			 * @param encoding кодировка исходного текста
			 * @return         название кодировки
			 *
			 * \~english
			 * @brief Method of getting the name of an encoding
			 * @param encoding encoding of the source text
			 * @return         name of the encoding
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const encoding_t encoding) noexcept;

			/**
			 * \~russian
			 * @brief Метод определения кодировки по её названию
			 *
			 * @details Договор этот один у INI, TOML и YAML: кодек, кодировки принимающий,
			 * обязан уметь и назвать её, и опознать по названию - иначе потребитель, взявший
			 * название из настроек, обратить его в кодировку ничем не может
			 *
			 * @param text название кодировки в любом регистре
			 * @return     определённая кодировка исходного текста
			 *
			 * \~english
			 * @brief Method of determining an encoding by its name
			 * @param text name of the encoding in any case
			 * @return     determined encoding of the source text
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ encoding_t encoding(const string_view text) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения названия типа значения
			 *
			 * @param type тип значения текста настроек
			 * @return     название типа значения
			 *
			 * \~english
			 * @brief Method of getting the name of a type of a value
			 * @param type type of a value of a settings text
			 * @return     name of the type of the value
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения последовательности знаков конца строки
			 *
			 * @param newline вид знака конца строки
			 * @return        последовательность знаков конца строки
			 *
			 * \~english
			 * @brief Method of getting the sequence of the line ending characters
			 * @param newline kind of the line ending character
			 * @return        sequence of the line ending characters
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string_view newline(const newline_t newline) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки знака на допустимость в имени ключа без кавычек
			 *
			 * @warning Приговор здесь один: допустим ли ОДИН знак набора US-ASCII. Пойдёт
			 * ли имя целиком голым, посредник не знает: у него на входе ни длины имени,
			 * ни настроек. Обход имени этим посредником расходится с записью по двум
			 * поводам. Первый: имя ПУСТОЕ обходом даёт истину впустую, а записывается
			 * оно кавычками - `"" = "v"`. Второй, и он обратный: при настройке
			 * `unicode` голым пишется и имя из знаков письменностей мира, тогда как
			 * посредник ложен на каждом его байте. Замер: имя «ключ» при `unicode`
			 * записывается голым, а `bare()` ложен на всех его байтах. Знаки эти
			 * поверяются `named()`, и по коду знака, а не по байту
			 *
			 * @param letter проверяемый знак имени
			 * @return       результат проверки
			 *
			 * \~english
			 * @brief Method of checking a character for admissibility in the name of a key without quotes
			 * @param letter character of the name being checked
			 * @return       result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool bare(const char letter) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки знака Юникода на допустимость в имени ключа без кавычек
			 *
			 * @details Набор знаков задан черновиком следующей версии описания: буквы,
			 * цифры и знаки письменностей мира, но не знаки препинания и не знаки
			 * оформления. Признаются они лишь по настройке, дозволяющей Юникод в имени
			 *
			 * @param code проверяемый знак имени
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking a Unicode character for admissibility in the name of a key without quotes
			 * @details The set of the characters is given by the draft of the next version of the specification: the letters,
			 * the digits and the characters of the writing systems of the world, but not the punctuation marks and not the
			 * formatting characters. They are recognized only by the setting permitting Unicode in a name
			 * @param code character of the name being checked
			 * @return     result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool named(const uint32_t code) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки даты на существование её в календаре
			 *
			 * @details Проверяются и пределы полей, и длина месяца вместе с високосным
			 * годом: «2026-02-31» записью даты не является, и принимать её значило бы
			 * выдавать потребителю день, которого нет
			 *
			 * @note Проверка общая у разбора, записи и правки дерева: расходись они,
			 * правка собирала бы текст, который свой же разбор отвергает
			 *
			 * @param year  проверяемый год
			 * @param month проверяемый месяц
			 * @param day   проверяемый день месяца
			 * @return      результат проверки
			 *
			 * \~english
			 * @brief Method of checking a date for its existence in the calendar
			 * @details Both the limits of the fields and the length of the month together with a leap
			 * year are checked: «2026-02-31» is not a record of a date, and to accept it would mean
			 * to issue the consumer a day that does not exist
			 * @note The check is common to the parsing, to the writing and to the editing of the tree: were they to diverge,
			 * an editing would assemble a text which its own parsing rejects
			 * @param year  year being checked
			 * @param month month being checked
			 * @param day   day of the month being checked
			 * @return      result of the check
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool calendar(const uint16_t year, const uint8_t month, const uint8_t day) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения очередного знака Юникода последовательности UTF-8
			 *
			 * @param text   последовательность знаков, из которой ведётся чтение
			 * @param offset положение начала знака в последовательности
			 * @param code   получаемый знак Юникода
			 * @return       количество байт знака, ноль - знак битый либо оборванный
			 *
			 * \~english
			 * @brief Method of getting the next Unicode character of a UTF-8 sequence
			 * @param text   sequence of characters from which the reading is conducted
			 * @param offset position of the beginning of the character in the sequence
			 * @param code   Unicode character being obtained
			 * @return       number of the bytes of the character, zero — the character is broken or cut off
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t decode(const string_view text, const size_t offset, uint32_t & code) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения длины последовательности UTF-8 по её ведущему байту
			 *
			 * @param leading ведущий байт последовательности знака
			 * @return        количество байт последовательности, ноль - байт ведущим не является
			 *
			 * \~english
			 * @brief Method of getting the length of a UTF-8 sequence by its leading byte
			 * @param leading leading byte of the character sequence
			 * @return        number of the bytes of the sequence, zero — the byte is not a leading one
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t sequence(const uint8_t leading) noexcept;
			/**
			 * \~russian
			 * @brief Метод разбора очередной последовательности UTF-8
			 *
			 * @details Свод правил разбора последовательности собран здесь единственным
			 * телом: чтение целым текстом и чтение по кускам расходятся лишь тем, как
			 * они докладывают исход, но не тем, какую последовательность признают
			 *
			 * @param text   последовательность знаков, из которой ведётся чтение
			 * @param offset положение начала знака в последовательности
			 * @param code   получаемый знак Юникода, при неудачном разборе не выставляется
			 * @param length количество байт, разбором пройденных, при нехватке байт нулевое
			 * @return       исход разбора последовательности
			 *
			 * \~english
			 * @brief Method of the parsing of the next UTF-8 sequence
			 * @details The set of the rules of the parsing of a sequence is gathered here as a single
			 * body: the reading by a whole text and the reading in chunks differ only in how
			 * they report the outcome, but not in which sequence they accept
			 * @param text   sequence of characters from which the reading is conducted
			 * @param offset position of the beginning of the character in the sequence
			 * @param code   Unicode character being obtained, is not set when the parsing fails
			 * @param length number of the bytes passed by the parsing, is zero when the bytes are not enough
			 * @return       outcome of the parsing of the sequence
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ utf8_t inspect(const string_view text, const size_t offset, uint32_t & code, size_t & length) noexcept;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/pop.hpp"

#endif // __AWH_CODEC_TOML_COMMON__
