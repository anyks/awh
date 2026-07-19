/**
 * @file: log.hpp
 * @date: 2025-10-25
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
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
	#define AWH_STRING_BREAKS AWH_STRING_BREAK""AWH_STRING_BREAK
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	// Формируем переносы строк лога
	#define AWH_STRING_BREAK "\n"
	#define AWH_STRING_BREAKS AWH_STRING_BREAK""AWH_STRING_BREAK
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
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"
#include "locker.hpp"
#include "chrono.hpp"
#include "screen.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс работы с логами
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Logging {
		public:
			/**
			 * @brief Флаги логирования
			 *
			 */
			enum class flag_t : uint8_t {
				NONE     = 0x00, // Флаг не установлен
				INFO     = 0x01, // Информационное сообщение
				WARNING  = 0x02, // Предупреждающее сообщение
				CRITICAL = 0x03  // Критическое сообщение
			};
			/**
			 * @brief Политика поведения при переполнении очереди асинхронного вывода
			 *
			 * @note значения совпадают с awh::Screen::overflow_t для прямого преобразования
			 */
			enum class overflow_t : uint8_t {
				WAIT     = 0x00, // Блокировать поставщика до появления свободного места
				DROP_NEW = 0x01, // Отбрасывать новое поступившее сообщение
				DROP_OLD = 0x02  // Вытеснять самое старое сообщение из очереди
			};
			/**
			 * @brief Флаги работы логов
			 *
			 */
			enum class mode_t : uint8_t {
				NONE     = 0x00, // Вывод логов запрещён
				FILE     = 0x01, // Разрешено выводить логи в файлы
				SYSLOG   = 0x02, // Разрешено отправлять логи в SysLog
				CONSOLE  = 0x03, // Разрешено выводить логи в консоль
				DEFERRED = 0x04  // Разрешено выводить логи в функцию обратного вызова
			};
			/**
			 * @brief Флаги разделителя формирования логов
			 *
			 */
			enum class separator_t : uint8_t {
				NONE   = 0x00, // Разделитель отключён
				SMART  = 0x01, // Умный разделитель по длине сообщения
				ALWAYS = 0x02  // Отображать разделитель всегда
			};
			/**
			 * @brief Уровни логирования
			 *
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
			 * @brief Класс полезной нагрузки
			 *
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
					 * @brief Оператор перемещающего присваивания параметров полезной нагрузки
					 *
					 * @param payload объект полезной нагрузки для перемещения
					 * @return        текущий объект полезной нагрузки
					 */
					Payload & operator = (Payload && payload) noexcept;
					/**
					 * @brief Оператор присваивания присваивания параметров полезной нагрузки
					 *
					 * @param payload объект полезной нагрузки для копирования
					 * @return        текущий объект полезной нагрузки
					 */
					Payload & operator = (const Payload & payload) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param payload объект полезной нагрузки для сравнения
					 * @return        результат сравнения
					 */
					bool operator == (const Payload & payload) noexcept;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param payload объект полезной нагрузки для перемещения
					 */
					Payload(Payload && payload) noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 * @param payload объект полезной нагрузки для копирования
					 */
					Payload(const Payload & payload) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Payload() noexcept;
				public:
					/**
					 * @brief Деструктор
					 *
					 */
					~Payload() noexcept {}
			} payload_t;
		private:
			/**
			 * @brief Базовый абстрактный приёмник вывода логов
			 *
			 */
			class Sink {
				protected:
					// Указатель на владеющий объект логирования
					const Logging * _log;
				public:
					/**
					 * @brief Метод записи полезной нагрузки в приёмник
					 *
					 * @param payload объект полезной нагрузки
					 */
					virtual void write(const payload_t & payload) const noexcept = 0;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 */
					explicit Sink(const Logging * log) noexcept : _log(log) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Sink() noexcept {}
			};
			/**
			 * @brief Приёмник вывода логов в консоль
			 *
			 */
			class ConsoleSink : public Sink {
				public:
					/**
					 * @brief Метод записи полезной нагрузки в консоль
					 *
					 * @param payload объект полезной нагрузки
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 */
					explicit ConsoleSink(const Logging * log) noexcept : Sink(log) {}
			};
			/**
			 * @brief Приёмник вывода логов в файл
			 *
			 */
			class FileSink : public Sink {
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
					 * @brief Метод (пере)открытия постоянного дескриптора записи
					 *
					 */
					void reopen() const noexcept;
					/**
					 * @brief Метод выполнения ротации файла лога
					 *
					 */
					void rotate() const noexcept;
					/**
					 * @brief Метод удаления устаревших архивов логов (retention)
					 *
					 */
					void retention() const noexcept;
				private:
					/**
					 * @brief Метод формирования уникального имени архива логов
					 *
					 * @return путь к файлу архива, гарантированно не конфликтующий с существующими
					 */
					string nextArchive() const noexcept;
				public:
					/**
					 * @brief Метод записи полезной нагрузки в файл
					 *
					 * @param payload объект полезной нагрузки
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 */
					explicit FileSink(const Logging * log) noexcept :
					 Sink(log), _pid(0), _fd(-1), _opened{""}, _size(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~FileSink() noexcept;
			};
			/**
			 * @brief Приёмник отправки логов в SysLog
			 *
			 */
			class SyslogSink : public Sink {
				public:
					/**
					 * @brief Метод отправки полезной нагрузки в SysLog
					 *
					 * @param payload объект полезной нагрузки
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 */
					explicit SyslogSink(const Logging * log) noexcept : Sink(log) {}
			};
			/**
			 * @brief Приёмник передачи логов в функцию обратного вызова
			 *
			 */
			class CallbackSink : public Sink {
				public:
					/**
					 * @brief Метод передачи полезной нагрузки в функцию обратного вызова
					 *
					 * @param payload объект полезной нагрузки
					 */
					void write(const payload_t & payload) const noexcept override;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param log объект логирования
					 */
					explicit CallbackSink(const Logging * log) noexcept : Sink(log) {}
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
			 * @brief Функция обратного вызова которая срабатывает при появлении лога
			 *
			 * @details Функция обратного вызова должна быть установлена извне через setCallback()
			 *          и срабатывать в контексте потока, который вызвал метод debug().
			 *          Если функция обратного вызова не установлена, то полезная нагрузка
			 *          будет передаваться в приёмники вывода логов (консоль, файл, SysLog) согласно текущему списку режимов.
			 *
			 * @param flag    флаг типа логирования
			 * @param message текстовое описание полезной нагрузки
			 */
			function <void (const flag_t, string_view)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * @brief Шаблон метода определения количества аргументов
			 *
			 * @tparam TupType тип аргументов
			 */
			template <typename TupType>
			/**
			 * @brief Метод определения количества аргументов
			 *
			 * @param args аргументы для определения их количества
			 * @return     количество найденных аргументов
			 */
			size_t count(TupType) const noexcept {
				// Возвращаем количество переданных аргументов
				return std::tuple_size_v <TupType>;
			}
		private:
			/**
			 * @brief Шаблон метода формирования строки аргументов
			 *
			 * @tparam TupType тип аргументов
			 * @tparam I       список последовательности
			 */
			template <class TupType, size_t... I>
			/**
			 * @brief Метод формирования строки аргументов
			 *
			 * @param args аргументы для формирования строки
			 * @return     сформированная строка аргументов
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
			 * @brief Шаблон входных параметров для серриализатора
			 *
			 * @tparam TupType тип аргументов
			 */
			template <class... TupType>
			/**
			 * @brief Метод серриализации входных аргументов
			 *
			 * @param args аргументы для серриализации
			 * @return     сформированная строка аргументов
			 */
			string serialization(const tuple <TupType...> & args) const noexcept {
				// Выполняем серриализацию полученных аргументов
				return this->formation(args, make_index_sequence <sizeof...(TupType)> ());
			}
		private:
			/**
			 * @brief Метод перестроения набора приёмников по текущему списку режимов
			 *
			 */
			void rebuild() noexcept;
		private:
			/**
			 * @brief Метод проверки разрешён ли вывод лога для указанного флага
			 *
			 * @param flag флаг типа логирования
			 * @return     результат проверки соответствия уровню логирования
			 */
			bool allowed(const flag_t flag) const noexcept;
		private:
			/**
			 * @brief Метод очистки строки от символов форматирования
			 *
			 * @param text текст для очистки
			 * @return     очищенный текст
			 */
			string & cleaner(string & text) const noexcept;
		private:
			/**
			 * @brief Метод маршрутизации полезной нагрузки в приёмники (синхронно или асинхронно)
			 *
			 * @param payload объект полезной нагрузки
			 */
			void dispatch(payload_t && payload) const noexcept;
		private:
			/**
			 * @brief Метод получения данных
			 *
			 * @param payload объект полезной нагрузки
			 */
			void receiving(const payload_t & payload) const noexcept;
		private:
			/**
			 * @brief Метод извлечения компонента адреса файла
			 *
			 * @param filename адрес где находится файл
			 * @return         параметры компонента (адрес, название файла без расширения)
			 */
			std::pair <string, string> components(string_view filename) const noexcept;
		private:
			/**
			 * @brief Метод формирования итоговой строки лога
			 *
			 * @param payload объект полезной нагрузки
			 * @param colored нужно ли добавлять символы цветового форматирования
			 * @return        сформированная строка лога
			 */
			string compose(const payload_t & payload, const bool colored) const noexcept;
		public:
			/**
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T    тип входных аргументов функции
			 * @tparam Args список входящих аргументов
			 */
			template <class... T, typename... Args>
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   аргументы формирования лога
			 */
			void debug(string_view format, string_view method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m"AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m"AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m"AWH_STRING_BREAK);
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
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T    тип входных аргументов функции
			 * @tparam Args список входящих аргументов
			 */
			template <class... T, typename... Args>
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   аргументы формирования лога
			 */
			void debug(wstring_view format, string_view method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m"AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m"AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m"AWH_STRING_BREAK);
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
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T тип входных аргументов функции
			 */
			template <class... T>
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void debug(string_view format, string_view method, const tuple <T...> & params, flag_t flag, const vector <string> & args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m"AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m"AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m"AWH_STRING_BREAK);
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
			 * @brief Шаблон метода вывода текстовой информации в консоль или файл
			 *
			 * @tparam T тип входных аргументов функции
			 */
			template <class... T>
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param method название вызываемого метода
			 * @param params параметры переданные в метод
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void debug(wstring_view format, string_view method, const tuple <T...> & params, flag_t flag, const vector <wstring> & args) const noexcept {
				// Если формат строки вывода передан
				if(!format.empty()){
					// Если метод названия функции передан
					if(!method.empty()){
						// Формируем результирующую строку отладки
						string debug = AWH_STRING_BREAKS"\x1B[1mCalled function:\x1B[0m"AWH_STRING_BREAK;
						// Добавляем название метода
						debug.append(method);
						// Добавляем перенос строки
						debug.append(AWH_STRING_BREAKS);
						// Если аргументы функции переданы
						if(this->count(params) > 0){
							// Добавляем входные аргументы функции
							debug.append("\x1B[1mArguments function:\x1B[0m"AWH_STRING_BREAK);
							// Добавляем список аргументов функции
							debug.append(this->serialization(params));
							// Добавляем перенос строки
							debug.append(AWH_STRING_BREAKS);
							// Добавляем описание входящего сообщения
							debug.append("\x1B[1mMessage:\x1B[0m"AWH_STRING_BREAK);
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
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 */
			void print(string_view format, flag_t flag, ...) const noexcept;
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 */
			void print(wstring_view format, flag_t flag, ...) const noexcept;
		public:
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void print(string_view format, flag_t flag, const vector <string> & args) const noexcept;
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void print(wstring_view format, flag_t flag, const vector <wstring> & args) const noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param mode флаг режима безопасности потоков
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод извлечения установленного формата лога
			 *
			 * @return формат лога для извлечения
			 */
			const string & format() const noexcept;
			/**
			 * @brief Метод установки формата даты и времени для вывода лога
			 *
			 * @param format формат даты и времени для вывода лога
			 */
			void format(string_view format) noexcept;
		public:
			/**
			 * @brief Метод получения установленных режимов вывода логов
			 *
			 * @return список режимов вывода логов
			 */
			const unordered_set <mode_t> & mode() const noexcept;
			/**
			 * @brief Метод добавления режимов вывода логов
			 *
			 * @param mode список режимов вывода логов
			 */
			void mode(const unordered_set <mode_t> & mode) noexcept;
		public:
			/**
			 * @brief Метод установки название сервиса для вывода лога
			 *
			 * @param name название сервиса для вывода лога
			 */
			void name(string_view name) noexcept;
			/**
			 * @brief Метод установки флага асинхронного режима работы
			 *
			 * @param mode флаг асинхронного режима работы
			 */
			void async(const bool mode) noexcept;
			/**
			 * @brief Метод установки максимального размера файла логов
			 *
			 * @param size максимальный размер файла логов
			 */
			void maxSize(const float size) noexcept;
			/**
			 * @brief Метод установки размера текста для формирования разделителя
			 *
			 * @param size размер текста для формирования разделителя
			 */
			void sepSize(const size_t size) noexcept;
			/**
			 * @brief Метод установки уровня логирования
			 *
			 * @param level уровень логирования для установки
			 */
			void level(const level_t level) noexcept;
			/**
			 * @brief Метод установки максимального размера очереди асинхронного вывода
			 *
			 * @param size максимальный размер очереди (0 - без ограничения)
			 */
			void maxQueue(const size_t size) noexcept;
			/**
			 * @brief Метод установки максимального количества хранимых архивов логов
			 *
			 * @param count максимальное количество архивов (0 - без ограничения)
			 */
			void maxFiles(const size_t count) noexcept;
			/**
			 * @brief Метод установки файла для сохранения логов
			 *
			 * @param filename путь к файлу для сохранения логов
			 */
			void filename(string_view filename) noexcept;
			/**
			 * @brief Метод установки разделителя сообщений логирования
			 *
			 * @param sep разделитель для установки
			 */
			void separator(const separator_t sep) noexcept;
			/**
			 * @brief Метод установки политики поведения при переполнении очереди асинхронного вывода
			 *
			 * @param overflow политика поведения при переполнении очереди
			 */
			void overflow(const overflow_t overflow) noexcept;
		public:
			/**
			 * @brief Метод подписки на события логов
			 *
			 * @param callback функция обратного вызова
			 */
			void subscribe(function <void (const flag_t, string_view)> callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk      объект фреймворка
			 * @param filename путь к файлу для сохранения логов
			 */
			explicit Logging(const fmk_t * fmk, string_view filename = "") noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Logging() noexcept;
	} log_t;
};

#endif // __AWH_LOG__
