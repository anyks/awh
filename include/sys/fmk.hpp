/**
 * @file fmk.hpp
 * @date 2026-09-13
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
 * @brief Заголовочный файл ядра фреймворка — пространство имён awh::fmk с базовыми
 *        средствами библиотеки: проверками и преобразованием текста, перекодировкой,
 *        форматированием, разбором чисел и работой с адресами
 *
 * @details Модуль выполнен пространством имён, а не классом: его средства нужны всякому
 *          модулю библиотеки, состояния, зависящего от места вызова, он не несёт, и
 *          передавать его указателем в конструкторы больше не требуется.
 *
 * \~english
 * @brief Header file of the core of the framework — the awh::fmk namespace with the base
 *        means of the library: the checks and the transformation of a text, the transcoding,
 *        the formatting, the parsing of the numbers and the work with the addresses
 *
 * @details The module is made a namespace and not a class: its means are needed by every
 *          module of the library, it carries no state depending on the place of the call, and
 *          handing it by a pointer into the constructors is no longer required.
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
#include <vector>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <cstdarg>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "macro/global.hpp"
#include "../encoding/charset/charset.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возврат в конце файла в конце файла)
 */
#include "macro/suppress.hpp"

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
	 * @brief Пространство имён ядра фреймворка
	 *
	 * @details Средства модуля глобальны на приложение: настройка локали и набор доменных
	 *          зон заводятся единственными на процесс, прочие средства состояния не несут.
	 *
	 * \~english
	 * @brief Namespace of the core of the framework
	 *
	 * @details The means of the module are global to the application: the setting of the locale and the set of the domain
	 *          zones are started single per process, the other means carry no state.
	 *
	 * \~
	 */
	namespace fmk {
		/**
		 * \~russian
		 * @brief Флаги трансформации строк
		 *
		 * @details Приведение регистра выполняется по таблице ASCII для узких строк
		 *          и по установленной локали для строк широких символов.
		 *
		 * @note Флаг «SMART_CASE» переводит в верхний регистр первую букву каждого
		 *       слова, а прочие буквы — в нижний. Границей слова служат пробельный
		 *       символ, знак переноса и знак подчёркивания
		 *
		 * \~english
		 * @brief Flags of the transformation of the strings
		 *
		 * @details The bringing of the case is performed by the ASCII table for the narrow strings
		 *          and by the set locale for the strings of wide characters.
		 *
		 * @note The «SMART_CASE» flag converts into the upper case the first letter of every
		 *       word, and the other letters — into the lower one. A boundary of a word is a whitespace
		 *       character, a hyphen and an underscore
		 *
		 * \~
		 */
		enum class transform_t : uint8_t {
			NONE       = 0x00, // Флаг не установлен
			TRIM       = 0x01, // Флаг удаления пробелов
			UPPER_CASE = 0x02, // Флаг перевода в верхний регистр
			LOWER_CASE = 0x03, // Флаг перевода в нижний регистр
			SMART_CASE = 0x04  // Флаг умного перевода начальных символов в верхний режим
		};
		/**
		 * \~russian
		 * @brief Тип штампа времени
		 *
		 * @details Значения задают единицу измерения, в которой выводится штамп
		 *          времени, а для текстового вывода — разряд, до которого штамп
		 *          записывается.
		 *
		 * \~english
		 * @brief Type of the timestamp
		 *
		 * @details The values set the unit of the measurement the timestamp is yielded in,
		 *          and for the text output — the digit the timestamp is
		 *          written up to.
		 *
		 * \~
		 */
		enum class chrono_t : uint8_t {
			NONE         = 0x00, // Не установлено
			YEAR         = 0x01, // Год
			MONTH        = 0x02, // Месяц
			WEEK         = 0x03, // Неделя
			DAY          = 0x04, // День
			HOUR         = 0x05, // Час
			MINUTES      = 0x06, // Минуты
			SECONDS      = 0x07, // Секунды
			MILLISECONDS = 0x08, // Миллисекунды
			MICROSECONDS = 0x09, // Микросекунды
			NANOSECONDS  = 0x0A  // Наносекунды
		};
		/**
		 * \~russian
		 * @brief Флаги проверки текстовых данных
		 *
		 * @details Проверки выполняются по таблице ASCII и от установленной локали
		 *          не зависят. Проверка на адрес выполняется разборщиком адресов,
		 *          проверка на запись UTF-8 — модулем Юникода.
		 *
		 * @note Флаг «PSEUDO_NUMBER» принимает записи, числом не являющиеся, но
		 *       начинающиеся либо завершающиеся цифрой: такие записи встречаются
		 *       в журналах и в заголовках протоколов
		 *
		 * \~english
		 * @brief Flags of the check of the text data
		 *
		 * @details The checks are performed by the ASCII table and do not depend on the set locale.
		 *          The check for an address is performed by the parser of the addresses,
		 *          the check for a UTF-8 record — by the Unicode module.
		 *
		 * @note The «PSEUDO_NUMBER» flag accepts the records that are not numbers, but
		 *       begin or end with a digit: such records occur
		 *       in the logs and in the headers of the protocols
		 *
		 * \~
		 */
		enum class check_t : uint8_t {
			NONE            = 0x00, // Флаг не установлен
			URL             = 0x01, // Флаг проверки на URL-адрес
			UTF8            = 0x02, // Флаг проверки на UTF-8
			PRINT           = 0x03, // Флаг проверки на печатаемый символ
			UPPER           = 0x04, // Флаг проверки на верхний регистр
			LOWER           = 0x05, // Флаг проверки на нижний регистр
			SPACE           = 0x06, // Флаг проверки на пробел
			LATIAN          = 0x07, // Флаг проверки на латинские символы
			NUMBER          = 0x08, // Флаг проверки на число
			DECIMAL         = 0x09, // Флаг проверки на число с плавающей точкой
			PSEUDO_NUMBER   = 0x0A, // Флаг проверки на псевдо-число
			PRESENCE_LATIAN = 0x0B  // Флаг проверки наличия латинских символов в строке
		};

		/**
		 * \~russian
		 * @brief Порядок обращения с символами, кодировке не представимыми
		 *
		 * \~english
		 * @brief Order of dealing with the characters not representable in an encoding
		 *
		 * \~
		 */
		using replace_t = charset::replace_t;
		/**
		 * \~russian
		 * @brief Кодировка текста
		 *
		 * @details Обозначение заведено затем, чтобы Framework задавал кодировки
		 *          собственным именем, не оговаривая их размещения. Набор кодировок
		 *          задан модулем перекодировки и стандартом кодировок консорциума
		 *          WHATWG, которого модуль держится.
		 *
		 * \~english
		 * @brief Encoding of a text
		 *
		 * @details The designation is started so that Framework would set the encodings by
		 *          its own name, without stipulating their placement. The set of the encodings
		 *          is set by the transcoding module and by the encoding standard of the
		 *          WHATWG consortium, which the module holds to.
		 *
		 * \~
		 */
		using codepage_t = charset::encoding_t;
		/**
		 * \~russian
		 * @brief Шаблон функции формирования форматированной строки
		 *
		 * @tparam C    тип символа строки
		 * @tparam Args типы аргументов подстановки
		 *
		 * \~english
		 * @brief Template of the function of building a formatted string
		 *
		 * @tparam C    type of the character of the string
		 * @tparam Args types of the arguments of the substitution
		 *
		 * \~
		 */
		template <typename C, typename... Args>
		/**
		 * \~russian
		 * @brief Условие пригодности перекрытия по записи «printf»
		 *
		 * @details Перекрытие исключается из разбора, если среди доводов встречается
		 *          список записей подстановки — этот вызов обслуживает подстановка по «$N».
		 *
		 * \~english
		 * @brief Condition of the fitness of the overload by the «printf» record
		 *
		 * @details The overload is excluded from the resolution if among the arguments there occurs
		 *          a list of the records of the substitution — this call is served by the substitution by «$N».
		 *
		 * \~
		 */
		using printable_t = enable_if_t <!(false || ... || is_same <decay_t <Args>, vector <basic_string <C>>>::value), basic_string <C>>;

		/**
		 * \~russian
		 * @brief Пространство имён служебных средств модуля
		 *
		 * @details Средства заведены для нужд самого модуля и частью его договора не являются.
		 *
		 * \~english
		 * @brief Namespace of the service means of the module
		 *
		 * @details The means are started for the needs of the module itself and are not a part of its contract.
		 *
		 * \~
		 */
		namespace detail {
			/**
			 * \~russian
			 * @brief Функция извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 * \~english
			 * @brief Function of extracting the list of the escaping characters by default
			 *
			 * @return list of the escaping characters by default
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const vector <string> & escapingText() noexcept;
			/**
			 * \~russian
			 * @brief Функция извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 * \~english
			 * @brief Function of extracting the list of the escaping characters by default
			 *
			 * @return list of the escaping characters by default
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const vector <wstring> & escapingWide() noexcept;
			/**
			 * \~russian
			 * @brief Функция формирования форматированной строки по записи «printf»
			 *
			 * @details Здесь выполняется само построение строки. Наружу метод не
			 *          выведен: обращение к нему идёт через перекрытие «format»,
			 *          отчего перечень доводов с переменным числом остаётся вне
			 *          разбора перекрытий и не сталкивается с подстановкой по «$N».
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 * \~english
			 * @brief Function of building a formatted string by the «printf» record
			 *
			 * @details Here the very building of the string is performed. Outwards the method is not
			 *          exposed: the address to it goes through the «format» overload,
			 *          and therefore the list of the arguments of a variable number remains outside
			 *          the resolution of the overloads and does not collide with the substitution by «$N».
			 *
			 * @param format format of the string of the output
			 * @param args   passed arguments
			 * @return       the built string
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ string formatted(const char * format, ...) noexcept;
			/**
			 * \~russian
			 * @brief Функция формирования форматированной строки по записи «printf»
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 * \~english
			 * @brief Function of building a formatted string by the «printf» record
			 *
			 * @param format format of the string of the output
			 * @param args   passed arguments
			 * @return       the built string
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ wstring formatted(const wchar_t * format, ...) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения штампа времени в указанных единицах измерения
			 *
			 * @param buffer буфер бинарных данных для установки штампа времени
			 * @param size   размер бинарных данных штампа времени
			 * @param type   тип формируемого штампа времени
			 * @param text   флаг извлечения данных в текстовом виде
			 *
			 * \~english
			 * @brief Function of getting a timestamp in the specified units of the measurement
			 *
			 * @param buffer buffer of the binary data to set the timestamp into
			 * @param size   size of the binary data of the timestamp
			 * @param type   type of the built timestamp
			 * @param text   flag of the extraction of the data in the text form
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ void timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) noexcept;
		};

		/**
		 * \~russian
		 * @brief Функция заведения модуля на весь процесс
		 *
		 * @details Заводит захват выдачи памяти, поднимает предел дампа ядра и
		 *          устанавливает локализацию приложения. Прежде это выполнял конструктор
		 *          класса Framework; теперь заведение происходит само при первом обращении
		 *          к модулю, а эта функция позволяет назначить мгновение заведения явно.
		 *
		 * @note Захват выдачи памяти требует, чтобы потоков в приложении ещё не было,
		 *       поэтому звать функцию следует в начале точки входа. Повторные вызовы
		 *       безвредны: заведение выполняется один раз на процесс.
		 *
		 * \~english
		 * @brief Function of the starting of the module for the whole process
		 *
		 * @details Starts the seizing of the memory allocation, raises the limit of the core dump and
		 *          sets the locale of the application. Previously this was performed by the constructor
		 *          of the Framework class; now the starting happens by itself on the first appeal
		 *          to the module, and this function allows to appoint the moment of the starting explicitly.
		 *
		 * @note The seizing of the memory allocation requires that there be no threads in the application yet,
		 *       therefore the function should be called at the beginning of the entry point. Repeated calls
		 *       are harmless: the starting is performed once per process.
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void initialize() noexcept;
		/**
		 * \~russian
		 * @brief Функция генерации уникального идентификатора
		 * 
		 * @return уникальный идентификатор
		 *
		 * \~english
		 * @brief Function of generating a unique identifier
		 *
		 * @return unique identifier
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint32_t identifier() noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции получения штампа времени в указанных единицах измерения
		 *
		 * @tparam T тип данных в котором извлекаются данные
		 *
		 * \~english
		 * @brief Template of the function of getting a timestamp in the specified units of the measurement
		 *
		 * @tparam T type of the data the data is extracted in
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция получения штампа времени в указанных единицах измерения
		 *
		 * @details Штамп снимается с системных часов. Числовой тип получает время,
		 *          прошедшее с начала эпохи в заданных единицах измерения, тогда как
		 *          строковый тип получает запись даты и времени, доведённую до
		 *          заданного разряда.
		 *
		 * @note Целый тип, в который штамп не помещается, получает его старшие
		 *       разряды, тогда как дробный тип получает штамп целиком, но с
		 *       точностью своей мантиссы: у «float» её 24 разряда, чего не хватает
		 *       для точной записи счётчика наносекунд. Требуется точность —
		 *       снимайте штамп типом «uint64_t»
		 *
		 * @warning Дробные типы обрабатываются отдельной ветвью шаблона, а не общим
		 *          разбором по размеру буфера: тот писан для целых видов и записал бы
		 *          в дробный тип двоичное представление целого вместо самого числа.
		 *          Закреплено тестом «FmkTimestampRealMatchesIntegerTest»
		 *
		 * @param type тип формируемого штампа времени
		 * @return     сгенерированный штамп времени
		 *
		 * @code{.cpp}
		 * fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS);
		 * fmk::timestamp <string> (awh::fmk::chrono_t::SECONDS);
		 * @endcode
		 *
		 * \~english
		 * @brief Function of getting a timestamp in the specified units of the measurement
		 *
		 * @details The timestamp is taken from the system clock. A numeric type receives the time
		 *          elapsed since the beginning of the epoch in the given units of the measurement, while
		 *          a string type receives a record of the date and the time brought up to
		 *          the given digit.
		 *
		 * @note An integer type the timestamp does not fit into receives its higher
		 *       digits, while a fractional type receives the timestamp entirely, but with
		 *       the precision of its mantissa: «float» has 24 digits of it, which is not enough
		 *       for an exact record of a counter of the nanoseconds. If the precision is required —
		 *       take the timestamp by the «uint64_t» type
		 *
		 * @warning The fractional types are handled by a separate branch of the template, and not by the common
		 *          resolution by the size of the buffer: that one is written for the integer kinds and would write
		 *          into a fractional type the binary representation of an integer instead of the number itself.
		 *          Fixed by the «FmkTimestampRealMatchesIntegerTest» test
		 *
		 * @param type type of the built timestamp
		 * @return     the generated timestamp
		 *
		 * @code{.cpp}
		 * fmk::timestamp <uint64_t> (awh::fmk::chrono_t::MILLISECONDS);
		 * fmk::timestamp <string> (awh::fmk::chrono_t::SECONDS);
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ T timestamp(const chrono_t type) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки UTF-8 в строку широких символов
		 *
		 * @details Узкая строка разбирается как запись в кодировке UTF-8 и выводится
		 *          строкой широких символов. Обратное действие выполняется одноимённым
		 *          методом, принимающим строку широких символов.
		 *
		 * @note Разрядность широкого символа задаётся операционной системой: на
		 *       MS Windows он двухбайтовый, и символы за пределами основной плоскости
		 *       записываются суррогатной парой, тогда как на прочих системах он
		 *       четырёхбайтовый и хранит кодовое значение целиком
		 *
		 * @note Текст, записью UTF-8 не являющийся, выводится пустой строкой МОЛЧА:
		 *       разбор записи UTF-8 отказа не оглашает вовсе, оттого пустой вывод
		 *       неотличим от пустого ввода. Требуется различать - проверяйте текст
		 *       ходом `is` с признаком `UTF8` до перевода
		 *
		 * @param str строка UTF-8 для конвертирования
		 * @return    строка широких символов
		 *
		 * @code{.cpp}
		 * const wstring wide = fmk::convert(string{"Привет"});
		 * const string text = fmk::convert(wide);
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting a UTF-8 string into a wide-character string
		 *
		 * @details A narrow string is parsed as a record in the UTF-8 encoding and is yielded
		 *          as a string of wide characters. The reverse action is performed by the method of the same name
		 *          taking a string of wide characters.
		 *
		 * @note The width of a wide character is set by the operating system: on
		 *       MS Windows it is a two-byte one, and the characters beyond the basic plane
		 *       are written by a surrogate pair, while on the other systems it is
		 *       a four-byte one and holds the code value entirely
		 *
		 * @note A text that is not a UTF-8 record is yielded as an empty string SILENTLY:
		 *       the parsing of a UTF-8 record does not announce a refusal at all, therefore an empty
		 *       yielded value is indistinguishable from an empty input. If the distinction is required,
		 *       check the text by the `is` function with the `UTF8` flag before the conversion
		 *
		 * @param str UTF-8 string to convert
		 * @return    wide-character string
		 *
		 * @code{.cpp}
		 * const wstring wide = fmk::convert(string{"Привет"});
		 * const string text = fmk::convert(wide);
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ wstring convert(string_view str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки широких символов в строку UTF-8
		 *
		 * @param str строка широких символов для конвертирования
		 * @return    строка в кодировке UTF-8
		 *
		 * \~english
		 * @brief Function of converting a wide-character string into a UTF-8 string
		 *
		 * @param str wide-character string to convert
		 * @return    string in the UTF-8 encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string convert(wstring_view str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки UTF-8 в строку широких символов
		 *
		 * @param str строка UTF-8 для конвертирования
		 * @return    строка широких символов
		 *
		 * \~english
		 * @brief Function of converting a UTF-8 string into a wide-character string
		 *
		 * @param str UTF-8 string to convert
		 * @return    wide-character string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring convert(const char * str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки широких символов в строку UTF-8
		 *
		 * @param str строка широких символов для конвертирования
		 * @return    строка в кодировке UTF-8
		 *
		 * \~english
		 * @brief Function of converting a wide-character string into a UTF-8 string
		 *
		 * @param str wide-character string to convert
		 * @return    string in the UTF-8 encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string convert(const wchar_t * str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки UTF-8 в строку широких символов
		 *
		 * @param str строка UTF-8 для конвертирования
		 * @return    строка широких символов
		 *
		 * \~english
		 * @brief Function of converting a UTF-8 string into a wide-character string
		 *
		 * @param str UTF-8 string to convert
		 * @return    wide-character string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring convert(const string & str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования строки широких символов в строку UTF-8
		 *
		 * @param str строка широких символов для конвертирования
		 * @return    строка в кодировке UTF-8
		 *
		 * \~english
		 * @brief Function of converting a wide-character string into a UTF-8 string
		 *
		 * @param str wide-character string to convert
		 * @return    string in the UTF-8 encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string convert(const wstring & str) noexcept;
		/**
		 * \~russian
		 * @brief функции определения точного размера, сколько занимает число байт
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief functions of determining the exact size of how many bytes a number occupies
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция определения точного размера, сколько занимает число байт
		 *
		 * @details Выводится количество байт, которым число записывается без потери
		 *          значения, а не размер его типа. Метод служит сжатию записи чисел
		 *          в двоичных протоколах.
		 *
		 * @param num число для проверки
		 * @return    фактический размер занимаемым числом байт
		 *
		 * @code{.cpp}
		 * fmk::size <uint64_t> (255);    // 1
		 * fmk::size <uint64_t> (256);    // 2
		 * fmk::size <uint64_t> (0);      // 0
		 * @endcode
		 *
		 * \~english
		 * @brief Function of determining the exact size of how many bytes a number occupies
		 *
		 * @details What is yielded is the number of the bytes a number is written by without a loss of
		 *          the value, and not the size of its type. The method serves the compression of the record of the numbers
		 *          in the binary protocols.
		 *
		 * @param num number to check
		 * @return    actual size of the bytes occupied by the number
		 *
		 * @code{.cpp}
		 * fmk::size <uint64_t> (255);    // 1
		 * fmk::size <uint64_t> (256);    // 2
		 * fmk::size <uint64_t> (0);      // 0
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t size(const T num) noexcept;
		/**
		 * \~russian
		 * @brief Функция определения точного размера, сколько занимают данные (в байтах) в буфере
		 *
		 * @details Выводится размер буфера за вычетом нулевых байтов, лежащих в его
		 *          конце. Буфер, заполненный нулями целиком, даёт нулевой размер.
		 *
		 * @warning Метод пригоден лишь для буферов, записанных ОБРАТНЫМ порядком байт
		 *          (little-endian), где старший разряд числа лежит в конце буфера:
		 *          именно оттуда и отсекаются нулевые байты. Прямой порядок байт
		 *          (big-endian) не поддержан - там старший разряд лежит в начале, и
		 *          отсечение съело бы значащие байты числа
		 *
		 * @param value значение бинарного буфера для проверки
		 * @param size  общий размер бинарного буфера
		 * @return      фактический размер буфера занимаемый данными
		 *
		 * \~english
		 * @brief Function of determining the exact size of how much data (in bytes) occupies in a buffer
		 *
		 * @details What is yielded is the size of the buffer minus the zero bytes lying at its
		 *          end. A buffer filled with zeroes entirely gives a zero size.
		 *
		 * @warning The method is suitable only for the buffers written in the REVERSE order of the
		 *          bytes (little-endian), where the higher digit of a number lies at the end of the buffer:
		 *          it is from there that the zero bytes are cut off. The direct order of the bytes
		 *          (big-endian) is not supported - there the higher digit lies at the beginning, and
		 *          the cutting off would eat the significant bytes of the number
		 *
		 * @param value value of the binary buffer to check
		 * @param size  total size of the binary buffer
		 * @return      actual size of the buffer occupied by the data
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t size(const void * value, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода римских цифр в арабские
		 *
		 * @details Запись разбирается без учёта регистра.
		 *
		 * @note Запись, римским числом не являющаяся, выводится нулевым значением
		 *
		 * @see arabic2rome
		 *
		 * @param word римское число
		 * @return     арабское число
		 *
		 * @code{.cpp}
		 * fmk::rome2arabic("XIV");   // 14
		 * fmk::rome2arabic("mcmxc"); // 1990
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting the Roman numerals into the Arabic ones
		 *
		 * @details The record is parsed without the case taken into account.
		 *
		 * @note A record that is not a Roman number is yielded as a zero value
		 *
		 * @see arabic2rome
		 *
		 * @param word Roman number
		 * @return     Arabic number
		 *
		 * @code{.cpp}
		 * fmk::rome2arabic("XIV");   // 14
		 * fmk::rome2arabic("mcmxc"); // 1990
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ uint16_t rome2arabic(string_view word) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода римских цифр в арабские
		 *
		 * @param word римское число
		 * @return     арабское число
		 *
		 * \~english
		 * @brief Function of converting the Roman numerals into the Arabic ones
		 *
		 * @param word Roman number
		 * @return     Arabic number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ uint16_t rome2arabic(wstring_view word) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода арабских чисел в римские
		 *
		 * @details Запись выводится прописными буквами.
		 *
		 * @note Число вне пределов от 1 до 4999 выводится пустой строкой: римская
		 *       запись больших чисел требует надчёркивания, записи не имеющего
		 *
		 * @see rome2arabic
		 *
		 * @param number арабское число от 1 до 4999
		 * @return       римское число
		 *
		 * @code{.cpp}
		 * fmk::arabic2rome(14);    // «XIV»
		 * fmk::arabic2rome(1990);  // «MCMXC»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting the Arabic numbers into the Roman ones
		 *
		 * @details The record is yielded in the capital letters.
		 *
		 * @note A number outside the limits from 1 to 4999 is yielded as an empty string: the Roman
		 *       record of the large numbers requires an overline, which the record does not have
		 *
		 * @see rome2arabic
		 *
		 * @param number Arabic number from 1 to 4999
		 * @return       Roman number
		 *
		 * @code{.cpp}
		 * fmk::arabic2rome(14);    // «XIV»
		 * fmk::arabic2rome(1990);  // «MCMXC»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ wstring arabic2rome(const uint32_t number) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода арабских чисел в римские
		 *
		 * @param word арабское число от 1 до 4999
		 * @return     римское число
		 *
		 * \~english
		 * @brief Function of converting the Arabic numbers into the Roman ones
		 *
		 * @param word Arabic number from 1 to 4999
		 * @return     Roman number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string arabic2rome(string_view word) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода арабских чисел в римские
		 *
		 * @param word арабское число от 1 до 4999
		 * @return     римское число
		 *
		 * \~english
		 * @brief Function of converting the Arabic numbers into the Roman ones
		 *
		 * @param word Arabic number from 1 to 4999
		 * @return     Roman number
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring arabic2rome(wstring_view word) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции инверсии бита в указанной позиции
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the inversion of a bit at the specified position
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция инверсии бита в указанной позиции
		 *
		 * @param pos позиция для инверсии
		 * @param num число в бинарном виде для инверсии бита
		 * @return    итоговое значение числа после инверсии
		 *
		 * \~english
		 * @brief Function of the inversion of a bit at the specified position
		 *
		 * @param pos position for the inversion
		 * @param num number in the binary form to invert the bit of
		 * @return    resulting value of the number after the inversion
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T flipBit(const T pos, const T num) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции сброса бита в указанной позиции
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the reset of a bit at the specified position
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция сброса бита в указанной позиции
		 *
		 * @param pos позиция для сброса
		 * @param num число в бинарном виде для сброса бита
		 * @return    итоговое значение числа после сброса бита
		 *
		 * \~english
		 * @brief Function of the reset of a bit at the specified position
		 *
		 * @param pos position for the reset
		 * @param num number in the binary form to reset the bit of
		 * @return    resulting value of the number after the reset of the bit
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T resetBit(const T pos, const T num) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции устанвки бита в указанную позицию
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the setting of a bit at the specified position
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция устанвки бита в указанную позицию
		 *
		 * @param pos позиция для установки бита
		 * @param num начальное значение бита
		 * @return    итоговое значение числа после установки бита
		 *
		 * \~english
		 * @brief Function of the setting of a bit at the specified position
		 *
		 * @param pos position for the setting of the bit
		 * @param num initial value of the bit
		 * @return    resulting value of the number after the setting of the bit
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T setBit(const T pos, const T num = 0) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции проверки установлен ли бит в указанной позиции
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of checking whether a bit is set at the specified position
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция проверки установлен ли бит в указанной позиции
		 *
		 * @details Разряды отсчитываются от младшего, начиная с нуля.
		 *
		 * @warning Позиция, разрядность типа превышающая, поведения не задаёт:
		 *          проверять её следует вызывающей стороне
		 *
		 * @param pos позиция для проверки
		 * @param num число в бинарном виде для проверки бита
		 * @return    результат проверки
		 *
		 * @code{.cpp}
		 * fmk::isBit <uint8_t> (0, 0b00000101);    // истина
		 * fmk::setBit <uint8_t> (3, 0b00000001);   // 0b00001001
		 * fmk::resetBit <uint8_t> (0, 0b00000101); // 0b00000100
		 * fmk::flipBit <uint8_t> (1, 0b00000101);  // 0b00000111
		 * @endcode
		 *
		 * \~english
		 * @brief Function of checking whether a bit is set at the specified position
		 *
		 * @details The digits are counted from the lowest one, starting from zero.
		 *
		 * @warning A position exceeding the width of the type does not set the behaviour:
		 *          it should be checked by the calling side
		 *
		 * @param pos position to check
		 * @param num number in the binary form to check the bit of
		 * @return    result of the check
		 *
		 * @code{.cpp}
		 * fmk::isBit <uint8_t> (0, 0b00000101);    // true
		 * fmk::setBit <uint8_t> (3, 0b00000001);   // 0b00001001
		 * fmk::resetBit <uint8_t> (0, 0b00000101); // 0b00000100
		 * fmk::flipBit <uint8_t> (1, 0b00000101);  // 0b00000111
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ bool isBit(const T pos, const T num) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения иконки
		 *
		 * @details Выводится случайно выбранный знак из набора: один набор отведён
		 *          началу работы, другой — её завершению. Метод служит выводу в
		 *          консоль и содержательной нагрузки не несёт.
		 *
		 * @param end флаг завершения работы
		 * @return    иконка напутствия работы
		 *
		 * \~english
		 * @brief Function of getting an icon
		 *
		 * @details What is yielded is a randomly chosen sign from a set: one set is given over to
		 *          the beginning of the work, another one — to its completion. The method serves the output into
		 *          the console and carries no meaningful load.
		 *
		 * @param end flag of the completion of the work
		 * @return    icon of the parting word of the work
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string icon(const bool end = false) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения количества байт в секунду из строки
		 *
		 * @details Пропускная способность сети задаётся в битах, а выводится в байтах:
		 *          разобранное значение делится на восемь. Приставки при этом задают
		 *          степени числа 1000, а не 1024, как принято для пропускной
		 *          способности сети.
		 *
		 * @note Приставки размера буфера, выводимого методом «bytes», задают степени
		 *       числа 1024: единицы измерения этих двух методов не совпадают намеренно
		 *
		 * @see bpsBuffer
		 *
		 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
		 * @return    количество байт в секунду
		 *
		 * @code{.cpp}
		 * fmk::bpsSize("8bps");     // 1
		 * fmk::bpsSize("1Kbps");    // 125
		 * fmk::bpsSize("1.5Mbps");  // 187500
		 * fmk::bpsSize("100Mbps");  // 12500000
		 * @endcode
		 *
		 * \~english
		 * @brief Function of getting the number of the bytes per second from a string
		 *
		 * @details The bandwidth of a network is set in bits, and is yielded in bytes:
		 *          the parsed value is divided by eight. The prefixes at that set
		 *          the powers of the number 1000, and not of 1024, as it is accepted for the bandwidth
		 *          of a network.
		 *
		 * @note The prefixes of the size of a buffer, yielded by the «bytes» method, set the powers
		 *       of the number 1024: the units of the measurement of these two methods do not coincide deliberately
		 *
		 * @see bpsBuffer
		 *
		 * @param str bandwidth of the network (bps, kbps, Mbps, Gbps)
		 * @return    number of the bytes per second
		 *
		 * @code{.cpp}
		 * fmk::bpsSize("8bps");     // 1
		 * fmk::bpsSize("1Kbps");    // 125
		 * fmk::bpsSize("1.5Mbps");  // 187500
		 * fmk::bpsSize("100Mbps");  // 12500000
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t bpsSize(const string_view str) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения размера буфера в байтах
		 *
		 * @details Выводится размер приёмного либо передающего буфера сокета,
		 *          отвечающий заданной пропускной способности сети.
		 *
		 * @see bpsSize
		 *
		 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
		 * @return    размер буфера в байтах
		 *
		 * \~english
		 * @brief Function of getting the size of a buffer in bytes
		 *
		 * @details What is yielded is the size of the receiving or of the transmitting buffer of a socket,
		 *          answering the given bandwidth of the network.
		 *
		 * @see bpsSize
		 *
		 * @param str bandwidth of the network (bps, kbps, Mbps, Gbps)
		 * @return    size of the buffer in bytes
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t bpsBuffer(const string_view str) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения размера в байтах из строки
		 *
		 * @details Единица измерения сличается без учёта регистра и может отделяться
		 *          от числа пробелом. Приставки задают степени числа 1024, а не 1000.
		 *
		 * @note Запись, не начинающаяся цифрой, выводится нулевым значением
		 *
		 * @note Единица измерения обязательна: задача метода — получить точное число
		 *       байт из записи размерности, а не разобрать число. Запись из одних
		 *       цифр выводится нулевым значением, и разбирать её следует модулем
		 *       лексического разбора чисел
		 *
		 * @see bytes(const double, const bool)
		 *
		 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
		 * @return    размер в байтах
		 *
		 * @code{.cpp}
		 * fmk::bytes("1Kb");        // 1024
		 * fmk::bytes("1 Kb");       // 1024
		 * fmk::bytes("1.5 Mb");     // 1572864
		 * fmk::bytes("100 Gb");     // 107374182400
		 * fmk::bytes("1024 bytes"); // 1024
		 * @endcode
		 *
		 * \~english
		 * @brief Function of getting the size in bytes from a string
		 *
		 * @details The unit of the measurement is matched without the case taken into account and may be separated
		 *          from the number by a space. The prefixes set the powers of the number 1024, and not of 1000.
		 *
		 * @note A record not beginning with a digit is yielded as a zero value
		 *
		 * @note The unit of the measurement is obligatory: the task of the method is to obtain the exact number
		 *       of the bytes from a record of a dimension, and not to parse a number. A record of the digits
		 *       alone is yielded as a zero value, and it should be parsed by the module
		 *       of the lexical parsing of the numbers
		 *
		 * @see bytes(const double, const bool)
		 *
		 * @param str string of the designation of the dimension (b, Kb, Mb, Gb, Tb)
		 * @return    size in bytes
		 *
		 * @code{.cpp}
		 * fmk::bytes("1Kb");        // 1024
		 * fmk::bytes("1 Kb");       // 1024
		 * fmk::bytes("1.5 Mb");     // 1572864
		 * fmk::bytes("100 Gb");     // 107374182400
		 * fmk::bytes("1024 bytes"); // 1024
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ double bytes(const string_view str) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертации байт в строку
		 *
		 * @details Единица измерения подбирается наибольшей из тех, при которой число
		 *          остаётся не меньше единицы. Запись числа выполняется методом «noexp»
		 *          и от установленной локали не зависит.
		 *
		 * @note Запись, выводимая этим методом, разбирается обратно одноимённым
		 *       методом до того же значения
		 *
		 * @see bytes(const string_view)
		 *
		 * @param value   количество байт
		 * @param onlyNum выводить только числа
		 * @return        полученная строка
		 *
		 * @code{.cpp}
		 * fmk::bytes(1024.);     // «1 Kb»
		 * fmk::bytes(1572864.);  // «1.5 Mb»
		 * fmk::bytes(512.);      // «512 bytes»
		 * fmk::bytes(0.);        // «0 bytes»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the conversion of the bytes into a string
		 *
		 * @details The unit of the measurement is picked the largest of those at which the number
		 *          remains not less than one. The record of the number is performed by the «noexp» method
		 *          and does not depend on the set locale.
		 *
		 * @note The record yielded by this method is parsed back by the method of the same
		 *       name up to the same value
		 *
		 * @see bytes(const string_view)
		 *
		 * @param value   number of the bytes
		 * @param onlyNum output only the numbers
		 * @return        the obtained string
		 *
		 * @code{.cpp}
		 * fmk::bytes(1024.);     // «1 Kb»
		 * fmk::bytes(1572864.);  // «1.5 Mb»
		 * fmk::bytes(512.);      // «512 bytes»
		 * fmk::bytes(0.);        // «0 bytes»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string bytes(const double value, const bool onlyNum = false) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки текста на соответствие флагу
		 *
		 * @param letter текст для проверки
		 * @param flag   флаг проверки
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Function of checking a text for the correspondence to a flag
		 *
		 * @param letter text to check
		 * @param flag   flag of the check
		 * @return       result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool is(const char letter, const check_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки текста на соответствие флагу
		 *
		 * @param letter текст для проверки
		 * @param flag   флаг проверки
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Function of checking a text for the correspondence to a flag
		 *
		 * @param letter text to check
		 * @param flag   flag of the check
		 * @return       result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool is(const wchar_t letter, const check_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки текста на соответствие флагу
		 *
		 * @details Проверка выполняется над всем текстом целиком: результат выводится
		 *          истиной, лишь когда флагу отвечает каждый символ текста. Исключение
		 *          составляют флаги «URL», «UTF8», «PSEUDO_NUMBER» и «PRESENCE_LATIAN»,
		 *          рассматривающие текст целиком, а не посимвольно.
		 *
		 * @note Пустой текст флагу не отвечает: проверка выводит ложь
		 *
		 * @param text текст для проверки
		 * @param flag флаг проверки
		 * @return     результат проверки
		 *
		 * @code{.cpp}
		 * fmk::is("12345", awh::fmk::check_t::NUMBER);          // истина
		 * fmk::is("12.45", awh::fmk::check_t::DECIMAL);         // истина
		 * fmk::is("v1.2", awh::fmk::check_t::PSEUDO_NUMBER);    // истина
		 * fmk::is("Привет", awh::fmk::check_t::UTF8);           // истина
		 * fmk::is("https://anyks.com", awh::fmk::check_t::URL); // истина
		 * @endcode
		 *
		 * \~english
		 * @brief Function of checking a text for the correspondence to a flag
		 *
		 * @details The check is performed over the whole text entirely: the result is yielded as
		 *          truth only when every character of the text answers the flag. An exception
		 *          is made by the «URL», «UTF8», «PSEUDO_NUMBER» and «PRESENCE_LATIAN» flags,
		 *          considering the text entirely, and not character by character.
		 *
		 * @note An empty text does not answer a flag: the check yields falsehood
		 *
		 * @param text text to check
		 * @param flag flag of the check
		 * @return     result of the check
		 *
		 * @code{.cpp}
		 * fmk::is("12345", awh::fmk::check_t::NUMBER);          // true
		 * fmk::is("12.45", awh::fmk::check_t::DECIMAL);         // true
		 * fmk::is("v1.2", awh::fmk::check_t::PSEUDO_NUMBER);    // true
		 * fmk::is("Привет", awh::fmk::check_t::UTF8);           // true
		 * fmk::is("https://anyks.com", awh::fmk::check_t::URL); // true
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ bool is(string_view text, const check_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки текста на соответствие флагу
		 *
		 * @param text текст для проверки
		 * @param flag флаг проверки
		 * @return     результат проверки
		 *
		 * \~english
		 * @brief Function of checking a text for the correspondence to a flag
		 *
		 * @param text text to check
		 * @param flag flag of the check
		 * @return     result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool is(wstring_view text, const check_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки существования слова в тексте
		 *
		 * @details Слово ищется как часть текста, границы слова при этом не
		 *          проверяются: слово «код» обнаруживается в тексте «кодировка».
		 *
		 * @param word слово для проверки
		 * @param text текст в котором выполнения проверка
		 * @return     результат выполнения проверки
		 *
		 * \~english
		 * @brief Function of checking the existence of a word in a text
		 *
		 * @details The word is searched for as a part of the text, the boundaries of the word are at that not
		 *          checked: the word «code» is found in the text «codepage».
		 *
		 * @param word word to check
		 * @param text text the check is performed in
		 * @return     result of the performance of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool exists(string_view word, string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки существования слова в тексте
		 *
		 * @param word слово для проверки
		 * @param text текст в котором выполнения проверка
		 * @return     результат выполнения проверки
		 *
		 * \~english
		 * @brief Function of checking the existence of a word in a text
		 *
		 * @param word word to check
		 * @param text text the check is performed in
		 * @return     result of the performance of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool exists(wstring_view word, wstring_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция сравнения двух строк без учёта регистра
		 *
		 * @details Регистр приводится по таблице ASCII, отчего сличение не зависит
		 *          от установленной локали и годится для заголовков протоколов, где
		 *          регистр значения не имеет.
		 *
		 * @note Строки разной длины сличаются ложью сразу, длину символа в кодировке
		 *       UTF-8 сличение не учитывает: буквы вне набора ASCII сличаются побайтно
		 *       и потому с учётом регистра
		 *
		 * @note Две пустые строки признаются равными
		 *
		 * @param first  первое слово
		 * @param second второе слово
		 * @return       результат сравнения
		 *
		 * @code{.cpp}
		 * fmk::compare("Content-Type", "content-type");  // истина
		 * fmk::compare("Привет", "ПРИВЕТ");              // ложь
		 * @endcode
		 *
		 * \~english
		 * @brief Function of comparing two strings without the case taken into account
		 *
		 * @details The case is brought by the ASCII table, and therefore the matching does not depend
		 *          on the set locale and is suitable for the headers of the protocols, where
		 *          the case has no meaning.
		 *
		 * @note The strings of different lengths are matched as falsehood at once, the length of a character in the UTF-8
		 *       encoding the matching does not take into account: the letters outside the ASCII set are matched byte by byte
		 *       and therefore with the case taken into account
		 *
		 * @note Two empty strings are recognized as equal
		 *
		 * @param first  first word
		 * @param second second word
		 * @return       result of the comparison
		 *
		 * @code{.cpp}
		 * fmk::compare("Content-Type", "content-type");  // true
		 * fmk::compare("Привет", "ПРИВЕТ");              // false
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ bool compare(string_view first, string_view second) noexcept;
		/**
		 * \~russian
		 * @brief Функция сравнения двух строк без учёта регистра
		 *
		 * @param first  первое слово
		 * @param second второе слово
		 * @return       результат сравнения
		 *
		 * \~english
		 * @brief Function of comparing two strings without the case taken into account
		 *
		 * @param first  first word
		 * @param second second word
		 * @return       result of the comparison
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool compare(const char * first, const char * second) noexcept;
		/**
		 * \~russian
		 * @brief Функция сравнения двух строк без учёта регистра
		 *
		 * @param first  первое слово
		 * @param second второе слово
		 * @return       результат сравнения
		 *
		 * \~english
		 * @brief Function of comparing two strings without the case taken into account
		 *
		 * @param first  first word
		 * @param second second word
		 * @return       result of the comparison
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool compare(wstring_view first, wstring_view second) noexcept;
		/**
		 * \~russian
		 * @brief Функция сравнения двух строк без учёта регистра
		 *
		 * @param first  первое слово
		 * @param second второе слово
		 * @return       результат сравнения
		 *
		 * \~english
		 * @brief Function of comparing two strings without the case taken into account
		 *
		 * @param first  first word
		 * @param second second word
		 * @return       result of the comparison
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool compare(const wchar_t * first, const wchar_t * second) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции проверки больше первое число второго или нет (бинарным методом)
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of checking whether the first number is greater than the second one or not (by the binary method)
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция проверки больше первое число второго или нет (бинарным методом)
		 *
		 * @param num1 значение первого числа в бинарном виде
		 * @param num2 значение второго числа в бинарном виде
		 * @return     результат проверки
		 *
		 * \~english
		 * @brief Function of checking whether the first number is greater than the second one or not (by the binary method)
		 *
		 * @param num1 value of the first number in the binary form
		 * @param num2 value of the second number in the binary form
		 * @return     result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool isGreater(const T num1, const T num2) noexcept;
		/**
		 * \~russian
		 * @brief Функция проверки больше первое число второго или нет (бинарным методом)
		 *
		 * @details Сличаются **числа**, лежащие в памяти так, как их кладёт сама
		 *          машина. Перебор идёт с последнего байта: на машине с обратным
		 *          порядком байт старший разряд числа лежит именно там. Так и
		 *          задумано, и разворачивать перебор не следует - иначе метод
		 *          перестанет сличать числа.
		 *
		 * @warning Двоичный буфер, числом не являющийся, сличать этим методом
		 *          **нельзя**: сетевой адрес, аппаратный адрес, отпечаток - всё
		 *          это лежит в своём порядке, старшим байтом вперёд, и порядок их
		 *          даёт побайтное сличение `memcmp`, а не этот метод
		 *
		 * @param value1 значение первого числа в бинарном виде
		 * @param value2 значение второго числа в бинарном виде
		 * @param size   размер бинарного буфера числа
		 * @return       результат проверки
		 *
		 * \~english
		 * @brief Function of checking whether the first number is greater than the second one or not (by the binary method)
		 *
		 * @details What is matched are the **numbers** lying in the memory the way the machine itself
		 *          puts them. The traversal goes from the last byte: on a machine with the reverse
		 *          order of the bytes the higher digit of a number lies exactly there. It is
		 *          intended so, and the traversal should not be turned around — otherwise the method
		 *          will stop matching numbers.
		 *
		 * @warning A binary buffer that is not a number **must not** be matched by this
		 *          method: a network address, a hardware address, a fingerprint — all
		 *          of this lies in its own order, the higher byte first, and their order
		 *          is given by the byte by byte matching `memcmp`, and not by this method
		 *
		 * @param value1 value of the first number in the binary form
		 * @param value2 value of the second number in the binary form
		 * @param size   size of the binary buffer of the number
		 * @return       result of the check
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool isGreater(const void * value1, const void * value2, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки системной локали
		 *
		 * @details Локаль ставится всему приложению, а не одному его объекту: функция
		 *          обращается к системной установке локали. Локаль по умолчанию задана
		 *          значением «AWH_LOCALE» и ставится заведением модуля - ходом
		 *          `initialize()` либо первым же обращением к модулю.
		 *
		 * @note Разбор протокольных данных от локали не зависит: она влияет на
		 *       действия над текстом широких символов и на вывод в консоль
		 *
		 * @warning Метод меняет состояние приложения целиком: вызывать его следует
		 *          однажды при запуске, до начала работы модулей
		 *
		 * @param locale локализация приложения
		 *
		 * \~english
		 * @brief Function of setting the system locale
		 *
		 * @details The locale is set to the whole application, and not to one of its objects: the function
		 *          addresses the system setting of the locale. The locale by default is set by the
		 *          «AWH_LOCALE» value and is established by the starting of the module - by the
		 *          `initialize()` function or by the very first appeal to the module.
		 *
		 * @note The parsing of the protocol data does not depend on the locale: it influences
		 *       the actions over the text of wide characters and the output into the console
		 *
		 * @warning The method changes the state of the whole application: it should be called
		 *          once at the startup, before the beginning of the work of the modules
		 *
		 * @param locale localization of the application
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void setLocale(string_view locale = "") noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения списка пользовательских зон интернета
		 *
		 * @return список доменных зон
		 *
		 * \~english
		 * @brief Function of extracting the list of the user zones of the internet
		 *
		 * @return list of the domain zones
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const unordered_set <string> & domainZones() noexcept;
		/**
		 * \~russian
		 * @brief Функция установки пользовательской зоны
		 *
		 * @details Набор доменных зон применяется разбором адресов: зона, набору не
		 *          принадлежащая, адресом не признаётся. Модуль везёт набор зон
		 *          общего пользования, а функция пополняет его зонами частными.
		 *
		 * @warning Функция меняет состояние ПРОЦЕССА, а не отдельного объекта: набор
		 *          зон один на всё приложение, и зона, добавленная одним потребителем,
		 *          меняет разбор адресов у всех остальных. Звать её следует до начала
		 *          работы модулей, обращающихся к разбору адресов
		 *
		 * @param zone пользовательская зона
		 *
		 * @code{.cpp}
		 * fmk::domainZone("local");
		 * fmk::is("http://server.local", awh::fmk::check_t::URL);  // истина
		 * @endcode
		 *
		 * \~english
		 * @brief Function of setting a user zone
		 *
		 * @details The set of the domain zones is applied by the parsing of the addresses: a zone not belonging
		 *          to the set is not recognized as an address. The module carries the set of the zones
		 *          of the common use, and the method supplements it with the private zones.
		 *
		 * @warning The function changes the state of the PROCESS, and not of a separate object: the set of
		 *          the zones is one for the whole application, and a zone added by one consumer changes
		 *          the parsing of the addresses for all the others. It should be called before the beginning of
		 *          the work of the modules addressing the parsing of the addresses
		 *
		 * @param zone user zone
		 *
		 * @code{.cpp}
		 * fmk::domainZone("local");
		 * fmk::is("http://server.local", awh::fmk::check_t::URL);  // true
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ void domainZone(const string_view zone) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки списка пользовательских зон
		 *
		 * @param zones список доменных зон интернета
		 *
		 * \~english
		 * @brief Function of setting the list of the user zones
		 *
		 * @param zones list of the domain zones of the internet
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void domainZones(const unordered_set <string> & zones) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения координат url адресов в строке
		 *
		 * @details Выводится набор пар «начало-конец» размещения каждого обнаруженного
		 *          адреса, а не сами адреса: по ним текст размечается либо разбирается
		 *          дальше без повторного поиска.
		 *
		 * @note Распознание адреса опирается на набор доменных зон, пополняемый
		 *       методом «domainZone»
		 *
		 * @see domainZone
		 *
		 * @param text текст для извлечения url адресов
		 * @return     список координат с url адресами
		 *
		 * @code{.cpp}
		 * for(auto & item : fmk::urls("см. https://anyks.com и ftp://a.b"))
		 *     const string url = text.substr(item.first, item.second - item.first);
		 * @endcode
		 *
		 * \~english
		 * @brief Function of extracting the coordinates of the url addresses in a string
		 *
		 * @details What is yielded is a set of the «beginning-end» pairs of the placement of every found
		 *          address, and not the addresses themselves: by them the text is marked up or parsed
		 *          further without a repeated search.
		 *
		 * @note The recognition of an address relies on the set of the domain zones supplemented
		 *       by the «domainZone» method
		 *
		 * @see domainZone
		 *
		 * @param text text to extract the url addresses from
		 * @return     list of the coordinates with the url addresses
		 *
		 * @code{.cpp}
		 * for(auto & item : fmk::urls("see https://anyks.com and ftp://a.b"))
		 *     const string url = text.substr(item.first, item.second - item.first);
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ unordered_map <size_t, size_t> urls(string_view text) noexcept;
		/**
		 * \~russian
		 * @brief Функция разбора имени кодировки
		 *
		 * @details Имя приводится к нормальному виду: буквы записываются в нижнем
		 *          регистре, окружающие пробельные символы опускаются. Распознаются
		 *          все имена, которыми кодировки обозначает стандарт кодировок
		 *          консорциума WHATWG, а не одни лишь канонические.
		 *
		 * @note Нераспознанное имя выводится значением «NONE» и ошибкой не считается:
		 *       имя задаётся отправителем произвольно
		 *
		 * @param name имя кодировки, заданное заголовком протокола
		 * @return     обозначение кодировки либо признак нераспознанного имени
		 *
		 * @code{.cpp}
		 * fmk::codepage("windows-1251");  // codepage_t::CP1251
		 * fmk::codepage(" CP1251 ");      // codepage_t::CP1251
		 * fmk::codepage("koi8-r");        // codepage_t::KOI8_R
		 * @endcode
		 *
		 * \~english
		 * @brief Function of parsing the name of an encoding
		 *
		 * @details The name is brought to the normal form: the letters are written in the lower
		 *          case, the surrounding whitespace characters are omitted. Recognized are
		 *          all the names by which the encodings are designated by the encoding standard of the
		 *          WHATWG consortium, and not the canonical ones alone.
		 *
		 * @note An unrecognized name is yielded as the value «NONE» and is not considered an error:
		 *       the name is set by the sender arbitrarily
		 *
		 * @param name name of the encoding set by a header of a protocol
		 * @return     designation of the encoding or a sign of an unrecognized name
		 *
		 * @code{.cpp}
		 * fmk::codepage("windows-1251");  // codepage_t::CP1251
		 * fmk::codepage(" CP1251 ");      // codepage_t::CP1251
		 * fmk::codepage("koi8-r");        // codepage_t::KOI8_R
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ codepage_t codepage(string_view name) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения имени кодировки по её обозначению
		 *
		 * @param codepage обозначение кодировки текста
		 * @return         каноническое имя кодировки
		 *
		 * \~english
		 * @brief Function of extracting the name of an encoding by its designation
		 *
		 * @param codepage designation of the encoding of a text
		 * @return         canonical name of the encoding
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string codepage(const codepage_t codepage) noexcept;
		/**
		 * \~russian
		 * @brief Функция определения кодировки текста
		 *
		 * @details Определение выполняется проверкой правильности записи текста
		 *          в кодировке UTF-8: текст, ей отвечающий, признаётся записанным
		 *          в UTF-8, а не отвечающий — записанным в заданной кодировке.
		 *
		 * @note Кодировку однобайтового текста определить нельзя: любая
		 *       последовательность байтов записана в любой однобайтовой кодировке.
		 *       Предполагаемая кодировка задаётся доводом и по умолчанию равна CP1251
		 *
		 * @param text     текст, кодировку которого требуется определить
		 * @param fallback кодировка, предполагаемая для текста, записью UTF-8 не являющегося
		 * @return         обозначение определённой кодировки текста
		 *
		 * @code{.cpp}
		 * fmk::detect("Привет");                   // codepage_t::UTF8
		 * fmk::detect("\xCF\xF0\xE8\xE2\xE5\xF2"); // codepage_t::CP1251
		 * @endcode
		 *
		 * \~english
		 * @brief Function of determining the encoding of a text
		 *
		 * @details The determination is performed by the check of the correctness of the record of the text
		 *          in the UTF-8 encoding: a text answering it is recognized as written
		 *          in UTF-8, and a not answering one — as written in the given encoding.
		 *
		 * @note The encoding of a single-byte text cannot be determined: any
		 *       sequence of bytes is written in any single-byte encoding.
		 *       The assumed encoding is set by an argument and by default equals CP1251
		 *
		 * @param text     text the encoding of which is required to be determined
		 * @param fallback encoding assumed for a text that is not a UTF-8 record
		 * @return         designation of the determined encoding of the text
		 *
		 * @code{.cpp}
		 * fmk::detect("Привет");                   // codepage_t::UTF8
		 * fmk::detect("\xCF\xF0\xE8\xE2\xE5\xF2"); // codepage_t::CP1251
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ codepage_t detect(string_view text, const codepage_t fallback = codepage_t::CP1251) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертирования текста из одной кодировки в другую
		 *
		 * @details Отказ конвертирования выводится пустым текстом и записывается
		 *          в лог. Кодировки задаются обозначением, полученным разбором имени
		 *          методом «codepage» либо определением кодировки методом «detect».
		 *
		 * @note Совпадение кодировок отказом не является: текст выводится без изменений
		 *
		 * @param text    текст для конвертирования
		 * @param from    кодировка, в которой записан текст
		 * @param to      кодировка, в которую требуется сконвертировать текст
		 * @param replace порядок обращения с символами, кодировке не представимыми
		 * @return        сконвертированный текст в требуемой кодировке
		 *
		 * @code{.cpp}
		 * // Приведение тела ответа к UTF-8 по заголовку Content-Type
		 * const auto codepage = fmk::codepage("windows-1251");
		 * const string body = fmk::transcode(payload, codepage, awh::fmk::codepage_t::UTF8);
		 *
		 * // Замена символов, кодировке не представимых, знаком вопроса
		 * fmk::transcode("Привет", awh::fmk::codepage_t::UTF8,
		 *               awh::fmk::codepage_t::ISO8859_1,
		 *               awh::fmk::replace_t::REPLACE);           // «??????»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting a text from one encoding into another
		 *
		 * @details A refusal of the conversion is yielded as an empty text and is written
		 *          into the log. The encodings are set by a designation obtained by the parsing of a name
		 *          by the «codepage» method or by the determination of the encoding by the «detect» method.
		 *
		 * @note A coincidence of the encodings is not a refusal: the text is yielded unchanged
		 *
		 * @param text    text to convert
		 * @param from    encoding the text is written in
		 * @param to      encoding the text is required to be converted into
		 * @param replace order of dealing with the characters not representable in the encoding
		 * @return        the converted text in the required encoding
		 *
		 * @code{.cpp}
		 * // The conversion of the body of an answer to UTF-8 by the Content-Type header
		 * const auto codepage = fmk::codepage("windows-1251");
		 * const string body = fmk::transcode(payload, codepage, awh::fmk::codepage_t::UTF8);
		 *
		 * // The replacement of the characters not representable in the encoding by a question mark
		 * fmk::transcode("Привет", awh::fmk::codepage_t::UTF8,
		 *               awh::fmk::codepage_t::ISO8859_1,
		 *               awh::fmk::replace_t::REPLACE);           // «??????»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string transcode(string_view text, const codepage_t from, const codepage_t to, const replace_t replace = replace_t::STRICT) noexcept;
		/**
		 * \~russian
		 * @brief Функция подсчёта количества указанной буквы в слове
		 *
		 * @details Подсчёт ведётся с учётом регистра.
		 *
		 * @param word   слово в котором нужно подсчитать букву
		 * @param letter букву которую нужно подсчитать
		 * @return       результат подсчёта
		 *
		 * @code{.cpp}
		 * fmk::countLetter("example.com", L'.');  // 1
		 * @endcode
		 *
		 * \~english
		 * @brief Function of counting the number of the specified letter in a word
		 *
		 * @details The counting is performed with the case taken into account.
		 *
		 * @param word   word the letter needs to be counted in
		 * @param letter letter that needs to be counted
		 * @return       result of the counting
		 *
		 * @code{.cpp}
		 * fmk::countLetter("example.com", L'.');  // 1
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t countLetter(string_view word, const wchar_t letter) noexcept;
		/**
		 * \~russian
		 * @brief Функция подсчёта количества указанной буквы в слове
		 *
		 * @param word   слово в котором нужно подсчитать букву
		 * @param letter букву которую нужно подсчитать
		 * @return       результат подсчёта
		 *
		 * \~english
		 * @brief Function of counting the number of the specified letter in a word
		 *
		 * @param word   word the letter needs to be counted in
		 * @param letter letter that needs to be counted
		 * @return       result of the counting
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ size_t countLetter(wstring_view word, const wchar_t letter) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации одного символа
		 *
		 * @param letter символ для трансформации
		 * @param flag   флаг трансформации
		 * @return       трансформированный символ
		 *
		 * \~english
		 * @brief Function of the transformation of a single character
		 *
		 * @param letter character to transform
		 * @param flag   flag of the transformation
		 * @return       the transformed character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ char transform(const char letter, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации одного символа
		 *
		 * @param letter символ для трансформации
		 * @param flag   флаг трансформации
		 * @return       трансформированный символ
		 *
		 * \~english
		 * @brief Function of the transformation of a single character
		 *
		 * @param letter character to transform
		 * @param flag   flag of the transformation
		 * @return       the transformed character
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wchar_t transform(const wchar_t letter, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @details Строка изменяется на месте и выводится ссылкой на неё же.
		 *
		 * @note Перегрузка, принимающая строку доводом-значением (`string_view`), исходной
		 *       записи не меняет, а выводит изменённую копию. Перегрузка же, принимающая
		 *       ссылку на постоянную строку, меняет поданную запись НА МЕСТЕ - так же,
		 *       как эта: постоянство снимается намеренно, чтобы править можно было
		 *       строку любого вида, а отвечает за это вызывающий. Предмет, заведённый
		 *       постоянным, подавать туда нельзя
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * @code{.cpp}
		 * string text = "  hello-world  ";
		 * fmk::transform(text, awh::fmk::transform_t::TRIM);        // «hello-world»
		 * fmk::transform(text, awh::fmk::transform_t::SMART_CASE);  // «Hello-World»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @details The string is changed in place and is yielded as a reference to itself.
		 *
		 * @note The overload taking a string as an argument by value (`string_view`) does not change
		 *       the original record, but yields a changed copy. The overload taking a reference to
		 *       a constant string, however, changes the submitted record IN PLACE - the same way as
		 *       this one: the constancy is removed deliberately, so that a string of any kind could be
		 *       changed, and the caller answers for that. An object declared constant must not be
		 *       submitted there
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * @code{.cpp}
		 * string text = "  hello-world  ";
		 * fmk::transform(text, awh::fmk::transform_t::TRIM);        // «hello-world»
		 * fmk::transform(text, awh::fmk::transform_t::SMART_CASE);  // «Hello-World»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string & transform(string & text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring & transform(wstring & text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const string & transform(const string & text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const wstring & transform(const wstring & text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string transform(string_view text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция трансформации строки
		 *
		 * @param text текст для трансформации
		 * @param flag флаг трансформации
		 * @return     трансформированная строка
		 *
		 * \~english
		 * @brief Function of the transformation of a string
		 *
		 * @param text text to transform
		 * @param flag flag of the transformation
		 * @return     the transformed string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring transform(wstring_view text, const transform_t flag) noexcept;
		/**
		 * \~russian
		 * @brief Функция объединения списка строк в одну строку
		 *
		 * @details Разделитель размещается между записями списка, а перед первой
		 *          записью и после последней не размещается.
		 *
		 * @note Пустой список даёт пустую строку, список из одной записи — саму запись
		 *
		 * @see split
		 *
		 * @param items список строк которые необходимо объединить
		 * @param delim разделитель
		 * @return      строка полученная после объединения
		 *
		 * @code{.cpp}
		 * fmk::join({"gzip", "deflate", "br"}, ", ");  // «gzip, deflate, br»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of joining a list of strings into one string
		 *
		 * @details The separator is placed between the records of the list, and before the first
		 *          record and after the last one it is not placed.
		 *
		 * @note An empty list gives an empty string, a list of one record — the record itself
		 *
		 * @see split
		 *
		 * @param items list of the strings that need to be joined
		 * @param delim separator
		 * @return      string obtained after the joining
		 *
		 * @code{.cpp}
		 * fmk::join({"gzip", "deflate", "br"}, ", ");  // «gzip, deflate, br»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string join(const vector <string> & items, string_view delim) noexcept;
		/**
		 * \~russian
		 * @brief Функция объединения списка строк в одну строку
		 *
		 * @param items список строк которые необходимо объединить
		 * @param delim разделитель
		 * @return      строка полученная после объединения
		 *
		 * \~english
		 * @brief Function of joining a list of strings into one string
		 *
		 * @param items list of the strings that need to be joined
		 * @param delim separator
		 * @return      string obtained after the joining
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring join(const vector <wstring> & items, wstring_view delim) noexcept;
		/**
		 * \~russian
		 * @brief Функция разделения строк на токены
		 *
		 * @details Разделителем служит вся переданная строка целиком, а не любой из
		 *          её символов. Полученный контейнер очищается перед заполнением.
		 *
		 * @note Контейнер выводится ссылкой на переданный, что позволяет
		 *       переиспользовать выделенную им память при разборе многих строк
		 *
		 * @see join
		 *
		 * @param text      строка для парсинга
		 * @param delim     разделитель
		 * @param container результирующий вектор
		 *
		 * @code{.cpp}
		 * vector <string> items;
		 * fmk::split("gzip, deflate, br", ", ", items);  // {"gzip", "deflate", "br"}
		 * @endcode
		 *
		 * \~english
		 * @brief Function of splitting strings into tokens
		 *
		 * @details The separator is the whole passed string entirely, and not any of
		 *          its characters. The obtained container is cleared before the filling.
		 *
		 * @note The container is yielded as a reference to the passed one, which allows
		 *       the memory allocated by it to be reused at the parsing of many strings
		 *
		 * @see join
		 *
		 * @param text      string to parse
		 * @param delim     separator
		 * @param container resulting vector
		 *
		 * @code{.cpp}
		 * vector <string> items;
		 * fmk::split("gzip, deflate, br", ", ", items);  // {"gzip", "deflate", "br"}
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ vector <string> & split(string_view text, string_view delim, vector <string> & container) noexcept;
		/**
		 * \~russian
		 * @brief Функция разделения строк на токены
		 *
		 * @param text      строка для парсинга
		 * @param delim     разделитель
		 * @param container результирующий вектор
		 *
		 * \~english
		 * @brief Function of splitting strings into tokens
		 *
		 * @param text      string to parse
		 * @param delim     separator
		 * @param container resulting vector
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ vector <wstring> & split(wstring_view text, wstring_view delim, vector <wstring> & container) noexcept;
		/**
		 * \~russian
		 * @brief Функция порверки на сколько процентов (A > B) или (A < B)
		 *
		 * @details Выводится отклонение первого числа от второго в процентах от
		 *          второго. Превышение выводится положительным значением, недостача —
		 *          отрицательным.
		 *
		 * @note Нулевое второе число даёт нулевой результат: делить на него нельзя
		 *
		 * @param a первое число
		 * @param b второе число
		 * @return  результат расчёта
		 *
		 * @code{.cpp}
		 * fmk::rate(150.f, 100.f);  // 50
		 * fmk::rate(50.f, 100.f);   // -50
		 * @endcode
		 *
		 * \~english
		 * @brief Function of checking by how many percent (A > B) or (A < B)
		 *
		 * @details What is yielded is the deviation of the first number from the second one in percent of
		 *          the second one. An excess is yielded as a positive value, a shortage —
		 *          as a negative one.
		 *
		 * @note A zero second number gives a zero result: it is impossible to divide by it
		 *
		 * @param a first number
		 * @param b second number
		 * @return  result of the computation
		 *
		 * @code{.cpp}
		 * fmk::rate(150.f, 100.f);  // 50
		 * fmk::rate(50.f, 100.f);   // -50
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ float rate(const float a, const float b) noexcept;
		/**
		 * \~russian
		 * @brief Функция приведения количества символов после запятой к указанному количества
		 *
		 * @details Дробная часть отсекается, а не округляется: число приводится
		 *          к ближайшему меньшему.
		 *
		 * @note Округление выполняется методом «noexp», выводящим запись числа
		 *
		 * @see noexp
		 *
		 * @param x число для приведения
		 * @param n количество символов после запятой
		 * @return  сформированное число
		 *
		 * @code{.cpp}
		 * fmk::floor(3.14159, 2);  // 3.14
		 * fmk::floor(3.999, 2);    // 3.99
		 * @endcode
		 *
		 * \~english
		 * @brief Function of bringing the number of the characters after the decimal point to the specified number
		 *
		 * @details The fractional part is cut off, and not rounded: the number is brought
		 *          to the nearest smaller one.
		 *
		 * @note The rounding is performed by the «noexp» method, yielding the record of a number
		 *
		 * @see noexp
		 *
		 * @param x number to bring
		 * @param n number of the characters after the decimal point
		 * @return  the built number
		 *
		 * @code{.cpp}
		 * fmk::floor(3.14159, 2);  // 3.14
		 * fmk::floor(3.999, 2);    // 3.99
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ double floor(const double x, const uint8_t n) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции конвертации чисел в указанную систему счисления
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the conversion of the numbers into the specified numeral system
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция конвертации чисел в указанную систему счисления
		 *
		 * @details Разряды свыше девятого записываются прописными буквами латиницы.
		 *          Запись по основанию 2 дополняется нулями до разрядности типа,
		 *          прочие основания записи не дополняют.
		 *
		 * @note Основание системы счисления принимается в пределах от 2 до 36:
		 *       при ином основании выводится пустая строка
		 *
		 * @warning Знак числа записью не сохраняется: число со знаком записывается
		 *          так, как лежит в памяти, отчего значение -42 типа int32_t даёт
		 *          запись «4294967254»
		 * @see atoi
		 *
		 * @param value число для конвертации
		 * @param radix система счисления
		 * @return      полученная строка в указанной системе счисления
		 *
		 * @code{.cpp}
		 * fmk::itoa <uint32_t> (255, 16);  // «FF»
		 * fmk::itoa <uint8_t> (255, 2);    // «11111111»
		 * fmk::itoa <uint32_t> (255, 2);   // «00000000000000000000000011111111»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the conversion of the numbers into the specified numeral system
		 *
		 * @details The digits beyond the ninth are written by the capital letters of the Latin alphabet.
		 *          The record by the base 2 is padded with zeroes up to the width of the type,
		 *          the other bases do not pad the record.
		 *
		 * @note The base of the numeral system is accepted within the limits from 2 to 36:
		 *       at another base an empty string is yielded
		 *
		 * @warning The sign of a number is not preserved by the record: a signed number is written
		 *          the way it lies in the memory, and therefore the value -42 of the int32_t type gives
		 *          the record «4294967254»
		 *
		 * @see atoi
		 *
		 * @param value number to convert
		 * @param radix numeral system
		 * @return      the obtained string in the specified numeral system
		 *
		 * @code{.cpp}
		 * fmk::itoa <uint32_t> (255, 16);  // «FF»
		 * fmk::itoa <uint8_t> (255, 2);    // «11111111»
		 * fmk::itoa <uint32_t> (255, 2);   // «00000000000000000000000011111111»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string itoa(const T value, const uint8_t radix) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертации чисел в указанную систему счисления
		 *
		 * @param value бинарный буфер числа для конвертации
		 * @param size  размер бинарного буфера
		 * @param radix система счисления
		 * @return      полученная строка в указанной системе счисления
		 *
		 * \~english
		 * @brief Function of the conversion of the numbers into the specified numeral system
		 *
		 * @param value binary buffer of the number to convert
		 * @param size  size of the binary buffer
		 * @param radix numeral system
		 * @return      the obtained string in the specified numeral system
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ string itoa(const void * value, const size_t size, const uint8_t radix) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @details Разбор выполняется по таблице ASCII и от установленной локали
		 *          не зависит: разделителем дробной части всегда служит точка.
		 *          Тип разбираемого числа задаётся доводом шаблона.
		 *
		 * @note Запись, числом не являющаяся, выводится нулевым значением. Тем же
		 *       нулём выводится и запись, за пределы типа выходящая: разбор её
		 *       отвергает, а иного значения у метода нет. Отличить «не число» от
		 *       «слишком велико» по выводу нельзя - требуется различать, разбирайте
		 *       модулем лексического разбора чисел
		 *
		 * @note Часть строки разбирается её представлением, а не парой из указателя
		 *       и длины
		 *
		 * @see itoa
		 *
		 * @param value строковое представление числа
		 * @return      числовое значение в десятичной системе счисления
		 *
		 * @code{.cpp}
		 * fmk::atoi <uint32_t> ("12345");     // 12345
		 * fmk::atoi <double> ("3.14159");     // 3.14159
		 * fmk::atoi <int32_t> ("-42");        // -42
		 * fmk::atoi <uint32_t> ("ff", 16);    // 255
		 * @endcode
		 *
		 * @code{.cpp}
		 * fmk::atoi <uint32_t> (string_view{text.data() + begin, length});
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @details The parsing is performed by the ASCII table and does not depend on the set locale:
		 *          the separator of the fractional part is always a dot.
		 *          The type of the parsed number is set by the argument of the template.
		 *
		 * @note A record that is not a number is yielded as a zero value. By the same
		 *       zero is yielded also a record going beyond the limits of the type: the parsing
		 *       rejects it, and another value the method does not have. To tell «not a number»
		 *       from «too large» by the yielded value is impossible - if the distinction is
		 *       required, parse by the module of the lexical parsing of the numbers
		 *
		 * @note A part of a string is parsed by its view, and not by a pair of a pointer
		 *       and a length
		 *
		 * @see itoa
		 *
		 * @param value string representation of a number
		 * @return      numeric value in the decimal numeral system
		 *
		 * @code{.cpp}
		 * fmk::atoi <uint32_t> ("12345");     // 12345
		 * fmk::atoi <double> ("3.14159");     // 3.14159
		 * fmk::atoi <int32_t> ("-42");        // -42
		 * fmk::atoi <uint32_t> ("ff", 16);    // 255
		 * @endcode
		 *
		 * @code{.cpp}
		 * fmk::atoi <uint32_t> (string_view{text.data() + begin, length});
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ T atoi(string_view value) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @param value число в бинарном виде для конвертации в 10-ю систему
		 * @param radix система счисления
		 * @return      полученное значение в десятичной системе счисления
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @param value number in the binary form to convert into the decimal system
		 * @param radix numeral system
		 * @return      the obtained value in the decimal numeral system
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T atoi(string_view value, const uint8_t radix) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @param value  число в бинарном виде для конвертации в 10-ю систему
		 * @param radix  система счисления
		 * @param buffer бинарный буфер куда следует положить результат
		 * @param size   размер бинарного буфера куда следует положить результат
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @param value  number in the binary form to convert into the decimal system
		 * @param radix  numeral system
		 * @param buffer binary buffer the result should be put into
		 * @param size   size of the binary buffer the result should be put into
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void atoi(string_view value, const uint8_t radix, void * buffer, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @param value строковое представление числа
		 * @return      числовое значение в десятичной системе счисления
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @param value string representation of a number
		 * @return      numeric value in the decimal numeral system
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T atoi(wstring_view value) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
		 *
		 * @tparam T тип данных с которым работает функция
		 *
		 * \~english
		 * @brief Template of the function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @tparam T type of the data the function works with
		 *
		 * \~
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @param value число в бинарном виде для конвертации в 10-ю систему
		 * @param radix система счисления
		 * @return      полученное значение в десятичной системе счисления
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @param value number in the binary form to convert into the decimal system
		 * @param radix numeral system
		 * @return      the obtained value in the decimal numeral system
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ T atoi(wstring_view value, const uint8_t radix) noexcept;
		/**
		 * \~russian
		 * @brief Функция конвертации строковых чисел в десятичную систему счисления
		 *
		 * @param value  число в бинарном виде для конвертации в 10-ю систему
		 * @param radix  система счисления
		 * @param buffer бинарный буфер куда следует положить результат
		 * @param size   размер бинарного буфера куда следует положить результат
		 *
		 * \~english
		 * @brief Function of the conversion of the string numbers into the decimal numeral system
		 *
		 * @param value  number in the binary form to convert into the decimal system
		 * @param radix  numeral system
		 * @param buffer binary buffer the result should be put into
		 * @param size   size of the binary buffer the result should be put into
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void atoi(wstring_view value, const uint8_t radix, void * buffer, const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода числа в безэкспоненциальную форму
		 *
		 * @details Количество знаков после запятой задаётся размером шага, дробная
		 *          часть при этом округляется. Целое число записывается без дробной
		 *          части вовсе, каким бы ни был размер шага.
		 *
		 * @note Запись не зависит от установленной локали: разделителем дробной части
		 *       всегда служит точка, разделителей разрядов запись не содержит
		 *
		 * @note Нулевой размер шага даёт запись «0»: округлять до нуля знаков после
		 *       запятой следует перегрузкой с подбором точности
		 *
		 * @param number число для перевода
		 * @param step   размер шага после запятой
		 * @return       число в безэкспоненциальной форме
		 *
		 * @see noexp(const double, const bool)
		 *
		 * @code{.cpp}
		 * fmk::noexp(2986.808299, static_cast <uint8_t> (3));  // «2986.808»
		 * fmk::noexp(2986.808299, static_cast <uint8_t> (4));  // «2986.8083»
		 * fmk::noexp(2986., static_cast <uint8_t> (4));        // «2986»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting a number into the non-exponential form
		 *
		 * @details The number of the digits after the decimal point is set by the size of the step, the fractional
		 *          part is at that rounded. An integer number is written without a fractional
		 *          part at all, whatever the size of the step may be.
		 *
		 * @note The record does not depend on the set locale: the separator of the fractional part
		 *       is always a dot, the record contains no separators of the groups of the digits
		 *
		 * @note A zero size of the step gives the record «0»: rounding to zero digits after
		 *       the decimal point should be done by the overload with the picking of the precision
		 *
		 * @param number number to convert
		 * @param step   size of the step after the decimal point
		 * @return       number in the non-exponential form
		 *
		 * @see noexp(const double, const bool)
		 *
		 * @code{.cpp}
		 * fmk::noexp(2986.808299, static_cast <uint8_t> (3));  // «2986.808»
		 * fmk::noexp(2986.808299, static_cast <uint8_t> (4));  // «2986.8083»
		 * fmk::noexp(2986., static_cast <uint8_t> (4));        // «2986»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string noexp(const double number, const uint8_t step) noexcept;
		/**
		 * \~russian
		 * @brief Функция перевода числа в безэкспоненциальную форму
		 *
		 * @details Количество знаков после запятой подбирается наименьшим из тех, при
		 *          котором запись читается обратно ровно тем же числом. Запись выходит
		 *          краткой, не теряя при этом ни одного значащего разряда.
		 *
		 * @note Запись не зависит от установленной локали: разделителем дробной части
		 *       всегда служит точка, разделителей разрядов запись не содержит
		 *
		 * @note Округления запись не выполняет: число, требующее семнадцати значащих
		 *       разрядов, все семнадцать и получит. Для краткой записи с потерей
		 *       точности следует пользоваться перегрузкой с размером шага
		 *
		 * @note Довод вывода одних лишь разрядов сохранён ради совместимости вызовов
		 *       и на запись не влияет: посторонних символов она не содержит
		 *
		 * @param number  число для перевода
		 * @param onlyNum выводить только числа
		 * @return        число в безэкспоненциальной форме
		 *
		 * @see noexp(const double, const uint8_t)
		 *
		 * @code{.cpp}
		 * fmk::noexp(1e+19);                 // «10000000000000000000»
		 * fmk::noexp(1e-5);                  // «0.00001»
		 * fmk::noexp(1536. / 1024.);         // «1.5»
		 * fmk::noexp(0.111);                 // «0.111»
		 * fmk::noexp(-2986.808299);          // «-2986.808299»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of converting a number into the non-exponential form
		 *
		 * @details The number of the digits after the decimal point is picked the smallest of those at
		 *          which the record is read back as exactly the same number. The record comes out
		 *          brief, without losing a single significant digit at that.
		 *
		 * @note The record does not depend on the set locale: the separator of the fractional part
		 *       is always a dot, the record contains no separators of the groups of the digits
		 *
		 * @note The record performs no rounding: a number requiring seventeen significant
		 *       digits will receive all seventeen of them. For a brief record with a loss of
		 *       the precision one should use the overload with the size of the step
		 *
		 * @note The argument of the output of the digits alone is preserved for the sake of the compatibility of the calls
		 *       and does not influence the record: it contains no extraneous characters
		 *
		 * @param number  number to convert
		 * @param onlyNum output only the numbers
		 * @return        number in the non-exponential form
		 *
		 * @see noexp(const double, const uint8_t)
		 *
		 * @code{.cpp}
		 * fmk::noexp(1e+19);                 // «10000000000000000000»
		 * fmk::noexp(1e-5);                  // «0.00001»
		 * fmk::noexp(1536. / 1024.);         // «1.5»
		 * fmk::noexp(0.111);                 // «0.111»
		 * fmk::noexp(-2986.808299);          // «-2986.808299»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string noexp(const double number, const bool onlyNum = false) noexcept;
		/**
		 * \~russian
		 * @brief Функция записи числа с разделением разрядов
		 *
		 * @details Разряды целой части разделяются знаком-разделителем, отчего запись
		 *          крупного числа читается глазом заметно легче. Запись выполняется
		 *          поверх метода @c noexp(), оттого разделителем дробной части в ней
		 *          всегда служит точка, а сама запись не зависит от установленной
		 *          местности.
		 *
		 * @details Разделение разрядов местностью **не** задаётся и задаваться не
		 *          может: признак «'» функции printf стандартом языка C не описан и
		 *          у MS Windows не работает вовсе, а установка местности меняет
		 *          запись чисел во всём приложении разом, включая ту, что уходит в
		 *          протокол. Оттого разделение включается вызовом этого метода, а
		 *          не состоянием, и приложение вправе ставить себе любую местность.
		 *
		 * @note Приложению, которому разделение не нужно, вызывать метод незачем:
		 *       @c noexp() разделителей не ставит и не ставил
		 *
		 * @param number    записываемое число
		 * @param precision количество знаков после запятой, отрицательное для подбора
		 * @param separator знак-разделитель разрядов целой части
		 * @param size      количество разрядов в одной группе
		 * @return          запись числа с разделёнными разрядами
		 *
		 * @see noexp(const double, const bool)
		 *
		 * @code{.cpp}
		 * fmk::grouped(10000000000.5, 4);            // «10,000,000,000.5000»
		 * fmk::grouped(10000000000.5, 4, ' ');       // «10 000 000 000.5000»
		 * fmk::grouped(1234567.);                    // «1,234,567»
		 * fmk::grouped(123456789., 0, ',', 4);       // «1,2345,6789»
		 * fmk::grouped(-9876.5, 1);                  // «-9,876.5»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of writing a number with the separation of the groups of the digits
		 *
		 * @details The groups of the digits of the integer part are separated by a separator character, and therefore a record
		 *          of a large number is read by the eye noticeably easier. The record is performed
		 *          on top of the @c noexp() method, and therefore the separator of the fractional part in it
		 *          is always a dot, and the record itself does not depend on the set locale.
		 *
		 * @details The separation of the groups of the digits is **not** set by the locale and cannot be
		 *          set by it: the «'» flag of the printf function is not described by the standard of the C language and
		 *          does not work at all on MS Windows, and setting the locale changes
		 *          the record of the numbers in the whole application at once, including the one going into
		 *          the protocol. Therefore the separation is switched on by a call to this method, and
		 *          not by a state, and the application is free to set itself any locale.
		 *
		 * @note An application that does not need the separation has no reason to call the method:
		 *       @c noexp() does not place separators and never did
		 *
		 * @param number    number being written
		 * @param precision number of the digits after the decimal point, a negative one for the picking
		 * @param separator separator character of the groups of the digits of the integer part
		 * @param size      number of the digits in one group
		 * @return          record of the number with the separated groups of the digits
		 *
		 * @see noexp(const double, const bool)
		 *
		 * @code{.cpp}
		 * fmk::grouped(10000000000.5, 4);            // «10,000,000,000.5000»
		 * fmk::grouped(10000000000.5, 4, ' ');       // «10 000 000 000.5000»
		 * fmk::grouped(1234567.);                    // «1,234,567»
		 * fmk::grouped(123456789., 0, ',', 4);       // «1,2345,6789»
		 * fmk::grouped(-9876.5, 1);                  // «-9,876.5»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string grouped(const double number, const int32_t precision = -1, const char separator = ',', const uint8_t size = 3) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции записи целого числа с разделением разрядов
		 *
		 * @tparam T тип записываемого целого числа
		 *
		 * \~english
		 * @brief Template of the function of writing an integer number with the separation of the groups of the digits
		 *
		 * @tparam T type of the integer number being written
		 *
		 */
		template <typename T>
		/**
		 * \~russian
		 * @brief Функция записи целого числа с разделением разрядов
		 *
		 * @details Целое число записывается своим видом, а не приведением к числу с
		 *          плавающей точкой: приведение теряло бы точность за пределами
		 *          девяти квадриллионов, где мантисса двойной точности кончается.
		 *
		 * @param number    записываемое число
		 * @param separator знак-разделитель разрядов
		 * @param size      количество разрядов в одной группе
		 * @return          запись числа с разделёнными разрядами
		 *
		 * @code{.cpp}
		 * fmk::grouped <uint64_t> (18446744073709551615ULL);  // «18,446,744,073,709,551,615»
		 * fmk::grouped <int32_t> (-1234567);                  // «-1,234,567»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of writing an integer number with the separation of the groups of the digits
		 *
		 * @details An integer number is written by its own kind, and not by a cast to a floating
		 *          point number: the cast would lose the precision beyond
		 *          nine quadrillions, where the mantissa of the double precision ends.
		 *
		 * @param number    number being written
		 * @param separator separator character of the groups of the digits
		 * @param size      number of the digits in one group
		 * @return          record of the number with the separated groups of the digits
		 *
		 * @code{.cpp}
		 * fmk::grouped <uint64_t> (18446744073709551615ULL);  // «18,446,744,073,709,551,615»
		 * fmk::grouped <int32_t> (-1234567);                  // «-1,234,567»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string grouped(const T number, const char separator = ',', const uint8_t size = 3) noexcept;
		/**
		 * \~russian
		 * @brief Шаблон функции реализации функции формирования форматированной строки
		 *
		 * @tparam Args типы доводов функции
		 *
		 * \~english
		 * @brief Template of the function of the implementation of the function of building a formatted string
		 *
		 * @tparam Args types of the arguments of the function
		 *
		 * \~
		 */
		template <typename... Args>
		/**
		 * \~russian
		 * @brief Функция реализации функции формирования форматированной строки
		 *
		 * @details Запись формата задана стандартной библиотекой и совпадает с
		 *          записью, принимаемой функцией «printf».
		 *
		 * @warning Проверки соответствия доводов записи формата не выполняется:
		 *          несоответствие ведёт к незаданному поведению, как и у «printf»
		 *
		 * @param format формат строки вывода
		 * @param args   передаваемые аргументы
		 * @return       сформированная строка
		 *
		 * @code{.cpp}
		 * fmk::format("%s:%u", "127.0.0.1", 8080);  // «127.0.0.1:8080»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the implementation of the function of building a formatted string
		 *
		 * @details The record of the format is set by the standard library and coincides with
		 *          the record accepted by the «printf» function.
		 *
		 * @warning No check of the correspondence of the arguments to the record of the format is performed:
		 *          a discrepancy leads to an unspecified behaviour, as with «printf»
		 *
		 * @param format format of the string of the output
		 * @param args   passed arguments
		 * @return       the built string
		 *
		 * @code{.cpp}
		 * fmk::format("%s:%u", "127.0.0.1", 8080);  // «127.0.0.1:8080»
		 * @endcode
		 *
		 */
		printable_t <char, Args...> format(const char * format, Args... args) noexcept {
			// Выполняем формирование строки по записи «printf»
			return detail::formatted(format, args...);
		}
		/**
		 * \~russian
		 * @brief Шаблон функции реализации функции формирования форматированной строки
		 *
		 * @tparam Args типы доводов функции
		 *
		 * \~english
		 * @brief Template of the function of the implementation of the function of building a formatted string
		 *
		 * @tparam Args types of the arguments of the function
		 *
		 * \~
		 */
		template <typename... Args>
		/**
		 * \~russian
		 * @brief Функция реализации функции формирования форматированной строки
		 *
		 * @param format формат строки вывода
		 * @param args   передаваемые аргументы
		 * @return       сформированная строка
		 *
		 * \~english
		 * @brief Function of the implementation of the function of building a formatted string
		 *
		 * @param format format of the string of the output
		 * @param args   passed arguments
		 * @return       the built string
		 *
		 * \~
		 */
		printable_t <wchar_t, Args...> format(const wchar_t * format, Args... args) noexcept {
			// Выполняем формирование строки по записи «printf»
			return detail::formatted(format, args...);
		}
		/**
		 * \~russian
		 * @brief Функция реализации функции формирования форматированной строки
		 *
		 * @details Места подстановки обозначаются знаком доллара с номером записи
		 *          списка, начиная с единицы. Записи подставляются по всему тексту,
		 *          сколько бы раз обозначение ни встретилось. Записи «\r», «\n»
		 *          и «\t» при этом раскрываются в соответствующие им символы.
		 *
		 * @note Формат проходится один раз, и вставленный текст повторно не разбирается:
		 *       запись с «$2» внутри вставляется дословно. Номер читается самым длинным,
		 *       какой есть в списке («$10» при десяти записях - десятая запись, при девяти -
		 *       первая и символ «0»). Пустая запись вставляется пустой, «$$» даёт одиночный
		 *       знак доллара, обозначение без записи в списке остаётся как есть
		 *
		 * @note Типов доводов запись не задаёт, отчего несоответствия доводов
		 *       записи, свойственного «printf», здесь не возникает
		 *
		 * @note Перекрытие с тем же именем выполняет подстановку по записи «printf».
		 *       Столкновения на строковом литерале не возникает: то перекрытие
		 *       исключается из разбора, когда среди доводов встречается список
		 *       записей подстановки
		 *
		 * @param format формат строки вывода
		 * @param items  список аргументов строки
		 * @return       сформированная строка
		 *
		 * @code{.cpp}
		 * fmk::format("$1 -> $2 ($1)", vector <string> {"a", "b"});
		 * // «a -> b (a)»
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the implementation of the function of building a formatted string
		 *
		 * @details The places of the substitution are designated by a dollar sign with the number of a record
		 *          of the list, starting from one. The records are substituted all over the text,
		 *          however many times the designation may occur. The records «\r», «\n»
		 *          and «\t» are at that expanded into the characters corresponding to them.
		 *
		 * @note The format is passed once, and the inserted text is not parsed again: a record with «$2»
		 *       inside is inserted verbatim. The number is read the longest one present in the list («$10»
		 *       with ten records is the tenth record, with nine - the first one and the character «0»). An empty
		 *       record is inserted empty, «$$» gives a single dollar sign, a designation without a record stays as is
		 *
		 * @note The record does not set the types of the arguments, and therefore the discrepancy of the arguments
		 *       to the record, inherent to «printf», does not arise here
		 *
		 * @note The overload with the same name performs the substitution by the «printf» record.
		 *       A collision on a string literal does not arise: that overload
		 *       is excluded from the resolution when among the arguments there occurs a list
		 *       of the records of the substitution
		 *
		 * @param format format of the string of the output
		 * @param items  list of the arguments of the string
		 * @return       the built string
		 *
		 * @code{.cpp}
		 * fmk::format("$1 -> $2 ($1)", vector <string> {"a", "b"});
		 * // «a -> b (a)»
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string format(string_view format, const vector <string> & items) noexcept;
		/**
		 * \~russian
		 * @brief Функция реализации функции формирования форматированной строки
		 *
		 * @param format формат строки вывода
		 * @param items  список аргументов строки
		 * @return       сформированная строка
		 *
		 * \~english
		 * @brief Function of the implementation of the function of building a formatted string
		 *
		 * @param format format of the string of the output
		 * @param items  list of the arguments of the string
		 * @return       the built string
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring format(wstring_view format, const vector <wstring> & items) noexcept;
		/**
		 * \~russian
		 * @brief Функция замены в тексте слово на другое слово
		 *
		 * @details Заменяются все вхождения слова, а не одно лишь первое. Текст
		 *          изменяется на месте и выводится ссылкой на него же.
		 *
		 * @note Опущенное слово замены задаёт удаление вхождений
		 *
		 * @param text текст в котором нужно произвести замену
		 * @param word слово для поиска
		 * @param alt  слово на которое нужно произвести замену
		 * @return     результирующий текст
		 *
		 * @code{.cpp}
		 * string text = "a-b-c";
		 * fmk::replace(text, "-", "/");  // «a/b/c»
		 * fmk::replace(text, "/");       // «abc», слово опускается
		 * @endcode
		 *
		 * \~english
		 * @brief Function of replacing a word in a text by another word
		 *
		 * @details All the occurrences of the word are replaced, and not the first one alone. The text
		 *          is changed in place and is yielded as a reference to itself.
		 *
		 * @note An omitted word of the replacement sets the removal of the occurrences
		 *
		 * @param text text the replacement needs to be performed in
		 * @param word word to search for
		 * @param alt  word the replacement needs to be performed by
		 * @return     resulting text
		 *
		 * @code{.cpp}
		 * string text = "a-b-c";
		 * fmk::replace(text, "-", "/");  // «a/b/c»
		 * fmk::replace(text, "/");       // "abc", the word is omitted
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ string & replace(string & text, const string & word, const string & alt = "") noexcept;
		/**
		 * \~russian
		 * @brief Функция замены в тексте слово на другое слово
		 *
		 * @param text текст в котором нужно произвести замену
		 * @param word слово для поиска
		 * @param alt  слово на которое нужно произвести замену
		 * @return     результирующий текст
		 *
		 * \~english
		 * @brief Function of replacing a word in a text by another word
		 *
		 * @param text text the replacement needs to be performed in
		 * @param word word to search for
		 * @param alt  word the replacement needs to be performed by
		 * @return     resulting text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ wstring & replace(wstring & text, const wstring & word, const wstring & alt = L"") noexcept;
		/**
		 * \~russian
		 * @brief Функция замены в тексте слово на другое слово
		 *
		 * @param text текст в котором нужно произвести замену
		 * @param word слово для поиска
		 * @param alt  слово на которое нужно произвести замену
		 * @return     результирующий текст
		 *
		 * \~english
		 * @brief Function of replacing a word in a text by another word
		 *
		 * @param text text the replacement needs to be performed in
		 * @param word word to search for
		 * @param alt  word the replacement needs to be performed by
		 * @return     resulting text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const string & replace(const string & text, const string & word, const string & alt = "") noexcept;
		/**
		 * \~russian
		 * @brief Функция замены в тексте слово на другое слово
		 *
		 * @param text текст в котором нужно произвести замену
		 * @param word слово для поиска
		 * @param alt  слово на которое нужно произвести замену
		 * @return     результирующий текст
		 *
		 * \~english
		 * @brief Function of replacing a word in a text by another word
		 *
		 * @param text text the replacement needs to be performed in
		 * @param word word to search for
		 * @param alt  word the replacement needs to be performed by
		 * @return     resulting text
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const wstring & replace(const wstring & text, const wstring & word, const wstring & alt = L"") noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения ключей и значений из текста
		 *
		 * @details Разбор рассчитан на записи журналов и заголовки протоколов, где
		 *          значение от следующего ключа ничем, кроме разделителя записей,
		 *          не отделено. Устройство разбора описано выше.
		 *
		 * @note Повторяющиеся ключи сохраняются полностью, отчего результатом
		 *       является multimap. Порядок следования записей в исходном тексте
		 *       контейнером не сохраняется: он снимается потоковым разбором
		 *
		 * @see kv(const uint64_t, string_view, string_view, function <void (const uint64_t, const string_view, const string_view)>, string_view, const vector <string> &)
		 *
		 * @param text      текст из которого извлекаются записи
		 * @param delim     разделитель записей
		 * @param separator разделитель ключа и значения
		 * @param escaping  символы экранирования
		 * @return          список найденных элементов
		 *
		 * @code{.cpp}
		 * const auto items = fmk::kv("cs1=a cs2=b cs3=", " ");
		 * // {"cs1": "a", "cs2": "b", "cs3": ""}
		 * @endcode
		 *
		 * \~english
		 * @brief Function of extracting the keys and the values from a text
		 *
		 * @details The parsing is designed for the records of the logs and for the headers of the protocols, where
		 *          a value is separated from the next key by nothing but the separator of the records.
		 *          The construction of the parsing is described above.
		 *
		 * @note The repeating keys are preserved entirely, and therefore the result
		 *       is a multimap. The order of the following of the records in the original text
		 *       is not preserved by the container: it is taken by the streaming parsing
		 *
		 * @see kv(const uint64_t, string_view, string_view, function <void (const uint64_t, const string_view, const string_view)>, string_view, const vector <string> &)
		 *
		 * @param text      text the records are extracted from
		 * @param delim     separator of the records
		 * @param separator separator of the key and the value
		 * @param escaping  escaping characters
		 * @return          list of the found elements
		 *
		 * @code{.cpp}
		 * const auto items = fmk::kv("cs1=a cs2=b cs3=", " ");
		 * // {"cs1": "a", "cs2": "b", "cs3": ""}
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ unordered_multimap <string, string> kv(string_view text, string_view delim, string_view separator = "=", const vector <string> & escaping = detail::escapingText()) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения ключей и значений из текста
		 *
		 * @param text      текст из которого извлекаются записи
		 * @param delim     разделитель записей
		 * @param separator разделитель ключа и значения
		 * @param escaping  символы экранирования
		 * @return          список найденных элементов
		 *
		 * \~english
		 * @brief Function of extracting the keys and the values from a text
		 *
		 * @param text      text the records are extracted from
		 * @param delim     separator of the records
		 * @param separator separator of the key and the value
		 * @param escaping  escaping characters
		 * @return          list of the found elements
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ unordered_multimap <wstring, wstring> kv(wstring_view text, wstring_view delim, wstring_view separator = L"=", const vector <wstring> & escaping = detail::escapingWide()) noexcept;
		/**
		 * \~russian
		 * @brief Функция потокового извлечения ключей и значений из текста
		 *
		 * @details Записи отдаются функции обратного вызова строго в порядке их
		 *          появления в тексте, а контейнер под них не заводится вовсе.
		 *          Идентификатор потока разбора передаётся функции без изменений
		 *          и позволяет разбирать несколько текстов одним обработчиком.
		 *
		 * @param sid       идентификатор потока разбора
		 * @param text      текст из которого извлекаются записи
		 * @param delim     разделитель записей
		 * @param callback  функция обратного вызова для каждой найденной записи
		 * @param separator разделитель ключа и значения
		 * @param escaping  символы экранирования
		 *
		 * @code{.cpp}
		 * fmk::kv(1, "cs1=a cs2=b", " ", [](const uint64_t sid,
		 *  const string_view key, const string_view value) noexcept {
		 *     // Записи придут в порядке «cs1=a», затем «cs2=b»
		 * });
		 * @endcode
		 *
		 * \~english
		 * @brief Function of the streaming extraction of the keys and the values from a text
		 *
		 * @details The records are yielded to the callback function strictly in the order of their
		 *          appearance in the text, and no container is started for them at all.
		 *          The identifier of the stream of the parsing is passed to the function unchanged
		 *          and allows several texts to be parsed by one handler.
		 *
		 * @param sid       identifier of the stream of the parsing
		 * @param text      text the records are extracted from
		 * @param delim     separator of the records
		 * @param callback  callback function for every found record
		 * @param separator separator of the key and the value
		 * @param escaping  escaping characters
		 *
		 * @code{.cpp}
		 * fmk::kv(1, "cs1=a cs2=b", " ", [](const uint64_t sid,
		 *  const string_view key, const string_view value) noexcept {
		 *     // The records will come in the order "cs1=a", then "cs2=b"
		 * });
		 * @endcode
		 *
		 */
		__AWH_SHARED_EXPORT__ void kv(const uint64_t sid, string_view text, string_view delim, function <void (const uint64_t, const string_view, const string_view)> callback, string_view separator = "=", const vector <string> & escaping = detail::escapingText()) noexcept;
		/**
		 * \~russian
		 * @brief Функция потокового извлечения ключей и значений из текста
		 *
		 * @param sid       идентификатор потока разбора
		 * @param text      текст из которого извлекаются записи
		 * @param delim     разделитель записей
		 * @param callback  функция обратного вызова для каждой найденной записи
		 * @param separator разделитель ключа и значения
		 * @param escaping  символы экранирования
		 *
		 * \~english
		 * @brief Function of the streaming extraction of the keys and the values from a text
		 *
		 * @param sid       identifier of the stream of the parsing
		 * @param text      text the records are extracted from
		 * @param delim     separator of the records
		 * @param callback  callback function for every found record
		 * @param separator separator of the key and the value
		 * @param escaping  escaping characters
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void kv(const uint64_t sid, wstring_view text, wstring_view delim, function <void (const uint64_t, const wstring_view, const wstring_view)> callback, wstring_view separator = L"=", const vector <wstring> & escaping = detail::escapingWide()) noexcept;
	}
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "macro/restore.hpp"
