/**
 * @file: log.hpp
 * @date: 2021-12-19
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
 * Стандартные модули
 */
#include <set>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdarg>
#include <functional>
#include <zlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>

/**
 * Наши модули
 */
#include "fmk.hpp"
#include "chrono.hpp"
#include "screen.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с логами
	 *
	 */
	typedef class AWHSHARED_EXPORT Log {
		public:
			/**
			 * Флаги логирования
			 */
			enum class flag_t : uint8_t {
				NONE     = 0x00, // Флаг не установлен
				INFO     = 0x01, // Информационное сообщение
				WARNING  = 0x02, // Предупреждающее сообщение
				CRITICAL = 0x03  // Критическое сообщение
			};
			/**
			 * Флаги работы логов
			 */
			enum class mode_t : uint8_t {
				NONE     = 0x00, // Вывод логов запрещён
				FILE     = 0x01, // Разерешно выводить логи в файлы
				CONSOLE  = 0x02, // Разрешено выводить логи в консоль
				DEFERRED = 0x03  // Разрешено выводить логи в функцию обратного вызова
			};
			/**
			 * Флаги разделителя формирования логов
			 */
			enum class separator_t : uint8_t {
				NONE   = 0x00, // Разделитель отключён
				SMART  = 0x01, // Умный разделитель по длине сообщения
				ALWAYS = 0x02  // Отображать разделитель всегда
			};
			/**
			 * Уровни логирования
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
			typedef class AWHSHARED_EXPORT Payload {
				public:
					// Флаг полезной нагрузки
					flag_t flag;
					// Текст полезной нагрузки
					string text;
				public:
					/**
					 * @brief Оператор [=] перемещения параметров полезной нагрузки
					 *
					 * @param payload объект полезной нагрузки для перемещения
					 * @return        текущий объект полезной нагрузки
					 */
					Payload & operator = (Payload && payload) noexcept;
					/**
					 * @brief Оператор [=] присванивания параметров полезной нагрузки
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
			// Идентификатор родительского процесса
			pid_t _pid;
		private:
			// Флаг асинхронного режима работы
			bool _async;
		private:
			// Максимальный размер файла лога
			size_t _maxSize;
		private:
			// Размер сообщения для формирования разделителя
			size_t _sepSize;
		private:
			// Уровень логирования
			level_t _level;
		private:
			// Флаг формирования разделителя
			separator_t _sep;
		private:
			// Объект работы с датой и временем
			chrono_t _chrono;
		private:
			// Название сервиса для вывода лога
			string _name;
			// Формат даты и времени для вывода лога
			string _format;
			// Адрес файла для сохранения логов
			string _filename;
		private:
			// Список доступных флагов
			std::set <mode_t> _mode;
		private:
			// Список проинициализированных процессов
			mutable std::set <pid_t> _initialized;
		private:
			// Мютекс для блокировки потока
			mutable std::recursive_mutex _mtx;
		private:
			// Объект работы с дочерними потоками
			mutable screen_t <payload_t> _screen;
		private:
			/**
			 * Функция обратного вызова которая срабатывает при появлении лога
			 */
			function <void (const flag_t, const string &)> _fn;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * @brief Шаблон типа аргументов
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
				// Выводим количество переданных аргументов
				return tuple_size_v <TupType>;
			}
		private:
			/**
			 * @brief Шаблон входных параметров для серриализатора
			 *
			 * @tclass TupType тип аргументов
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
				// Выводим полученный результат
				return ss.str();
			}
			/**
			 * @brief Шаблон входных параметров для серриализатора
			 *
			 * @tclass TupType тип аргументов
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
			 * @brief Метод выполнения ротации логов
			 *
			 */
			void rotate() const noexcept;
		private:
			/**
			 * @brief Метод очистки строки от символов форматирования
			 *
			 * @param text текст для очистки
			 * @return     ощиченный текста
			 */
			string & cleaner(string & text) const noexcept;
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
			std::pair <string, string> components(const string & filename) const noexcept;
		public:
			/**
			 * @brief Шаблон входных аргументов функции
			 *
			 * @tclass T    тип входных аргументов функции
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
			void debug(const string & format, const string & method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
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
						// Выводим полученный нами лог
						this->print(debug + format, flag, args...);
					// Выводим лог в том виде как он пришёл
					} else this->print(format, flag, args...);
				}
			}
			/**
			 * @brief Шаблон входных аргументов функции
			 *
			 * @tclass T    тип входных аргументов функции
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
			void debug(const wstring & format, const string & method, const tuple <T...> & params, flag_t flag, Args&&... args) const noexcept {
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
						// Выводим полученный нами лог
						this->print(this->_fmk->convert(debug) + format, flag, args...);
					// Выводим лог в том виде как он пришёл
					} else this->print(format, flag, args...);
				}
			}
		public:
			/**
			 * @brief Шаблон входных аргументов функции
			 *
			 * @tclass T тип входных аргументов функции
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
			void debug(const string & format, const string & method, const tuple <T...> & params, flag_t flag, const vector <string> & args) const noexcept {
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
						// Выводим полученный нами лог
						this->print(debug + format, flag, args);
					// Выводим лог в том виде как он пришёл
					} else this->print(format, flag, args);
				}
			}
			/**
			 * @brief Шаблон входных аргументов функции
			 *
			 * @tclass T тип входных аргументов функции
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
			void debug(const wstring & format, const string & method, const tuple <T...> & params, flag_t flag, const vector <wstring> & args) const noexcept {
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
						// Выводим полученный нами лог
						this->print(this->_fmk->convert(debug) + format, flag, args);
					// Выводим лог в том виде как он пришёл
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
			void print(const string & format, flag_t flag, ...) const noexcept;
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 */
			void print(const wstring & format, flag_t flag, ...) const noexcept;
		public:
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void print(const string & format, flag_t flag, const vector <string> & args) const noexcept;
			/**
			 * @brief Метод вывода текстовой информации в консоль или файл
			 *
			 * @param format формат строки вывода
			 * @param flag   флаг типа логирования
			 * @param args   список аргументов для замены
			 */
			void print(const wstring & format, flag_t flag, const vector <wstring> & args) const noexcept;
		public:
			/**
			 * @brief Метод получения установленных режимов вывода логов
			 *
			 * @return список режимов вывода логов
			 */
			const std::set <mode_t> & mode() const noexcept;
			/**
			 * @brief Метод добавления режимов вывода логов
			 *
			 * @param mode список режимов вывода логов
			 */
			void mode(const std::set <mode_t> & mode) noexcept;
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
			void format(const string & format) noexcept;
		public:
			/**
			 * @brief Метод установки флага асинхронного режима работы
			 *
			 * @param mode флаг асинхронного режима работы
			 */
			void async(const bool mode) noexcept;
			/**
			 * @brief Метод установки название сервиса для вывода лога
			 *
			 * @param name название сервиса для вывода лога
			 */
			void name(const string & name) noexcept;
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
			 * @brief Метод установки разделителя сообщений логирования
			 *
			 * @param sep разделитель для установки
			 */
			void separator(const separator_t sep) noexcept;
			/**
			 * @brief Метод установки файла для сохранения логов
			 *
			 * @param filename адрес файла для сохранения логов
			 */
			void filename(const string & filename) noexcept;
		public:
			/**
			 * @brief Метод подписки на события логов
			 *
			 * @param callback функция обратного вызова
			 */
			void subscribe(function <void (const flag_t, const string &)> callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk      объект фреймворка
			 * @param filename адрес файла для сохранения логов
			 */
			Log(const fmk_t * fmk, const string & filename = "") noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Log() noexcept;
	} log_t;
};

#endif // __AWH_LOG__
