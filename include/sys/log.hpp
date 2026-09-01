/**
 * @file log.hpp
 * @date 2025-10-25
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
 * @brief Заголовочный файл модуля логирования — класс Logging с уровнями важности, форматированием сообщений,
 *        ротацией файлов и набором приёмников вывода: консоль, файл,
 *        SysLog и пользовательская функция обратного вызова
 *
 * \~english
 * @brief Header file of the logging module — the Logging class with severity levels, message formatting,
 *        file rotation and a set of output sinks: console, file,
 *        SysLog and a user callback function
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_LOG__
#define __AWH_LOG__

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	// Формируем переносы строк лога
	#define AWH_STRING_BREAK "\r\n"
	#define AWH_STRING_BREAKS AWH_STRING_BREAK"" AWH_STRING_BREAK
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	// Формируем переносы строк лога
	#define AWH_STRING_BREAK "\n"
	#define AWH_STRING_BREAKS AWH_STRING_BREAK"" AWH_STRING_BREAK
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <tuple>
#include <vector>
#include <memory>
#include <atomic>
#include <string>
#include <cstdint>
#include <sstream>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"
#include "locker.hpp"
#include "chrono.hpp"
#include "screen.hpp"

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
	 * @brief Класс работы с логами
	 *
	 * \~english
	 * @brief Class for working with logs
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Logging {
		public:
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
		private:
			/**
			 * \~russian
			 * @brief Класс полезной нагрузки
			 *
			 * @details Полезная нагрузка формируется в момент вызова логирования и содержит текст сообщения, дату формирования и флаг типа сообщения.
			 *
			 * \~english
			 * @brief Payload class
			 *
			 * @details The payload is built at the moment of the logging call and holds the text of the message, the date of the building and the flag of the type of the message.
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Payload {
				public:
					// Флаг полезной нагрузки
					flag_t flag;
					// Текст полезной нагрузки
					string text;
					// Дата формирования сообщения (фиксируется в момент вызова)
					string date;
				public:
					/**
					 * \~russian
					 * @brief Оператор перемещающего присваивания параметров полезной нагрузки
					 *
					 * @param payload объект полезной нагрузки для перемещения
					 * @return        текущий объект полезной нагрузки
					 *
					 * \~english
					 * @brief Move assignment operator of the parameters of the payload
					 *
					 * @param payload payload object to move
					 * @return        the current payload object
					 *
					 * \~
					 */
					Payload & operator = (Payload && payload) noexcept;
					/**
					 * \~russian
					 * @brief Оператор присваивания присваивания параметров полезной нагрузки
					 *
					 * @param payload объект полезной нагрузки для копирования
					 * @return        текущий объект полезной нагрузки
					 *
					 * \~english
					 * @brief Copy assignment operator of the parameters of the payload
					 *
					 * @param payload payload object to copy
					 * @return        the current payload object
					 *
					 * \~
					 */
					Payload & operator = (const Payload & payload) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Оператор сравнения
					 *
					 * @param payload объект полезной нагрузки для сравнения
					 * @return        результат сравнения
					 *
					 * \~english
					 * @brief Comparison operator
					 *
					 * @param payload payload object to compare with
					 * @return        result of the comparison
					 *
					 * \~
					 */
					bool operator == (const Payload & payload) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор перемещения
					 *
					 * @param payload объект полезной нагрузки для перемещения
					 *
					 * \~english
					 * @brief Move constructor
					 *
					 * @param payload payload object to move
					 *
					 * \~
					 */
					explicit Payload(Payload && payload) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор копирования
					 *
					 * @param payload объект полезной нагрузки для копирования
					 *
					 * \~english
					 * @brief Copy constructor
					 *
					 * @param payload payload object to copy
					 *
					 * \~
					 */
					explicit Payload(const Payload & payload) noexcept;
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
					explicit Payload() noexcept;
				public:
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
					~Payload() noexcept = default;
			} payload_t;
		private:
			/**
			 * \~russian
			 * @brief Базовый абстрактный приёмник вывода логов
			 *
			 * @details Приёмник вывода логов может быть реализован в виде консольного вывода,
			 *          записи в файл, отправки в SysLog или передачи в функцию обратного вызова.
			 *
			 * \~english
			 * @brief Base abstract sink of the log output
			 *
			 * @details A sink of the log output may be implemented as a console output,
			 *          a write into a file, a send into SysLog or a hand-over into a callback function.
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ Sink {
				protected:
					// Указатель на владеющий объект логирования
					const Logging * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод записи полезной нагрузки в приёмник
					 *
					 * @param payload объект полезной нагрузки
					 *
					 * \~english
					 * @brief Method of writing the payload into the sink
					 *
					 * @param payload payload object
					 *
					 * \~
					 */
					virtual void write(const payload_t & payload) const noexcept = 0;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log logging object
					 *
					 * \~
					 */
					explicit Sink(const Logging * log) noexcept;
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
					virtual ~Sink() noexcept = default;
			};
			/**
			 * \~russian
			 * @brief Приёмник вывода логов в консоль
			 *
			 * @details Приёмник вывода логов в консоль может быть реализован в виде стандартного вывода (stdout) или стандартного потока ошибок (stderr).
			 *
			 * \~english
			 * @brief Sink of the log output into the console
			 *
			 * @details Sink of the log output into the console may be implemented as the standard output (stdout) or the standard error stream (stderr).
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ ConsoleSink : public Sink {
				public:
					/**
					 * \~russian
					 * @brief Метод записи полезной нагрузки в консоль
					 *
					 * @param payload объект полезной нагрузки
					 *
					 * \~english
					 * @brief Method of writing the payload into the console
					 *
					 * @param payload payload object
					 *
					 * \~
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log logging object
					 *
					 * \~
					 */
					explicit ConsoleSink(const Logging * log) noexcept;
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
					virtual ~ConsoleSink() noexcept = default;
			};
			/**
			 * \~russian
			 * @brief Приёмник вывода логов в файл
			 *
			 * @details Приёмник вывода логов в файл может быть реализован в виде записи в указанный файл с возможностью ротации и удаления устаревших архивов.
			 *
			 * \~english
			 * @brief Sink of the log output into a file
			 *
			 * @details The sink of the log output into a file may be implemented as a write into the specified file with the possibility of rotation and of the removal of the outdated archives.
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ FileSink : public Sink {
				private:
					// Идентификатор процесса, владеющего дескриптором
					mutable pid_t _pid;
				private:
					// Постоянный дескриптор записи (на POSIX - файловый дескриптор, на Windows - HANDLE)
					mutable intptr_t _fd;
				private:
					// Путь к файлу, который сейчас открыт
					mutable string _opened;
					// Текущий размер открытого файла лога
					mutable uintmax_t _size;
				private:
					/**
					 * \~russian
					 * @brief Метод (пере)открытия постоянного дескриптора записи
					 *
					 * \~english
					 * @brief Method of (re)opening the permanent descriptor of the writing
					 *
					 * \~
					 */
					void reopen() const noexcept;
					/**
					 * \~russian
					 * @brief Метод выполнения ротации файла лога
					 *
					 * \~english
					 * @brief Method of performing the rotation of the log file
					 *
					 * \~
					 */
					void rotate() const noexcept;
					/**
					 * \~russian
					 * @brief Метод удаления устаревших архивов логов (retention)
					 *
					 * \~english
					 * @brief Method of removing the outdated log archives (retention)
					 *
					 * \~
					 */
					void retention() const noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод формирования уникального имени архива логов
					 *
					 * @return путь к файлу архива, гарантированно не конфликтующий с существующими
					 *
					 * \~english
					 * @brief Method of building a unique name of a log archive
					 *
					 * @return path to the archive file, guaranteed not to conflict with the existing ones
					 *
					 * \~
					 */
					string nextArchive() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод записи полезной нагрузки в файл
					 *
					 * @param payload объект полезной нагрузки
					 *
					 * \~english
					 * @brief Method of writing the payload into a file
					 *
					 * @param payload payload object
					 *
					 * \~
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log logging object
					 *
					 * \~
					 */
					explicit FileSink(const Logging * log) noexcept;
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
					~FileSink() noexcept;
			};
			/**
			 * \~russian
			 * @brief Приёмник отправки логов в SysLog
			 *
			 * @details Приёмник отправки логов в SysLog может быть реализован в виде отправки сообщений в системный журнал.
			 *
			 * \~english
			 * @brief Sink of the sending of the logs into SysLog
			 *
			 * @details The sink of the sending of the logs into SysLog may be implemented as the sending of messages into the system journal.
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ SyslogSink : public Sink {
				public:
					/**
					 * \~russian
					 * @brief Метод отправки полезной нагрузки в SysLog
					 *
					 * @param payload объект полезной нагрузки
					 *
					 * \~english
					 * @brief Method of sending the payload into SysLog
					 *
					 * @param payload payload object
					 *
					 * \~
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log logging object
					 *
					 * \~
					 */
					explicit SyslogSink(const Logging * log) noexcept;
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
					virtual ~SyslogSink() noexcept = default;
			};
			/**
			 * \~russian
			 * @brief Приёмник передачи логов в функцию обратного вызова
			 *
			 * @details Приёмник передачи логов в функцию обратного вызова может быть реализован в виде вызова пользовательской функции, которая будет обрабатывать полезную нагрузку.
			 *
			 * \~english
			 * @brief Sink of the hand-over of the logs into a callback function
			 *
			 * @details The sink of the hand-over of the logs into a callback function may be implemented as a call of a user function that will handle the payload.
			 *
			 * \~
			 */
			class __AWH_SHARED_EXPORT__ CallbackSink : public Sink {
				public:
					/**
					 * \~russian
					 * @brief Метод передачи полезной нагрузки в функцию обратного вызова
					 *
					 * @param payload объект полезной нагрузки
					 *
					 * \~english
					 * @brief Method of handing the payload over into the callback function
					 *
					 * @param payload payload object
					 *
					 * \~
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param log logging object
					 *
					 * \~
					 */
					explicit CallbackSink(const Logging * log) noexcept;
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
					virtual ~CallbackSink() noexcept = default;
			};
		private:
			// Флаг асинхронного режима работы
			bool _async;
		private:
			// Уровень логирования
			level_t _level;
		private:
			// Флаг формирования разделителя
			separator_t _sep;
		private:
			// Название сервиса для вывода лога
			string _name;
			// Формат даты и времени для вывода лога
			string _format;
			// Адрес файла для сохранения логов
			string _filename;
		private:
			// Максимальный размер файла лога
			size_t _maxSize;
			// Размер сообщения для формирования разделителя
			size_t _sepSize;
			// Максимальный размер очереди асинхронного вывода (0 - без ограничения)
			size_t _maxQueue;
			// Максимальное количество хранимых архивов логов (0 - без ограничения)
			size_t _maxFiles;
		private:
			// Объект работы с датой и временем
			chrono_t _chrono;
		private:
			// Политика поведения при переполнении очереди асинхронного вывода
			overflow_t _overflow;
		private:
			// Список доступных флагов
			unordered_set <mode_t> _mode;
		private:
			// Идентификатор процесса, владеющего асинхронным потоком
			mutable atomic <pid_t> _pid;
		private:
			// Счётчик для сброса накопленных логов
			mutable atomic_uint8_t _counter;
		private:
			// Объект работы с дочерними потоками
			mutable screen_t <payload_t> _screen;
		private:
			// Мютекс для блокировки потока
			mutable lock_state_t <std::mutex> _mtx;
		private:
			// Набор приёмников вывода логов, построенный по текущему списку режимов
			mutable vector <unique_ptr <Sink>> _sinks;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова которая срабатывает при появлении лога
			 *
			 * @details Функция обратного вызова должна быть установлена извне через setCallback()
			 *          и срабатывать в контексте потока, который вызвал метод debug().
			 *          Если функция обратного вызова не установлена, то полезная нагрузка
			 *          будет передаваться в приёмники вывода логов (консоль, файл, SysLog) согласно текущему списку режимов.
			 *
			 * @param flag    флаг типа логирования
			 * @param message текстовое описание полезной нагрузки
			 *
			 * \~english
			 * @brief Callback function that fires on the appearance of a log
			 *
			 * @details The callback function must be set from the outside through setCallback()
			 *          and fire in the context of the thread that called the debug() method.
			 *          If the callback function is not set, the payload
			 *          will be handed over into the sinks of the log output (console, file, SysLog) according to the current list of modes.
			 *
			 * @param flag    flag of the type of the logging
			 * @param message text description of the payload
			 *
			 * \~
			 */
			function <void (const flag_t, string_view)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода определения количества аргументов
			 *
			 * @tparam TupType тип аргументов
			 *
			 * \~english
			 * @brief Template of the method of determining the number of arguments
			 *
			 * @tparam TupType type of the arguments
			 *
			 * \~
			 */
			template <typename TupType>
			/**
			 * \~russian
			 * @brief Метод определения количества аргументов
			 *
			 * @param args аргументы для определения их количества
			 * @return     количество найденных аргументов
			 *
			 * \~english
			 * @brief Method of determining the number of arguments
			 *
			 * @param args arguments to determine their number of
			 * @return     number of the found arguments
			 *
			 * \~
			 */
			size_t count(TupType) const noexcept {
				// Возвращаем количество переданных аргументов
				return std::tuple_size_v <TupType>;
			}
		private:
			/**
			 * \~russian
			 * @brief Шаблон метода формирования строки аргументов
			 *
			 * @tparam TupType тип аргументов
			 * @tparam I       список последовательности
			 *
			 * \~english
			 * @brief Template of the method of building the string of the arguments
			 *
			 * @tparam TupType type of the arguments
			 * @tparam I       list of the sequence
			 *
			 * \~
			 */
			template <class TupType, size_t... I>
			/**
			 * \~russian
			 * @brief Метод формирования строки аргументов
			 *
			 * @param args аргументы для формирования строки
			 * @return     сформированная строка аргументов
			 *
			 * \~english
			 * @brief Method of building the string of the arguments
			 *
			 * @param args arguments to build the string from
			 * @return     the built string of the arguments
			 *
			 * \~
			 */
			string formation(const TupType & args, index_sequence <I...>) const noexcept {
				// Создаём объект строкового потока
				stringstream ss;
				// Выполняем добавление открывающую скобку
				ss << "(";
				// Выполняем запись всех аргументов
				(..., (ss << (I == 0 ? "" : ", ") << get <I> (args)));
				// Выполняем добавление закрывающую скобку
				ss << ")";
				// Возвращаем результат
				return ss.str();
			}
			/**
			 * \~russian
			 * @brief Шаблон входных параметров для серриализатора
			 *
			 * @tparam TupType тип аргументов
			 *
			 * \~english
			 * @brief Template of the input parameters for the serialiser
			 *
			 * @tparam TupType type of the arguments
			 *
			 * \~
			 */
			template <class... TupType>
			/**
			 * \~russian
			 * @brief Метод серриализации входных аргументов
			 *
			 * @param args аргументы для серриализации
			 * @return     сформированная строка аргументов
			 *
			 * \~english
			 * @brief Method of serialising the input arguments
			 *
			 * @param args arguments to serialise
			 * @return     the built string of the arguments
			 *
			 * \~
			 */
			string serialization(const tuple <TupType...> & args) const noexcept {
				// Выполняем серриализацию полученных аргументов
				return this->formation(args, make_index_sequence <sizeof...(TupType)> ());
			}
		private:
			/**
			 * \~russian
			 * @brief Метод перестроения набора приёмников по текущему списку режимов
			 *
			 * \~english
			 * @brief Method of rebuilding the set of the sinks by the current list of modes
			 *
			 * \~
			 */
			void rebuild() noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод проверки разрешён ли вывод лога для указанного флага
			 *
			 * @param flag флаг типа логирования
			 * @return     результат проверки соответствия уровню логирования
			 *
			 * \~english
			 * @brief Method of checking whether the log output is allowed for the specified flag
			 *
			 * @param flag flag of the type of the logging
			 * @return     result of checking the correspondence to the logging level
			 *
			 * \~
			 */
			bool allowed(const flag_t flag) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод очистки строки от символов форматирования
			 *
			 * @param text текст для очистки
			 * @return     очищенный текст
			 *
			 * \~english
			 * @brief Method of clearing a string of the formatting characters
			 *
			 * @param text text to clear
			 * @return     the cleared text
			 *
			 * \~
			 */
			string & cleaner(string & text) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод маршрутизации полезной нагрузки в приёмники (синхронно или асинхронно)
			 *
			 * @param payload объект полезной нагрузки
			 *
			 * \~english
			 * @brief Method of routing the payload into the sinks (synchronously or asynchronously)
			 *
			 * @param payload payload object
			 *
			 * \~
			 */
			void dispatch(payload_t && payload) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод получения данных
			 *
			 * @param payload объект полезной нагрузки
			 *
			 * \~english
			 * @brief Method of receiving the data
			 *
			 * @param payload payload object
			 *
			 * \~
			 */
			void receiving(const payload_t & payload) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод извлечения компонента адреса файла
			 *
			 * @param filename адрес где находится файл
			 * @return         параметры компонента (адрес, название файла без расширения)
			 *
			 * \~english
			 * @brief Method of getting the component of the address of a file
			 *
			 * @param filename address where the file is located
			 * @return         parameters of the component (address, name of the file without the extension)
			 *
			 * \~
			 */
			pair <string, string> components(string_view filename) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод формирования итоговой строки лога
			 *
			 * @param payload объект полезной нагрузки
			 * @param colored нужно ли добавлять символы цветового форматирования
			 * @return        сформированная строка лога
			 *
			 * \~english
			 * @brief Method of building the resulting string of the log
			 *
			 * @param payload payload object
			 * @param colored whether the colour formatting characters should be added
			 * @return        the built string of the log
			 *
			 * \~
			 */
			string compose(const payload_t & payload, const bool colored) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T    тип входных аргументов функции
			 * @tparam Args список входящих аргументов
			 *
			 * \~english
			 * @brief Template of the method of outputting text information into the console or into a file
			 *
			 * @tparam T    type of the input arguments of the function
			 * @tparam Args list of the incoming arguments
			 *
			 * \~
			 */
			template <class... T, typename... Args>
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   аргументы формирования лога
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param method name of the called method
			 * @param params parameters passed into the method
			 * @param flag   flag of the type of the logging
			 * @param args   arguments of the building of the log
			 *
			 * \~
			 */
			void debug(string_view format, string_view method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m" AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m" AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m" AWH_STRING_BREAK);
						}
						// Добавляем формат сообщения
						debug.append(format);
						// Пишем полученный лог
						this->print(debug, flag, args...);
					// Пишем лог без изменений
					} else this->print(format, flag, args...);
				}
			}
			/**
			 * \~russian
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T    тип входных аргументов функции
			 * @tparam Args список входящих аргументов
			 *
			 * \~english
			 * @brief Template of the method of outputting text information into the console or into a file
			 *
			 * @tparam T    type of the input arguments of the function
			 * @tparam Args list of the incoming arguments
			 *
			 * \~
			 */
			template <class... T, typename... Args>
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   аргументы формирования лога
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param method name of the called method
			 * @param params parameters passed into the method
			 * @param flag   flag of the type of the logging
			 * @param args   arguments of the building of the log
			 *
			 * \~
			 */
			void debug(wstring_view format, string_view method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m" AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m" AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m" AWH_STRING_BREAK);
						}
						// Добавляем формат сообщения
						debug.append(this->_fmk->convert(format));
						// Пишем полученный лог
						this->print(debug, flag, args...);
					// Пишем лог без изменений
					} else this->print(format, flag, args...);
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T тип входных аргументов функции
			 *
			 * \~english
			 * @brief Template of the method of outputting text information into the console or into a file
			 *
			 * @tparam T type of the input arguments of the function
			 *
			 * \~
			 */
			template <class... T>
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param method name of the called method
			 * @param params parameters passed into the method
			 * @param flag   flag of the type of the logging
			 * @param args   list of the arguments for the substitution
			 *
			 * \~
			 */
			void debug(string_view format, string_view method, const tuple <T...> & params, flag_t flag, const vector <string> & args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m" AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m" AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m" AWH_STRING_BREAK);
						}
						// Добавляем формат сообщения
						debug.append(format);
						// Пишем полученный лог
						this->print(debug, flag, args);
					// Пишем лог без изменений
					} else this->print(format, flag, args);
				}
			}
			/**
			 * \~russian
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T тип входных аргументов функции
			 *
			 * \~english
			 * @brief Template of the method of outputting text information into the console or into a file
			 *
			 * @tparam T type of the input arguments of the function
			 *
			 * \~
			 */
			template <class... T>
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param method name of the called method
			 * @param params parameters passed into the method
			 * @param flag   flag of the type of the logging
			 * @param args   list of the arguments for the substitution
			 *
			 * \~
			 */
			void debug(wstring_view format, string_view method, const tuple <T...> & params, flag_t flag, const vector <wstring> & args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m" AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m" AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m" AWH_STRING_BREAK);
						}
						// Добавляем формат сообщения
						debug.append(this->_fmk->convert(format));
						// Пишем полученный лог
						this->print(debug, flag, args);
					// Пишем лог без изменений
					} else this->print(format, flag, args);
				}
			}
		public:
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param flag   flag of the type of the logging
			 *
			 * \~
			 */
			void print(string_view format, flag_t flag, ...) const noexcept;
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param flag   flag of the type of the logging
			 *
			 * \~
			 */
			void print(wstring_view format, flag_t flag, ...) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param flag   flag of the type of the logging
			 * @param args   list of the arguments for the substitution
			 *
			 * \~
			 */
			void print(string_view format, flag_t flag, const vector <string> & args) const noexcept;
			/**
			 * \~russian
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 *
			 * \~english
			 * @brief Method of outputting text information into the console or into a file
			 *
			 * @param format format of the output string
			 * @param flag   flag of the type of the logging
			 * @param args   list of the arguments for the substitution
			 *
			 * \~
			 */
			void print(wstring_view format, flag_t flag, const vector <wstring> & args) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 *
			 * \~english
			 * @brief Method of setting the thread safety of the work
			 *
			 * @param mode flag of the thread safety mode
			 *
			 * \~
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения установленного формата лога
			 *
			 * @return формат лога для извлечения
			 *
			 * \~english
			 * @brief Method of getting the set format of the log
			 *
			 * @return format of the log to get
			 *
			 * \~
			 */
			const string & format() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки формата даты и времени для вывода лога
			 *
			 * @param format формат даты и времени для вывода лога
			 *
			 * \~english
			 * @brief Method of setting the format of the date and time for the log output
			 *
			 * @param format format of the date and time for the log output
			 *
			 * \~
			 */
			void format(string_view format) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения установленных режимов вывода логов
			 *
			 * @return список режимов вывода логов
			 *
			 * \~english
			 * @brief Method of getting the set modes of the log output
			 *
			 * @return list of the modes of the log output
			 *
			 * \~
			 */
			const unordered_set <mode_t> & mode() const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления режимов вывода логов
			 *
			 * @param mode список режимов вывода логов
			 *
			 * \~english
			 * @brief Method of adding the modes of the log output
			 *
			 * @param mode list of the modes of the log output
			 *
			 * \~
			 */
			void mode(const unordered_set <mode_t> & mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки название сервиса для вывода лога
			 *
			 * @param name название сервиса для вывода лога
			 *
			 * \~english
			 * @brief Method of setting the name of the service for the log output
			 *
			 * @param name name of the service for the log output
			 *
			 * \~
			 */
			void name(string_view name) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки флага асинхронного режима работы
			 *
			 * @param mode флаг асинхронного режима работы
			 *
			 * \~english
			 * @brief Method of setting the flag of the asynchronous mode of the work
			 *
			 * @param mode flag of the asynchronous mode of the work
			 *
			 * \~
			 */
			void async(const bool mode) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального размера файла логов
			 *
			 * @param size максимальный размер файла логов
			 *
			 * \~english
			 * @brief Method of setting the maximum size of the log file
			 *
			 * @param size maximum size of the log file
			 *
			 * \~
			 */
			void maxSize(const float size) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки размера текста для формирования разделителя
			 *
			 * @param size размер текста для формирования разделителя
			 *
			 * \~english
			 * @brief Method of setting the size of the text for the building of the separator
			 *
			 * @param size size of the text for the building of the separator
			 *
			 * \~
			 */
			void sepSize(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки уровня логирования
			 *
			 * @param level уровень логирования для установки
			 *
			 * \~english
			 * @brief Method of setting the logging level
			 *
			 * @param level logging level to set
			 *
			 * \~
			 */
			void level(const level_t level) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального размера очереди асинхронного вывода
			 *
			 * @param size максимальный размер очереди (0 - без ограничения)
			 *
			 * \~english
			 * @brief Method of setting the maximum size of the queue of the asynchronous output
			 *
			 * @param size maximum size of the queue (0 — without a limit)
			 *
			 * \~
			 */
			void maxQueue(const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки максимального количества хранимых архивов логов
			 *
			 * @param count максимальное количество архивов (0 - без ограничения)
			 *
			 * \~english
			 * @brief Method of setting the maximum number of the kept log archives
			 *
			 * @param count maximum number of the archives (0 — without a limit)
			 *
			 * \~
			 */
			void maxFiles(const size_t count) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки файла для сохранения логов
			 *
			 * @param filename путь к файлу для сохранения логов
			 *
			 * \~english
			 * @brief Method of setting the file for saving the logs
			 *
			 * @param filename path to the file for saving the logs
			 *
			 * \~
			 */
			void filename(string_view filename) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки разделителя сообщений логирования
			 *
			 * @param sep разделитель для установки
			 *
			 * \~english
			 * @brief Method of setting the separator of the logging messages
			 *
			 * @param sep separator to set
			 *
			 * \~
			 */
			void separator(const separator_t sep) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки политики поведения при переполнении очереди асинхронного вывода
			 *
			 * @param overflow политика поведения при переполнении очереди
			 *
			 * \~english
			 * @brief Method of setting the policy of the behaviour on an overflow of the queue of the asynchronous output
			 *
			 * @param overflow policy of the behaviour on an overflow of the queue
			 *
			 * \~
			 */
			void overflow(const overflow_t overflow) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод подписки на события логов
			 *
			 * @param callback функция обратного вызова
			 *
			 * \~english
			 * @brief Method of subscribing to the log events
			 *
			 * @param callback callback function
			 *
			 * \~
			 */
			void subscribe(function <void (const flag_t, string_view)> callback) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk      объект фреймворка
			 * @param filename путь к файлу для сохранения логов
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param fmk      framework object
			 * @param filename path to the file for saving the logs
			 *
			 * \~
			 */
			explicit Logging(const fmk_t * fmk, string_view filename = "") noexcept;
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
			~Logging() noexcept;
	} log_t;
};

#endif // __AWH_LOG__
