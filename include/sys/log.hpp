/**
 * @file log.hpp
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
 * @brief Заголовочный файл модуля логирования — пространство имён awh::log с уровнями важности,
 *        форматированием сообщений, ротацией файлов и набором приёмников вывода: консоль, файл,
 *        SysLog и пользовательская функция обратного вызова
 *
 * @details Модуль выполнен пространством имён, а не классом: журнал нужен всякому модулю
 *          библиотеки, и его настройка в пределах приложения одна. Состояние журнала
 *          заведено единственным на процесс, отчего указатель на объект журнала больше
 *          никуда не передаётся, а заголовочный файл не тянет за собой ни модуль
 *          форматирования, ни средства работы с потоками.
 *
 * \~english
 * @brief Header file of the logging module — the awh::log namespace with severity levels,
 *        message formatting, file rotation and a set of output sinks: console, file,
 *        SysLog and a user callback function
 *
 * @details The module is made a namespace and not a class: the log is needed by every module
 *          of the library, and its setting within the application is one. The state of the log
 *          is started single per process, whereby a pointer to the log object is no longer
 *          handed anywhere, and the header file does not drag along either the formatting
 *          module or the means of working with the threads.
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
#include <cstdint>
#include <type_traits>
#include <initializer_list>
#include <functional>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "macro/global.hpp"

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
	 * @brief Пространство имён работы с логами
	 *
	 * @details Состояние журнала единственно на процесс и заводится при первом
	 *          обращении. Настройка журнала выполняется функциями настройки,
	 *          а вывод — функциями print() и debug().
	 *
	 * \~english
	 * @brief Namespace of the work with the logs
	 *
	 * @details The state of the log is single per process and is started on the first
	 *          appeal. The setting of the log is performed by the functions of the setting,
	 *          and the output — by the print() and debug() functions.
	 *
	 * \~
	 */
	namespace log {
		/**
		 * \~russian
		 * @brief Флаги логирования
		 *
		 * \~english
		 * @brief Logging flags
		 *
		 * \~
		 */
		enum class flag_t : uint8_t {
			NONE     = 0x00, // Флаг не установлен
			INFO     = 0x01, // Информационное сообщение
			WARNING  = 0x02, // Предупреждающее сообщение
			CRITICAL = 0x03  // Критическое сообщение
		};
		/**
		 * \~russian
		 * @brief Политика поведения при переполнении очереди асинхронного вывода
		 *
		 * @note значения совпадают с awh::Screen::overflow_t для прямого преобразования
		 *
		 * \~english
		 * @brief Policy of the behaviour on an overflow of the queue of the asynchronous output
		 *
		 * @note the values coincide with awh::Screen::overflow_t for a direct conversion
		 *
		 * \~
		 */
		enum class overflow_t : uint8_t {
			WAIT     = 0x00, // Блокировать поставщика до появления свободного места
			DROP_NEW = 0x01, // Отбрасывать новое поступившее сообщение
			DROP_OLD = 0x02  // Вытеснять самое старое сообщение из очереди
		};
		/**
		 * \~russian
		 * @brief Флаги разделителя формирования логов
		 *
		 * \~english
		 * @brief Flags of the separator of the building of the logs
		 *
		 * \~
		 */
		enum class separator_t : uint8_t {
			NONE   = 0x00, // Разделитель отключён
			SMART  = 0x01, // Умный разделитель по длине сообщения
			ALWAYS = 0x02  // Отображать разделитель всегда
		};
		/**
		 * \~russian
		 * @brief Флаги работы логов
		 *
		 * \~english
		 * @brief Flags of the work of the logs
		 *
		 * \~
		 */
		enum class mode_t : uint8_t {
			NONE     = 0x00, // Вывод логов запрещён
			FILE     = 0x01, // Разрешено выводить логи в файлы
			SYSLOG   = 0x02, // Разрешено отправлять логи в SysLog
			CONSOLE  = 0x03, // Разрешено выводить логи в консоль
			DEFERRED = 0x04  // Разрешено выводить логи в функцию обратного вызова
		};
		/**
		 * \~russian
		 * @brief Уровни логирования
		 *
		 * \~english
		 * @brief Logging levels
		 *
		 * \~
		 */
		enum class level_t : uint8_t {
			NONE             = 0x00, // Логирование отключено
			ALL              = 0x07, // Разрешено выводить все виды логов
			INFO             = 0x01, // Разрешено выводить только информационные логи
			WARNING          = 0x02, // Разрешено выводить только логи предупреждения
			CRITICAL         = 0x03, // Разрешено выводить только критические логи
			INFO_WARNING     = 0x04, // Разрешено выводить логи информационные и предупреждения
			INFO_CRITICAL    = 0x05, // Разрешено выводить логи информационные и критические
			WARNING_CRITICAL = 0x06  // Разрешено выводить логи предупреждения и критические
		};

		/**
		 * \~russian
		 * @brief Функция обратного вызова для подписки на генерацию логов
		 *
		 * \~english
		 * @brief Callback function for subscribing to log generation
		 *
		 * \~
		 */
		using callback_t = function <void (const flag_t, string_view)>;

		/**
		 * \~russian
		 * @brief Класс аргумента вызываемого метода, уложенный к записи в журнал
		 *
		 * @details Тип заведён взамен кортежа `tuple <T...>`, каким доводы подавались прежде.
		 *          Кортеж вынуждал держать сборку записи в заголовочном файле: его состав -
		 *          часть типа, и добраться до доводов без шаблона нельзя. Набор же видов,
		 *          какие журналу подаются, замкнут - строки, числа, указатели да перечисления,
		 *          приводимые к числу, - и замкнутый этот набор укладывается в одно значение.
		 *          Оттого debug() и стала обычной функцией, целиком живущей в реализации.
		 *
		 * @warning Заведения НЕЯВНЫЕ намеренно, `explicit` у них стоять не может: доводы
		 *          подаются списком заведения `{a, b}`, а список этот приводит каждый свой
		 *          член неявно. С `explicit` не собирается НИ ОДИН вызов debug()
		 *
		 * @warning Целочисленные виды названы основными именами - `short`, `int`, `long`,
		 *          `long long`, - а НЕ именами постоянной ширины. Имена постоянной ширины
		 *          суть прозвища основных, и прозвища эти по системам ложатся по-разному:
		 *          у macOS `int64_t` есть `long long`, а у Linux LP64 - `long`. Объяви мы
		 *          рядом `long` и `int64_t` - у macOS вышли бы два разных заведения, а у
		 *          Linux одно и то же дважды, и сборка бы встала. Основные же имена
		 *          различны всегда и покрывают собою все прозвища
		 *
		 * @note Виды перечислены поимённо, а не сведены к паре «знаковое - беззнаковое»:
		 *       довод `int` при двух таких заведениях подошёл бы к обоим одинаково, и выбор
		 *       вышел бы неоднозначным. Заведения указателя и перечисления оставлены
		 *       шаблонными по необходимости: поимённо ни те, ни другие не перечислимы,
		 *       но тела у них нет - оба передоверяют работу заведениям выше.
		 *
		 * \~english
		 * @brief One argument of the called method laid out for the writing into the log
		 *
		 * @details The type is started instead of the tuple `tuple <T...>` the arguments were passed by before.
		 *          The tuple forced to keep the assembling of the record in the header file: its composition is
		 *          a part of the type, and it is not possible to get to the arguments without a template. But the set
		 *          of the kinds that are passed to the log is closed - strings, numbers, pointers and enumerations
		 *          reducible to a number, - and this closed set fits into one value.
		 *          Because of that the debug() became an ordinary function living entirely in the implementation.
		 *
		 * @note The kinds are enumerated by name and not reduced to a pair of «signed - unsigned»:
		 *       an argument of the `int` at two such constructors would suit both of them equally, and the choice
		 *       would come out ambiguous. The constructors of the pointer and of the enumeration are left
		 *       templated by necessity: neither of them is enumerable by name,
		 *       but they have no body - both delegate the work to the constructors above.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Argument {
			public:
				/**
				 * \~russian
				 * @brief Вид уложенного довода
				 *
				 * \~english
				 * @brief Kind of the laid out argument
				 *
				 * \~
				 */
				enum class kind_t : uint8_t {
					NONE     = 0x00, // Довод не уложен
					REAL     = 0x01, // Довод числовой дробный
					TEXT     = 0x02, // Довод строковый узкий
					WTEXT    = 0x03, // Довод строковый широкий
					SYMBOL   = 0x04, // Довод символьный
					POINTER  = 0x05, // Довод указателем
					BOOLEAN  = 0x06, // Довод логический
					SIGNED   = 0x07, // Довод числовой со знаком
					UNSIGNED = 0x08  // Довод числовой без знака
				};
			private:
				// Вид уложенного довода
				kind_t _kind;
			private:
				/**
				 * \~russian
				 * @brief Уложенное значение довода
				 *
				 * @note Объединение допустимо оттого, что всякий из видов тривиально копируем,
				 *       `string_view` с `wstring_view` в том числе: особого обхождения при
				 *       заведении и снятии они не требуют
				 *
				 * \~english
				 * @brief The laid out value of the argument
				 *
				 * @note The union is admissible because every one of the kinds is trivially copyable,
				 *       the `string_view` with the `wstring_view` among them: they require no special handling
				 *       at the construction and at the destruction
				 *
				 * \~
				 */
				union {
					// Символьный тип данных
					char _symbol;
					// Числовой тип данных с плавающей точкой
					double _real;
					// Булевый тип данных
					bool _boolean;
					// Знаковый тип данных
					int64_t _signed;
					// Беззнаковый тип данных
					uint64_t _unsigned;
					// Строковый тип данных
					string_view _text;
					// Тип данных широкой строки
					wstring_view _wtext;
					// Тип данных указателя
					const void * _pointer;
				};
			public:
				/**
				 * \~russian
				 * @brief Метод укладки довода в строку доводов
				 *
				 * @param result строка доводов, куда ведётся укладка
				 *
				 * \~english
				 * @brief Method of the laying of the argument into the string of the arguments
				 *
				 * @param result string of the arguments the laying is performed into
				 *
				 * \~
				 */
				void lay(string & result) const noexcept;
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
				explicit Argument() noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				Argument(nullptr_t) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const bool value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const char value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const long value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const unsigned long value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const signed char value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const unsigned char value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const short value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const unsigned short value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const int value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const unsigned int value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const long long value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const unsigned long long value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const float value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const double value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const long double value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(char * value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(wchar_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const char * value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const wchar_t * value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(wstring_view value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const void * value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const string & value) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор заведения довода по видам его значения
				 *
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructors of the argument by the kinds of its value
				 *
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const wstring & value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон конструктора заведения довода перечислением
				 *
				 * @tparam T вид перечисления
				 *
				 * \~english
				 * @brief Template constructor of the argument by an enumeration
				 *
				 * @tparam T kind of the enumeration
				 *
				 * \~
				 */
				template <typename T, typename = typename enable_if <is_enum <T>::value>::type>
				/**
				 * \~russian
				 * @brief Конструктор заведения довода перечислением
				 *
				 * @details Тела у заведения нет: перечисление приводится к числу и
				 *          передоверяется заведению знакового довода.
				 *
				 * @tparam T вид перечисления
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructor of the argument by an enumeration
				 *
				 * @details The constructor has no body: the enumeration is reduced to a number and
				 *          is delegated to the constructor of the signed argument.
				 *
				 * @tparam T kind of the enumeration
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(const T value) noexcept : Argument(static_cast <int64_t> (value)) {}
				/**
				 * \~russian
				 * @brief Шаблон конструктора заведения довода указателем произвольного вида
				 *
				 * @tparam T вид указуемого
				 *
				 * \~english
				 * @brief Template constructor of the argument by a pointer of an arbitrary kind
				 *
				 * @tparam T kind of the pointed to
				 *
				 * \~
				 */
				template <typename T>
				/**
				 * \~russian
				 * @brief Конструктор заведения довода указателем произвольного вида
				 *
				 * @details Тела у заведения нет: указатель приводится к безвидовому и
				 *          передоверяется заведению указательного довода. Указатели же
				 *          строковые к нему не попадают - заведения их стоят выше и
				 *          выбираются прежде шаблонного.
				 *
				 * @tparam T вид указуемого
				 * @param value укладываемое значение довода
				 *
				 * \~english
				 * @brief Constructor of the argument by a pointer of an arbitrary kind
				 *
				 * @details The constructor has no body: the pointer is reduced to a typeless one and
				 *          is delegated to the constructor of the pointer argument. But the string
				 *          pointers do not reach it - their constructors stand above and
				 *          are chosen before the templated one.
				 *
				 * @tparam T kind of the pointed to
				 * @param value the laid out value of the argument
				 *
				 * \~
				 */
				Argument(T * value) noexcept : Argument(static_cast <const void *> (value)) {}
		} arg_t;
		
		/**
		 * \~russian
		 * @brief Функция установки название сервиса для вывода лога
		 *
		 * @param name название сервиса для вывода лога
		 *
		 * \~english
		 * @brief Function of setting the name of the service for the log output
		 *
		 * @param name name of the service for the log output
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void name(string_view name) noexcept;
		/**
		 * \~russian
		 * @brief Функция извлечения установленного формата лога
		 *
		 * @return формат лога для извлечения
		 *
		 * \~english
		 * @brief Function of getting the set format of the log
		 *
		 * @return format of the log to get
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const string & format() noexcept;
		/**
		 * \~russian
		 * @brief Функция установки формата даты и времени для вывода лога
		 *
		 * @param format формат даты и времени для вывода лога
		 *
		 * \~english
		 * @brief Function of setting the format of the date and time for the log output
		 *
		 * @param format format of the date and time for the log output
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void format(string_view format) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки безопасности работы потоков
		 *
		 * @param mode флаг режима безопасности потоков
		 *
		 * \~english
		 * @brief Function of setting the thread safety of the work
		 *
		 * @param mode flag of the thread safety mode
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void threadSafety(const bool mode) noexcept;
		/**
		 * \~russian
		 * @brief Функция получения установленных режимов вывода логов
		 *
		 * @return список режимов вывода логов
		 *
		 * \~english
		 * @brief Function of getting the set modes of the log output
		 *
		 * @return list of the modes of the log output
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const unordered_set <mode_t> & mode() noexcept;
		/**
		 * \~russian
		 * @brief Функция добавления режимов вывода логов
		 *
		 * @param mode список режимов вывода логов
		 *
		 * \~english
		 * @brief Function of adding the modes of the log output
		 *
		 * @param mode list of the modes of the log output
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void mode(const unordered_set <mode_t> & mode) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки флага асинхронного режима работы
		 *
		 * @param mode флаг асинхронного режима работы
		 *
		 * \~english
		 * @brief Function of setting the flag of the asynchronous mode of the work
		 *
		 * @param mode flag of the asynchronous mode of the work
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void async(const bool mode) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки максимального размера файла логов
		 *
		 * @param size максимальный размер файла логов
		 *
		 * \~english
		 * @brief Function of setting the maximum size of the log file
		 *
		 * @param size maximum size of the log file
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void maxSize(const float size) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки размера текста для формирования разделителя
		 *
		 * @param size размер текста для формирования разделителя
		 *
		 * \~english
		 * @brief Function of setting the size of the text for the building of the separator
		 *
		 * @param size size of the text for the building of the separator
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void sepSize(const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки уровня логирования
		 *
		 * @param level уровень логирования для установки
		 *
		 * \~english
		 * @brief Function of setting the logging level
		 *
		 * @param level logging level to set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void level(const level_t level) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки максимального размера очереди асинхронного вывода
		 *
		 * @param size максимальный размер очереди (0 - без ограничения)
		 *
		 * \~english
		 * @brief Function of setting the maximum size of the queue of the asynchronous output
		 *
		 * @param size maximum size of the queue (0 — without a limit)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void maxQueue(const size_t size) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки максимального количества хранимых архивов логов
		 *
		 * @param count максимальное количество архивов (0 - без ограничения)
		 *
		 * \~english
		 * @brief Function of setting the maximum number of the kept log archives
		 *
		 * @param count maximum number of the archives (0 — without a limit)
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void maxFiles(const size_t count) noexcept;
		/**
		 * \~russian
		 * @brief Функция подписки на события логов
		 *
		 * @param callback функция обратного вызова
		 *
		 * \~english
		 * @brief Function of subscribing to the log events
		 *
		 * @param callback callback function
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void subscribe(callback_t callback) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки файла для сохранения логов
		 *
		 * @param filename путь к файлу для сохранения логов
		 *
		 * \~english
		 * @brief Function of setting the file for saving the logs
		 *
		 * @param filename path to the file for saving the logs
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void filename(string_view filename) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки разделителя сообщений логирования
		 *
		 * @param sep разделитель для установки
		 *
		 * \~english
		 * @brief Function of setting the separator of the logging messages
		 *
		 * @param sep separator to set
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void separator(const separator_t sep) noexcept;
		/**
		 * \~russian
		 * @brief Функция установки политики поведения при переполнении очереди асинхронного вывода
		 *
		 * @param overflow политика поведения при переполнении очереди
		 *
		 * \~english
		 * @brief Function of setting the policy of the behaviour on an overflow of the queue of the asynchronous output
		 *
		 * @param overflow policy of the behaviour on an overflow of the queue
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void overflow(const overflow_t overflow) noexcept;
		/**
		 * \~russian
		 * @brief Функция вывода текстовой информации в консоль или файл
		 *
		 * @param format формат строки вывода
		 * @param flag   флаг типа логирования
		 *
		 * \~english
		 * @brief Function of outputting text information into the console or into a file
		 *
		 * @param format format of the output string
		 * @param flag   flag of the type of the logging
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void print(string_view format, flag_t flag, ...) noexcept;
		/**
		 * \~russian
		 * @brief Функция вывода текстовой информации в консоль или файл
		 *
		 * @param format формат строки вывода
		 * @param flag   флаг типа логирования
		 *
		 * \~english
		 * @brief Function of outputting text information into the console or into a file
		 *
		 * @param format format of the output string
		 * @param flag   flag of the type of the logging
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void print(wstring_view format, flag_t flag, ...) noexcept;
		/**
		 * \~russian
		 * @brief Функция вывода текстовой информации в консоль или файл
		 *
		 * @param format формат строки вывода
		 * @param flag   флаг типа логирования
		 * @param args   список аргументов для замены
		 *
		 * \~english
		 * @brief Function of outputting text information into the console or into a file
		 *
		 * @param format format of the output string
		 * @param flag   flag of the type of the logging
		 * @param args   list of the arguments for the substitution
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void print(string_view format, flag_t flag, const vector <string> & args) noexcept;
		/**
		 * \~russian
		 * @brief Функция вывода текстовой информации в консоль или файл
		 *
		 * @param format формат строки вывода
		 * @param flag   флаг типа логирования
		 * @param args   список аргументов для замены
		 *
		 * \~english
		 * @brief Function of outputting text information into the console or into a file
		 *
		 * @param format format of the output string
		 * @param flag   flag of the type of the logging
		 * @param args   list of the arguments for the substitution
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void print(wstring_view format, flag_t flag, const vector <wstring> & args) noexcept;
		/**
		 * \~russian
		 * @brief Функции вывода отладочной информации в консоль или файл
		 *
		 * @details Доводы вызываемого метода подаются списком заведения - `{a, b, c}`,
		 *          а при их отсутствии пустым списком `{}`. Прежде подавался кортеж
		 *          `make_tuple(a, b, c)`, и он вынуждал держать сборку записи здесь же,
		 *          в заголовочном файле: состав кортежа есть часть его типа.
		 *
		 * @param format формат строки вывода
		 * @param method название вызываемого метода
		 * @param params доводы, переданные в метод
		 * @param flag   флаг типа логирования
		 * @param args   аргументы формирования лога
		 *
		 * \~english
		 * @brief Functions of the outputting of the debug information into the console or into a file
		 *
		 * @details The arguments of the called method are passed by an initializer list - `{a, b, c}`,
		 *          and at their absence by an empty list `{}`. Before a tuple was passed,
		 *          the `make_tuple(a, b, c)`, and it forced to keep the assembling of the record right here,
		 *          in the header file: the composition of the tuple is a part of its type.
		 *
		 * @param format format of the output string
		 * @param method name of the called method
		 * @param params arguments passed into the method
		 * @param flag   flag of the type of the logging
		 * @param args   arguments of the building of the log
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void debug(string_view format, string_view method, initializer_list <arg_t> params, flag_t flag, ...) noexcept;
		/**
		 * \~russian
		 * @brief Функции вывода отладочной информации в консоль или файл
		 *
		 * @details Доводы вызываемого метода подаются списком заведения - `{a, b, c}`,
		 *          а при их отсутствии пустым списком `{}`. Прежде подавался кортеж
		 *          `make_tuple(a, b, c)`, и он вынуждал держать сборку записи здесь же,
		 *          в заголовочном файле: состав кортежа есть часть его типа.
		 *
		 * @param format формат строки вывода
		 * @param method название вызываемого метода
		 * @param params доводы, переданные в метод
		 * @param flag   флаг типа логирования
		 * @param args   аргументы формирования лога
		 *
		 * \~english
		 * @brief Functions of the outputting of the debug information into the console or into a file
		 *
		 * @details The arguments of the called method are passed by an initializer list - `{a, b, c}`,
		 *          and at their absence by an empty list `{}`. Before a tuple was passed,
		 *          the `make_tuple(a, b, c)`, and it forced to keep the assembling of the record right here,
		 *          in the header file: the composition of the tuple is a part of its type.
		 *
		 * @param format format of the output string
		 * @param method name of the called method
		 * @param params arguments passed into the method
		 * @param flag   flag of the type of the logging
		 * @param args   arguments of the building of the log
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void debug(wstring_view format, string_view method, initializer_list <arg_t> params, flag_t flag, ...) noexcept;
		/**
		 * \~russian
		 * @brief Функции вывода отладочной информации в консоль или файл
		 *
		 * @details Доводы вызываемого метода подаются списком заведения - `{a, b, c}`,
		 *          а при их отсутствии пустым списком `{}`. Прежде подавался кортеж
		 *          `make_tuple(a, b, c)`, и он вынуждал держать сборку записи здесь же,
		 *          в заголовочном файле: состав кортежа есть часть его типа.
		 *
		 * @param format формат строки вывода
		 * @param method название вызываемого метода
		 * @param params доводы, переданные в метод
		 * @param flag   флаг типа логирования
		 * @param args   аргументы формирования лога
		 *
		 * \~english
		 * @brief Functions of the outputting of the debug information into the console or into a file
		 *
		 * @details The arguments of the called method are passed by an initializer list - `{a, b, c}`,
		 *          and at their absence by an empty list `{}`. Before a tuple was passed,
		 *          the `make_tuple(a, b, c)`, and it forced to keep the assembling of the record right here,
		 *          in the header file: the composition of the tuple is a part of its type.
		 *
		 * @param format format of the output string
		 * @param method name of the called method
		 * @param params arguments passed into the method
		 * @param flag   flag of the type of the logging
		 * @param args   arguments of the building of the log
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void debug(string_view format, string_view method, initializer_list <arg_t> params, flag_t flag, const vector <string> & args) noexcept;
		/**
		 * \~russian
		 * @brief Функции вывода отладочной информации в консоль или файл
		 *
		 * @details Доводы вызываемого метода подаются списком заведения - `{a, b, c}`,
		 *          а при их отсутствии пустым списком `{}`. Прежде подавался кортеж
		 *          `make_tuple(a, b, c)`, и он вынуждал держать сборку записи здесь же,
		 *          в заголовочном файле: состав кортежа есть часть его типа.
		 *
		 * @param format формат строки вывода
		 * @param method название вызываемого метода
		 * @param params доводы, переданные в метод
		 * @param flag   флаг типа логирования
		 * @param args   аргументы формирования лога
		 *
		 * \~english
		 * @brief Functions of the outputting of the debug information into the console or into a file
		 *
		 * @details The arguments of the called method are passed by an initializer list - `{a, b, c}`,
		 *          and at their absence by an empty list `{}`. Before a tuple was passed,
		 *          the `make_tuple(a, b, c)`, and it forced to keep the assembling of the record right here,
		 *          in the header file: the composition of the tuple is a part of its type.
		 *
		 * @param format format of the output string
		 * @param method name of the called method
		 * @param params arguments passed into the method
		 * @param flag   flag of the type of the logging
		 * @param args   arguments of the building of the log
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ void debug(wstring_view format, string_view method, initializer_list <arg_t> params, flag_t flag, const vector <wstring> & args) noexcept;
	}
};
